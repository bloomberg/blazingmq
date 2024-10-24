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

// mqbc_storagemanager.cpp                                            -*-C++-*-
#include <mqbc_storagemanager.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterutil.h>
#include <mqbc_recoverymanager.h>
#include <mqbi_storage.h>
#include <mqbnet_cluster.h>
#include <mqbs_storageprintutil.h>
#include <mqbu_exit.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_storagemessageiterator.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bsl_algorithm.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbc {

namespace {
const int k_GC_MESSAGES_INTERVAL_SECONDS = 30;

bool isPrimaryActive(const mqbi::StorageManager_PartitionInfo pinfo)
{
    return pinfo.primaryStatus() == bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE;
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
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_recoveryManager_mp);

    BALL_LOG_INFO_BLOCK
    {
        StorageUtil::printRecoveryPhaseOneBanner(
            BALL_LOG_OUTPUT_STREAM,
            d_clusterData_p->identity().description(),
            partitionId);
    }

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping partition recovery.";
        return;  // RETURN
    }

    d_recoveryStartTimes[partitionId] = bmqsys::Time::highResolutionTimer();
}

void StorageManager::sendMessage(const bmqp_ctrlmsg::ControlMessage& message,
                                 mqbnet::ClusterNode* destination)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(destination);

    d_clusterData_p->messageTransmitter().sendMessage(message, destination);
}

void StorageManager::shutdownCb(int partitionId, bslmt::Latch* latch)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'
    StorageUtil::shutdown(partitionId,
                          latch,
                          &d_fileStores,
                          d_clusterData_p->identity().description(),
                          d_clusterConfig);
}

void StorageManager::recoveredQueuesCb(int                    partitionId,
                                       const QueueKeyInfoMap& queueKeyInfoMap)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    const int numPartitions = static_cast<int>(d_fileStores.size());
    BSLS_ASSERT_SAFE(0 <= partitionId && partitionId < numPartitions);
    BSLS_ASSERT_SAFE(d_numPartitionsRecoveredQueues < numPartitions);

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping partition recovery.";
        return;  // RETURN
    }

    // Main logic
    StorageUtil::recoveredQueuesCb(&d_storages[partitionId],
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
                                   true);  // isCSLMode

    if (++d_numPartitionsRecoveredQueues < numPartitions) {
        return;  // RETURN
    }

    StorageUtil::dumpUnknownRecoveredDomains(
        d_clusterData_p->identity().description(),
        &d_unrecognizedDomainsLock,
        d_unrecognizedDomains);
}

void StorageManager::onWatchDog(int partitionId)
{
    // executed by the *SCHEDULER* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&StorageManager::onWatchDogDispatched,
                             this,
                             partitionId),
        d_cluster_p);
}

void StorageManager::onWatchDogDispatched(int partitionId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         d_clusterConfig.partitionConfig().numPartitions());

    BMQTSK_ALARMLOG_ALARM("RECOVERY")
        << d_clusterData_p->identity().description() << " Partition ["
        << partitionId
        << "]: " << "Watch dog triggered because partition startup healing "
        << "sequence was not completed in the configured time of "
        << d_watchDogTimeoutInterval.totalSeconds() << " seconds."
        << BMQTSK_ALARMLOG_END;

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
                              1);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_WATCH_DOG,
                             eventDataVec);
}

void StorageManager::onPartitionDoneSendDataChunksCb(
    int                             partitionId,
    int                             requestId,
    const PartitionSeqNumDataRange& range,
    mqbnet::ClusterNode*            destination,
    int                             status)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received status " << status
                  << " for sending data chunks from Recovery Manager.";

    EventData eventDataVec;
    eventDataVec.emplace_back(destination, requestId, partitionId, 1, range);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    // Note that currently the two FSM events below are no-op if we are the
    // primary.  As replica, we will send either success or failure
    // ReplicaDataResponsePull depending on 'status'.  In the future, it might
    // no longer be no-op for primary.
    if (status != 0) {
        dispatchEventToPartition(
            fs,
            PartitionFSM::Event::e_ERROR_SENDING_DATA_CHUNKS,
            eventDataVec);
    }
    else {
        dispatchEventToPartition(
            fs,
            PartitionFSM::Event::e_DONE_SENDING_DATA_CHUNKS,
            eventDataVec);
    }
}

void StorageManager::onPartitionRecovery(int partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    bmqu::MemOutStream out;
    mqbs::StoragePrintUtil::printRecoveredStorages(
        out,
        &d_storagesLock,
        d_storages[partitionId],
        partitionId,
        d_clusterData_p->identity().description(),
        d_recoveryStartTimes[partitionId]);
    BALL_LOG_INFO << out.str();

    if (++d_numPartitionsRecoveredFully ==
        static_cast<int>(d_fileStores.size())) {
        // First time healing logic

        out.reset();
        const bool success =
            mqbs::StoragePrintUtil::printStorageRecoveryCompletion(
                out,
                d_fileStores,
                d_clusterData_p->identity().description());
        BALL_LOG_INFO << out.str();

        if (success) {
            // All partitions have opened successfully.  Schedule a recurring
            // event in StorageMgr, which will in turn enqueue an event in each
            // partition's dispatcher thread for GC'ing expired messages as
            // well as cleaning history.

            d_clusterData_p->scheduler().scheduleRecurringEvent(
                &d_gcMessagesEventHandle,
                bsls::TimeInterval(k_GC_MESSAGES_INTERVAL_SECONDS),
                bdlf::BindUtil::bind(&StorageManager::forceFlushFileStores,
                                     this));

            // Even though Cluster FSM and all Partition FSMs are now healed,
            // we must check that all partitions have an active primary before
            // transitioning ourself to E_AVAILABLE.
            if (allPartitionsAvailable()) {
                d_recoveryStatusCb(0);
            }
        }
        else {
            d_recoveryStatusCb(-1);
        }
    }
}

void StorageManager::dispatchEventToPartition(mqbs::FileStore*          fs,
                                              PartitionFSM::Event::Enum event,
                                              const EventData& eventDataVec)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << eventDataVec[0].partitionId()
                      << "]: Cluster is stopping; skip dispatching Event '"
                      << event << "' to Partition FSM.";
        return;  // RETURN
    }

    bsl::shared_ptr<bsl::queue<PartitionFSM::EventWithData> > queueSp =
        bsl::allocate_shared<bsl::queue<PartitionFSM::EventWithData> >(
            d_allocator_p);

    // Note that the queueSp lives for the duration of this event and all
    // successive events which may be generated as part of performing included
    // actions as documented in the state transition table for PartitionFSM.
    queueSp->emplace(event, eventDataVec);

    fs->execute(bdlf::BindUtil::bind(
        &PartitionFSM::applyEvent,
        d_partitionFSMVec[eventDataVec[0].partitionId()].get(),
        queueSp));
}

void StorageManager::setPrimaryStatusForPartitionDispatched(
    int                                partitionId,
    bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    if (!pinfo.primary()) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Failed to set primary status to " << value
                       << "because primary is perceived as ** NULL **";

        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(pinfo.primaryLeaseId() > 0);

    const bmqp_ctrlmsg::PrimaryStatus::Value oldValue = pinfo.primaryStatus();
    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: " << "Setting the status of primary: "
                  << pinfo.primary()->nodeDescription()
                  << ", primaryLeaseId: " << pinfo.primaryLeaseId()
                  << ", from " << oldValue << " to " << value << ".";

    pinfo.setPrimaryStatus(value);
    if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == value) {
        d_fileStores[partitionId]->setActivePrimary(pinfo.primary(),
                                                    pinfo.primaryLeaseId());

        if (oldValue != bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE &&
            allPartitionsAvailable()) {
            d_recoveryStatusCb(0);
        }
    }
}

void StorageManager::processPrimaryDetect(int                  partitionId,
                                          mqbnet::ClusterNode* primaryNode,
                                          unsigned int         primaryLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(primaryNode->nodeId() ==
                     d_clusterData_p->membership().selfNode()->nodeId());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping self transition to "
                      << "Primary to the Partition FSM.";
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Self Transition to Primary in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
                              1,
                              primaryNode,
                              primaryLeaseId);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_DETECT_SELF_PRIMARY,
                             eventDataVec);
}

void StorageManager::processReplicaDetect(int                  partitionId,
                                          mqbnet::ClusterNode* primaryNode,
                                          unsigned int         primaryLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(primaryNode->nodeId() !=
                     d_clusterData_p->membership().selfNode()->nodeId());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping self transition to "
                      << "Replica to the Partition FSM.";
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Self Transition to Replica in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
                              1,
                              primaryNode,
                              primaryLeaseId);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_DETECT_SELF_REPLICA,
                             eventDataVec);
}

void StorageManager::processReplicaDataRequestPull(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaDataRequestValue());

    const bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataRequest();

    BSLS_ASSERT_SAFE(replicaDataRequest.replicaDataType() ==
                     bmqp_ctrlmsg::ReplicaDataType::E_PULL);

    const int partitionId = replicaDataRequest.partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received ReplicaDataRequestPull: " << message << " from "
                  << source->nodeDescription() << ".";

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaDataRequestPull.";
        return;  // RETURN
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(
        source,
        message.rId().isNull() ? -1 : message.rId().value(),
        partitionId,
        1,
        PartitionSeqNumDataRange(replicaDataRequest.beginSequenceNumber(),
                                 replicaDataRequest.endSequenceNumber()));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_DATA_RQST_PULL,
                             eventDataVec);
}

void StorageManager::processReplicaDataRequestPush(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaDataRequestValue());

    const bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataRequest();

    BSLS_ASSERT_SAFE(replicaDataRequest.replicaDataType() ==
                     bmqp_ctrlmsg::ReplicaDataType::E_PUSH);

    const int partitionId = replicaDataRequest.partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(
        source->nodeId() ==
        d_clusterState.partitionsInfo().at(partitionId).primaryNodeId());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received ReplicaDataRequestPush: " << message << " from "
                  << source->nodeDescription() << ".";

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaDataRequestPush.";
        return;  // RETURN
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(
        source,
        message.rId().isNull() ? -1 : message.rId().value(),
        partitionId,
        1,
        source,
        d_clusterState.partitionsInfo().at(partitionId).primaryLeaseId(),
        bmqp_ctrlmsg::PartitionSequenceNumber(),
        source,
        PartitionSeqNumDataRange(replicaDataRequest.beginSequenceNumber(),
                                 replicaDataRequest.endSequenceNumber()));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_DATA_RQST_PUSH,
                             eventDataVec);
}

void StorageManager::processReplicaDataRequestDrop(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaDataRequestValue());

    const bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataRequest();

    BSLS_ASSERT_SAFE(replicaDataRequest.replicaDataType() ==
                     bmqp_ctrlmsg::ReplicaDataType::E_DROP);

    const int partitionId = replicaDataRequest.partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(
        source->nodeId() ==
        d_clusterState.partitionsInfo().at(partitionId).primaryNodeId());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received ReplicaDataRequestDrop: " << message << " from "
                  << source->nodeDescription() << ".";

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaDataRequestDrop.";
        return;  // RETURN
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(source,
                              message.rId().isNull() ? -1
                                                     : message.rId().value(),
                              partitionId,
                              1);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_DATA_RQST_DROP,
                             eventDataVec);
}

void StorageManager::processPrimaryStateResponseDispatched(
    const RequestManagerType::RequestSp& context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster().isLocal());

    BSLS_ASSERT_SAFE(context->response().rId() != NULL);
    if (context->request().rId().isNull()) {
        BSLS_ASSERT_SAFE(context->response().rId().isNull());
    }
    else {
        BSLS_ASSERT_SAFE(context->request().rId().value() ==
                         context->response().rId().value());
    }

    const int responseId  = context->response().rId().isNull()
                                ? -1
                                : context->response().rId().value();
    const int partitionId = context->request()
                                .choice()
                                .clusterMessage()
                                .choice()
                                .partitionMessage()
                                .choice()
                                .primaryStateRequest()
                                .partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "PrimaryStateResponse.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (context->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received FAIL_PrmryStateRspn event "
                      << context->response() << " from primary "
                      << responder->nodeDescription();

        EventData eventDataVec;
        eventDataVec.emplace_back(responder, responseId, partitionId, 1);

        dispatchEventToPartition(
            fs,
            PartitionFSM::Event::e_FAIL_PRIMARY_STATE_RSPN,
            eventDataVec);
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(context->response().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(context->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isPartitionMessageValue());
    BSLS_ASSERT_SAFE(context->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isPrimaryStateResponseValue());

    const bmqp_ctrlmsg::PrimaryStateResponse& response =
        context->response()
            .choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .primaryStateResponse();
    BSLS_ASSERT_SAFE(response.partitionId() == partitionId);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": received PrimaryStateResponse " << context->response()
                  << " from " << responder->nodeDescription();

    EventData eventDataVec;
    eventDataVec.emplace_back(responder,
                              responseId,
                              partitionId,
                              1,
                              response.sequenceNumber());

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_PRIMARY_STATE_RSPN,
                             eventDataVec);
}

void StorageManager::processPrimaryStateResponse(
    const RequestManagerType::RequestSp& context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by *any* thread
    // dispatch to the CLUSTER DISPATCHER
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &StorageManager::processPrimaryStateResponseDispatched,
            this,
            context,
            responder),
        d_cluster_p);
}

void StorageManager::processReplicaStateResponseDispatched(
    const RequestContextSp& requestContext)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));

    const NodeResponsePairs& pairs = requestContext->response();
    if (d_clusterData_p->cluster().isLocal()) {
        BSLS_ASSERT_SAFE(pairs.empty());
        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(!pairs.empty());

    // Fetch partitionId from request
    const int requestPartitionId = requestContext->request()
                                       .choice()
                                       .clusterMessage()
                                       .choice()
                                       .partitionMessage()
                                       .choice()
                                       .replicaStateRequest()
                                       .partitionId();

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << requestPartitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaStateResponse(s).";
        return;  // RETURN
    }

    EventData eventDataVec(d_allocator_p);
    EventData failedEventDataVec(d_allocator_p);

    for (NodeResponsePairsCIter cit = pairs.cbegin(); cit != pairs.cend();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->first);

        if (cit->second.choice().isStatusValue()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << " Partition [" << requestPartitionId << "]: "
                          << "Received failed ReplicaStateResponse "
                          << cit->second.choice().status() << " from "
                          << cit->first->nodeDescription()
                          << ". Skipping this node's response.";
            failedEventDataVec.emplace_back(
                cit->first,
                cit->second.rId().isNull() ? -1 : cit->second.rId().value(),
                requestPartitionId,
                1);
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(cit->second.choice().isClusterMessageValue());
        BSLS_ASSERT_SAFE(cit->second.choice()
                             .clusterMessage()
                             .choice()
                             .isPartitionMessageValue());
        BSLS_ASSERT_SAFE(cit->second.choice()
                             .clusterMessage()
                             .choice()
                             .partitionMessage()
                             .choice()
                             .isReplicaStateResponseValue());

        const int responseId = cit->second.rId().isNull()
                                   ? -1
                                   : cit->second.rId().value();

        const bmqp_ctrlmsg::ReplicaStateResponse& response =
            cit->second.choice()
                .clusterMessage()
                .choice()
                .partitionMessage()
                .choice()
                .replicaStateResponse();

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << requestPartitionId << "]: "
                      << "Received ReplicaStateResponse " << cit->second
                      << " from " << cit->first->nodeDescription();

        BSLS_ASSERT_SAFE(d_clusterState.partitionsInfo()
                             .at(response.partitionId())
                             .primaryNodeId() ==
                         d_clusterData_p->membership().selfNode()->nodeId());

        const unsigned int primaryLeaseId = d_clusterState.partitionsInfo()
                                                .at(response.partitionId())
                                                .primaryLeaseId();
        eventDataVec.emplace_back(cit->first,
                                  responseId,
                                  response.partitionId(),
                                  1,
                                  d_clusterData_p->membership().selfNode(),
                                  primaryLeaseId,
                                  response.sequenceNumber());

        BSLS_ASSERT_SAFE(requestPartitionId == response.partitionId());
    }

    mqbs::FileStore* fs = d_fileStores[requestPartitionId].get();
    if (eventDataVec.size() > 0) {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_REPLICA_STATE_RSPN,
                                 eventDataVec);
    }

    if (failedEventDataVec.size() > 0) {
        dispatchEventToPartition(
            fs,
            PartitionFSM::Event::e_FAIL_REPLICA_STATE_RSPN,
            failedEventDataVec);
    }
}

void StorageManager::processReplicaStateResponse(
    const RequestContextSp& requestContext)
{
    // executed by *any* thread
    // dispatch to the CLUSTER DISPATCHER
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &StorageManager::processReplicaStateResponseDispatched,
            this,
            requestContext),
        d_cluster_p);
}

void StorageManager::processReplicaDataResponseDispatched(
    const RequestManagerType::RequestSp& context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster().isLocal());

    BSLS_ASSERT_SAFE(context->response().rId() != NULL);
    if (context->request().rId().isNull()) {
        BSLS_ASSERT_SAFE(context->response().rId().isNull());
    }
    else {
        BSLS_ASSERT_SAFE(context->request().rId().value() ==
                         context->response().rId().value());
    }

    const int responseId = context->response().rId().isNull()
                               ? -1
                               : context->response().rId().value();

    const bmqp_ctrlmsg::ReplicaDataRequest& request =
        context->request()
            .choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataRequest();
    const int partitionId = request.partitionId();
    const bmqp_ctrlmsg::ReplicaDataType::Value& dataType =
        request.replicaDataType();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaDataResponse.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (context->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received FAIL_ReplicaDataResponse event "
                      << context->response() << " from replica "
                      << responder->nodeDescription();

        EventData eventDataVec;
        eventDataVec.emplace_back(responder, responseId, partitionId, 1);

        switch (dataType) {
        case bmqp_ctrlmsg::ReplicaDataType::E_UNKNOWN: {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << ": FAIL_ReplicaDataResponse has an unknown data "
                           << "type, ignoring.";
        } break;
        case bmqp_ctrlmsg::ReplicaDataType::E_PULL: {
            dispatchEventToPartition(
                fs,
                PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_PULL,
                eventDataVec);
        } break;
        case bmqp_ctrlmsg::ReplicaDataType::E_PUSH: {
            dispatchEventToPartition(
                fs,
                PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_PUSH,
                eventDataVec);
        } break;
        case bmqp_ctrlmsg::ReplicaDataType::E_DROP: {
            dispatchEventToPartition(
                fs,
                PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_DROP,
                eventDataVec);
        } break;
        }
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(context->response().choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(context->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .isPartitionMessageValue());
    BSLS_ASSERT_SAFE(context->response()
                         .choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaDataResponseValue());

    const bmqp_ctrlmsg::ReplicaDataResponse& response =
        context->response()
            .choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataResponse();
    BSLS_ASSERT_SAFE(response.partitionId() == partitionId);
    BSLS_ASSERT_SAFE(response.beginSequenceNumber() ==
                     request.beginSequenceNumber());
    BSLS_ASSERT_SAFE(response.endSequenceNumber() ==
                     request.endSequenceNumber());

    int incrementCount = 0;
    if (dataType == bmqp_ctrlmsg::ReplicaDataType::E_PULL) {
        // Increment for both self and the replica node who healed self.
        incrementCount = 2;
    }
    else if (dataType == bmqp_ctrlmsg::ReplicaDataType::E_PUSH) {
        incrementCount = 1;
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(
        responder,
        responseId,
        partitionId,
        incrementCount,
        PartitionSeqNumDataRange(response.beginSequenceNumber(),
                                 response.endSequenceNumber()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId
                  << "]: received ReplicaDataResponse " << context->response()
                  << " of type [" << dataType << "] from "
                  << responder->nodeDescription();

    switch (dataType) {
    case bmqp_ctrlmsg::ReplicaDataType::E_UNKNOWN: {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId
                       << "]: ReplicaDataResponse has an unknown data type, "
                       << "ignoring.";
    } break;
    case bmqp_ctrlmsg::ReplicaDataType::E_PULL: {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_REPLICA_DATA_RSPN_PULL,
                                 eventDataVec);
    } break;
    case bmqp_ctrlmsg::ReplicaDataType::E_PUSH: {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_REPLICA_DATA_RSPN_PUSH,
                                 eventDataVec);
    } break;
    case bmqp_ctrlmsg::ReplicaDataType::E_DROP: {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_REPLICA_DATA_RSPN_DROP,
                                 eventDataVec);
    } break;
    }
}

void StorageManager::processReplicaDataResponse(
    const RequestManagerType::RequestSp& context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by *any* thread

    // dispatch to the CLUSTER DISPATCHER
    d_dispatcher_p->execute(
        bdlf::BindUtil::bind(
            &StorageManager::processReplicaDataResponseDispatched,
            this,
            context,
            responder),
        d_cluster_p);
}

void StorageManager::bufferPrimaryStatusAdvisoryDispatched(
    const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    mqbnet::ClusterNode*                       source)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    const int pid = advisory.partitionId();
    BSLS_ASSERT_SAFE(0 <= pid && pid < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[pid]->inDispatcherThread());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << pid
                  << "]: Buffering primary status advisory: " << advisory;

    d_bufferedPrimaryStatusAdvisoryInfosVec.at(pid).push_back(
        bsl::make_pair(advisory, source));
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

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (!d_partitionFSMVec[partitionId]->isSelfHealed()) {
        EventData eventDataVec;
        eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                                  -1,  // placeholder requestId
                                  partitionId,
                                  1);

        bsl::shared_ptr<bsl::queue<PartitionFSM::EventWithData> > queueSp =
            bsl::allocate_shared<bsl::queue<PartitionFSM::EventWithData> >(
                d_allocator_p);
        queueSp->emplace(PartitionFSM::Event::e_RST_UNKNOWN, eventDataVec);

        d_partitionFSMVec[partitionId]->applyEvent(queueSp);
    }

    StorageUtil::processShutdownEventDispatched(
        d_clusterData_p,
        &d_partitionInfoVec[partitionId],
        fs,
        partitionId);
}

void StorageManager::forceFlushFileStores()
{
    // executed by event scheduler's dispatcher thread

    StorageUtil::forceFlushFileStores(&d_fileStores);
}

void StorageManager::do_startWatchDog(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    if (static_cast<const bdlmt::EventSchedulerEventHandle::Event*>(
            d_watchDogEventHandles[partitionId]) != 0) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not starting watchdog since it has already been "
                      << "started.";
        return;  // RETURN
    }

    d_clusterData_p->scheduler().scheduleEvent(
        &d_watchDogEventHandles[partitionId],
        d_clusterData_p->scheduler().now() + d_watchDogTimeoutInterval,
        bdlf::BindUtil::bind(&StorageManager::onWatchDog, this, partitionId));
}

void StorageManager::do_stopWatchDog(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    const int rc = d_clusterData_p->scheduler().cancelEvent(
        d_watchDogEventHandles[partitionId]);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Failed to cancel WatchDog, rc: " << rc;
    }

    d_watchDogEventHandles[partitionId].release();
}

void StorageManager::do_openRecoveryFileSet(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->config().partitionId() == partitionId);
    if (fs->isOpen()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not opening recovery file set because FileStore is "
                      << "already open";

        return;  // RETURN
    }

    bmqu::MemOutStream errorDesc;
    int rc = d_recoveryManager_mp->openRecoveryFileSet(errorDesc, partitionId);
    if (rc == 1) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "No recovery file set is found.  Create new set.";

        // Creating file set will populate the file headers.  We need to do
        // this before receiving data chunks from the recovery peer.
        rc = d_recoveryManager_mp->createRecoveryFileSet(errorDesc,
                                                         fs,
                                                         partitionId);
        if (rc != 0) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId << "]: "
                           << "Error while creating recovery file set, rc: "
                           << rc << ", error: " << errorDesc.str();
        }

        return;  // RETURN
    }

    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Error while recovering file set from storage, rc: "
                       << rc << ", error: " << errorDesc.str();
        return;  // RETURN
    }
}

void StorageManager::do_closeRecoveryFileSet(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    const int rc = d_recoveryManager_mp->closeRecoveryFileSet(partitionId);
    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Failure while closing recovery file set"
                       << ", rc: " << rc;

        return;  // RETURN
    }
}

void StorageManager::do_storeSelfSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData&    eventData   = eventDataVec[0];
    const int                       partitionId = eventData.partitionId();
    const PartitionSeqNumDataRange& dataRange =
        eventData.partitionSeqNumDataRange();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(dataRange.first <= dataRange.second);

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();
    NodeSeqNumContext&   nodeSeqNumCtx =
        d_nodeToSeqNumCtxMapVec[partitionId][selfNode];
    if (dataRange.second > bmqp_ctrlmsg::PartitionSequenceNumber()) {
        nodeSeqNumCtx.first = dataRange.second;
    }
    else {
        mqbs::FileStore* fs = d_fileStores[partitionId].get();
        BSLS_ASSERT_SAFE(fs);
        if (fs->isOpen()) {
            nodeSeqNumCtx.first.primaryLeaseId() = fs->primaryLeaseId();
            nodeSeqNumCtx.first.sequenceNumber() = fs->sequenceNumber();
        }
        else {
            d_recoveryManager_mp->recoverSeqNum(&nodeSeqNumCtx.first,
                                                partitionId);
        }
    }
    nodeSeqNumCtx.second = false;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": In Partition [" << partitionId << "]'s FSM, "
                  << "storing self sequence number as "
                  << d_nodeToSeqNumCtxMapVec[partitionId][selfNode].first;
}

void StorageManager::do_storePrimarySeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();
    const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum =
        eventData.partitionSequenceNumber();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BSLA_MAYBE_UNUSED const PartitionInfo& partitionInfo =
        d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(partitionInfo.primary() == eventData.source());
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfReplica());

    // Information from 'ReplicaStateRequest' or 'PrimaryStateResponse' could
    // be stale; ignore if so.
    bool                   hasNew = false;
    NodeToSeqNumCtxMapIter it     = d_nodeToSeqNumCtxMapVec[partitionId].find(
        eventData.source());
    if (it == d_nodeToSeqNumCtxMapVec[partitionId].end()) {
        d_nodeToSeqNumCtxMapVec[partitionId].insert(
            bsl::make_pair(eventData.source(), bsl::make_pair(seqNum, false)));
        hasNew = true;
    }
    else if (seqNum > it->second.first) {
        it->second.first  = seqNum;
        it->second.second = false;
        hasNew            = true;
    }

    if (hasNew) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": In Partition [" << partitionId << "]'s FSM, "
                      << "storing the sequence number of "
                      << eventData.source()->nodeDescription() << " as "
                      << seqNum;
    }
}

void StorageManager::do_storeReplicaSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const PartitionFSM::Event::Enum event        = eventWithData.first;
    const EventData&                eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    for (EventDataCIter cit = eventDataVec.cbegin();
         cit != eventDataVec.cend();
         cit++) {
        int partitionId = cit->partitionId();
        const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum =
            cit->partitionSequenceNumber();

        BSLS_ASSERT_SAFE(0 <= partitionId &&
                         partitionId < static_cast<int>(d_fileStores.size()));
        BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());
        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary()->nodeId() ==
                         d_clusterData_p->membership().selfNode()->nodeId());

        // Information from 'PrimaryStateRequest' or 'ReplicaStateResponse'
        // could be stale; ignore if so.
        bool                   hasNew = false;
        NodeToSeqNumCtxMapIter it = d_nodeToSeqNumCtxMapVec[partitionId].find(
            cit->source());
        if (it == d_nodeToSeqNumCtxMapVec[partitionId].end()) {
            d_nodeToSeqNumCtxMapVec[partitionId].insert(
                bsl::make_pair(cit->source(), bsl::make_pair(seqNum, false)));
            hasNew = true;
        }
        else if (seqNum > it->second.first ||
                 event == PartitionFSM::Event::e_PRIMARY_STATE_RQST) {
            it->second.first  = seqNum;
            it->second.second = false;
            hasNew            = true;
        }

        if (hasNew) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << ": In Partition [" << partitionId << "]'s FSM, "
                          << "storing the sequence number of "
                          << cit->source()->nodeDescription() << " as "
                          << seqNum;
        }
    }
}

void StorageManager::do_storePartitionInfo(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData      = eventDataVec[0];
    const int                    partitionId    = eventData.partitionId();
    mqbnet::ClusterNode*         primaryNode    = eventData.primary();
    unsigned int                 primaryLeaseId = eventData.primaryLeaseId();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    if (pinfo.primary() &&
        (pinfo.primary()->nodeId() == primaryNode->nodeId())) {
        // Primary node did not change
        pinfo.setPrimaryLeaseId(primaryLeaseId);
        return;  // RETURN
    }

    pinfo.setPrimary(primaryNode);
    pinfo.setPrimaryLeaseId(primaryLeaseId);
    pinfo.setPrimaryStatus(bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);

    if (d_partitionFSMVec[partitionId]->isSelfReplica()) {
        d_recoveryManager_mp->setLiveDataSource(primaryNode, partitionId);
    }
}

void StorageManager::do_clearPartitionInfo(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         primaryNode = eventData.primary();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbs::FileStore* fs    = d_fileStores[partitionId].get();
    PartitionInfo&   pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());

    StorageUtil::clearPrimaryForPartition(
        fs,
        &pinfo,
        d_clusterData_p->identity().description(),
        partitionId,
        primaryNode);
}

void StorageManager::do_replicaStateRequest(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    bsl::vector<mqbnet::ClusterNode*> replicas;

    ClusterUtil::loadPeerNodes(&replicas, *d_clusterData_p);

    RequestContextSp contextSp =
        d_clusterData_p->multiRequestManager().createRequestContext();
    bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
        contextSp->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaStateRequest();

    replicaStateRequest.partitionId() = partitionId;

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();
    BSLS_ASSERT_SAFE(d_nodeToSeqNumCtxMapVec[partitionId].find(selfNode) !=
                     d_nodeToSeqNumCtxMapVec[partitionId].end());

    replicaStateRequest.sequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId][selfNode].first;

    contextSp->setDestinationNodes(replicas);
    contextSp->setResponseCb(
        bdlf::BindUtil::bind(&StorageManager::processReplicaStateResponse,
                             this,
                             bdlf::PlaceHolders::_1));

    d_clusterData_p->multiRequestManager().sendRequest(contextSp,
                                                       bsls::TimeInterval(10));
}

void StorageManager::do_replicaStateResponse(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::PartitionMessage& partitionMessage =
        clusterMsg.choice().makePartitionMessage();
    bmqp_ctrlmsg::ReplicaStateResponse& response =
        partitionMessage.choice().makeReplicaStateResponse();

    response.partitionId() = partitionId;
    response.sequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId]
                               [d_clusterData_p->membership().selfNode()]
                                   .first;

    BSLS_ASSERT_SAFE(eventData.source() ==
                     d_partitionInfoVec[partitionId].primary());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to ReplicaStateRequest from primary node "
                  << eventData.source()->nodeDescription();

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               eventData.source()),
                          d_cluster_p);
}

void StorageManager::do_failureReplicaStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BSLS_ASSERT_SAFE(eventData.source() !=
                     d_partitionInfoVec[partitionId].primary());
    BSLS_ASSERT_SAFE(!d_partitionFSMVec[partitionId]->isSelfReplica());
    // Replicas should never send an explicit failure ReplicaStateResponse.

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_REPLICA;
    response.message()             = "Not a replica";

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Self not replica! Sent failure response " << controlMsg
                  << " to ReplicaStateRequest from node claiming to be primary"
                  << eventData.source()->nodeDescription() << ".";

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               eventData.source()),
                          d_cluster_p);
}

void StorageManager::do_logFailureReplicaStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    for (EventDataCIter cit = eventDataVec.cbegin();
         cit != eventDataVec.cend();
         cit++) {
        int                        partitionId = cit->partitionId();
        const mqbnet::ClusterNode* sourceNode  = cit->source();

        BSLS_ASSERT_SAFE(0 <= partitionId &&
                         partitionId < static_cast<int>(d_fileStores.size()));

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Received failure ReplicaStateResponse from node "
                      << sourceNode->nodeDescription() << ".";
    }
}

void StorageManager::do_logFailurePrimaryStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    int                        partitionId = eventDataVec[0].partitionId();
    const mqbnet::ClusterNode* sourceNode  = eventDataVec[0].source();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() == sourceNode);

    BALL_LOG_WARN << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received failure PrimaryStateResponse from node "
                  << sourceNode->nodeDescription() << ".";
}

void StorageManager::do_primaryStateRequest(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    RequestManagerType::RequestSp request =
        d_clusterData_p->requestManager().createRequest();

    bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makePrimaryStateRequest();

    primaryStateRequest.partitionId() = partitionId;
    primaryStateRequest.sequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId]
                               [d_clusterData_p->membership().selfNode()]
                                   .first;

    mqbnet::ClusterNode* destNode = eventData.primary();

    BSLS_ASSERT_SAFE(destNode);

    request->setResponseCb(
        bdlf::BindUtil::bind(&StorageManager::processPrimaryStateResponse,
                             this,
                             bdlf::PlaceHolders::_1,
                             destNode));

    bmqt::GenericResult::Enum status = d_clusterData_p->cluster().sendRequest(
        request,
        destNode,
        bsls::TimeInterval(10));

    if (bmqt::GenericResult::e_SUCCESS != status) {
        EventData failedEventDataVec;
        failedEventDataVec.emplace_back(destNode,
                                        -1,  // placeholder responseId
                                        partitionId,
                                        1);

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_PRIMARY_STATE_RSPN,
            failedEventDataVec);
    }
}

void StorageManager::do_primaryStateResponse(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::PartitionMessage& partitionMessage =
        clusterMsg.choice().makePartitionMessage();
    bmqp_ctrlmsg::PrimaryStateResponse& response =
        partitionMessage.choice().makePrimaryStateResponse();

    response.partitionId() = partitionId;
    response.sequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId]
                               [d_clusterData_p->membership().selfNode()]
                                   .first;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to PrimaryStateRequest from replica node "
                  << eventData.source()->nodeDescription();

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               eventData.source()),
                          d_cluster_p);
}

void StorageManager::do_failurePrimaryStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode()->nodeId() !=
                     d_partitionInfoVec[partitionId].primary()->nodeId());
    BSLS_ASSERT_SAFE(!d_partitionFSMVec[partitionId]->isSelfPrimary());
    // Primary should never send an explicit failure PrimaryStateResponse.

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_PRIMARY;
    response.message()             = "Not a primary";

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Self not primary! Sent failure response " << controlMsg
                  << " to PrimaryStateRequest from node who thinks otherwise "
                  << eventData.source()->nodeDescription() << ".";

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               eventData.source()),
                          d_cluster_p);
}

void StorageManager::do_replicaDataRequestPush(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec.at(partitionId)->isSelfPrimary());

    if (d_recoveryManager_mp->expectedDataChunks(partitionId)) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not sending ReplicaDataRequestPush to replicas "
                      << "because self is still expecting recovery data "
                      << "chunks.";

        return;  // RETURN
    }

    // Self primary is sending request to all outdated and up-to-date replicas

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();

    NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        d_nodeToSeqNumCtxMapVec[partitionId];
    BSLS_ASSERT_SAFE(nodeToSeqNumCtxMap.find(selfNode) !=
                     nodeToSeqNumCtxMap.end());
    const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum =
        nodeToSeqNumCtxMap.at(selfNode).first;

    // Determine the outdated and up-to-date replicas
    ClusterNodeVec outdatedReplicas;
    for (NodeToSeqNumCtxMapCIter cit = nodeToSeqNumCtxMap.cbegin();
         cit != nodeToSeqNumCtxMap.cend();
         cit++) {
        if (cit->first->nodeId() == selfNode->nodeId()) {
            continue;  // CONTINUE
        }
        if (cit->second.first <= selfSeqNum && !cit->second.second) {
            outdatedReplicas.emplace_back(cit->first);
        }
    }

    // Send ReplicaDataRequestPush to outdated and up-to-date replicas.  It is
    // important to inform up-to-date replicas such that they know they can
    // transition to healed replica.
    EventData failedEventDataVec;
    for (ClusterNodeVecCIter cit = outdatedReplicas.cbegin();
         cit != outdatedReplicas.cend();
         ++cit) {
        mqbnet::ClusterNode* destNode = *cit;
        BSLS_ASSERT_SAFE(destNode->nodeId() != selfNode->nodeId());

        RequestManagerType::RequestSp request =
            d_clusterData_p->requestManager().createRequest();
        bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRqst =
            request->request()
                .choice()
                .makeClusterMessage()
                .choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaDataRequest();

        replicaDataRqst.replicaDataType() =
            bmqp_ctrlmsg::ReplicaDataType::E_PUSH;
        replicaDataRqst.partitionId() = partitionId;
        replicaDataRqst.beginSequenceNumber() =
            nodeToSeqNumCtxMap.at(destNode).first;
        replicaDataRqst.endSequenceNumber() = selfSeqNum;

        request->setResponseCb(
            bdlf::BindUtil::bind(&StorageManager::processReplicaDataResponse,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 destNode));

        const bmqt::GenericResult::Enum status =
            d_clusterData_p->cluster().sendRequest(request,
                                                   destNode,
                                                   bsls::TimeInterval(10));

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Sent ReplicaDataRequestPush: " << replicaDataRqst
                      << " to " << destNode->nodeDescription() << ".";

        if (bmqt::GenericResult::e_SUCCESS != status) {
            failedEventDataVec.emplace_back(destNode,
                                            -1,  // placeholder responseId
                                            partitionId,
                                            1);
        }
    }

    if (!failedEventDataVec.empty()) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_PUSH,
            failedEventDataVec);
    }
}

void StorageManager::do_replicaDataResponsePush(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         destNode    = eventData.source();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfReplica());

    BSLS_ASSERT_SAFE(destNode);
    BSLS_ASSERT_SAFE(destNode == d_partitionInfoVec[partitionId].primary());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    if (eventData.requestId() >= 0) {
        // Responding immediately to a ReplicaDataRequestPush

        controlMsg.rId() = eventData.requestId();

        bmqp_ctrlmsg::ReplicaDataResponse& response =
            controlMsg.choice()
                .makeClusterMessage()
                .choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaDataResponse();

        response.replicaDataType() = bmqp_ctrlmsg::ReplicaDataType::E_PUSH;
        response.partitionId()     = partitionId;
        response.beginSequenceNumber() =
            eventData.partitionSeqNumDataRange().first;
        response.endSequenceNumber() =
            eventData.partitionSeqNumDataRange().second;
    }
    else {
        // Responding after receiving all data chunks

        d_recoveryManager_mp->loadReplicaDataResponsePush(&controlMsg,
                                                          partitionId);
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Sent response " << controlMsg
                  << " to ReplicaDataRequestPush from primary node "
                  << destNode->nodeDescription() << ".";

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               destNode),
                          d_cluster_p);
}

void StorageManager::do_replicaDataRequestDrop(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec.at(partitionId)->isSelfPrimary());

    if (d_recoveryManager_mp->expectedDataChunks(partitionId)) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not sending ReplicaDataRequestDrop to replicas "
                      << "because self is still expecting recovery data "
                      << "chunks.";

        return;  // RETURN
    }

    mqbnet::ClusterNode* const selfNode =
        d_clusterData_p->membership().selfNode();

    NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        d_nodeToSeqNumCtxMapVec[partitionId];
    BSLS_ASSERT_SAFE(nodeToSeqNumCtxMap.find(selfNode) !=
                     nodeToSeqNumCtxMap.end());
    const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum =
        nodeToSeqNumCtxMap.at(selfNode).first;

    // Determine the replicas with obsolete data to be dropped
    ClusterNodeVec obsoleteDataReplicas;
    for (NodeToSeqNumCtxMapCIter cit = nodeToSeqNumCtxMap.cbegin();
         cit != nodeToSeqNumCtxMap.cend();
         cit++) {
        if (cit->second.first > selfSeqNum && !cit->second.second) {
            obsoleteDataReplicas.emplace_back(cit->first);
        }
    }

    // Send ReplicaDataRequestDrop to replicas with obsolete data
    EventData failedEventDataVec;
    for (ClusterNodeVecCIter cit = obsoleteDataReplicas.cbegin();
         cit != obsoleteDataReplicas.cend();
         ++cit) {
        mqbnet::ClusterNode* destNode = *cit;
        BSLS_ASSERT_SAFE(destNode->nodeId() != selfNode->nodeId());

        RequestManagerType::RequestSp request =
            d_clusterData_p->requestManager().createRequest();
        bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRqst =
            request->request()
                .choice()
                .makeClusterMessage()
                .choice()
                .makePartitionMessage()
                .choice()
                .makeReplicaDataRequest();

        replicaDataRqst.replicaDataType() =
            bmqp_ctrlmsg::ReplicaDataType::E_DROP;
        replicaDataRqst.partitionId() = partitionId;

        request->setResponseCb(
            bdlf::BindUtil::bind(&StorageManager::processReplicaDataResponse,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 destNode));

        const bmqt::GenericResult::Enum status =
            d_clusterData_p->cluster().sendRequest(request,
                                                   destNode,
                                                   bsls::TimeInterval(10));

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Sent ReplicaDataRequestDrop: " << replicaDataRqst
                      << " to " << destNode->nodeDescription() << ".";

        if (bmqt::GenericResult::e_SUCCESS != status) {
            failedEventDataVec.emplace_back(destNode,
                                            -1,  // placeholder responseId
                                            partitionId,
                                            1);
        }
        else {
            nodeToSeqNumCtxMap.at(destNode).second = true;
        }
    }

    if (!failedEventDataVec.empty()) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_DROP,
            failedEventDataVec);
    }
}

void StorageManager::do_replicaDataRequestPull(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbnet::ClusterNode* destNode = eventData.highestSeqNumNode();

    BSLS_ASSERT_SAFE(destNode);
    BSLS_ASSERT_SAFE(destNode->nodeId() !=
                     d_clusterData_p->membership().selfNode()->nodeId());

    RequestManagerType::RequestSp request =
        d_clusterData_p->requestManager().createRequest();

    bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        request->request()
            .choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaDataRequest();

    replicaDataRequest.replicaDataType() =
        bmqp_ctrlmsg::ReplicaDataType::E_PULL;
    replicaDataRequest.partitionId() = partitionId;
    replicaDataRequest.beginSequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId]
                               [d_clusterData_p->membership().selfNode()]
                                   .first;
    replicaDataRequest.endSequenceNumber() =
        d_nodeToSeqNumCtxMapVec[partitionId][destNode].first;

    request->setResponseCb(
        bdlf::BindUtil::bind(&StorageManager::processReplicaDataResponse,
                             this,
                             bdlf::PlaceHolders::_1,
                             destNode));

    bmqt::GenericResult::Enum status = d_clusterData_p->cluster().sendRequest(
        request,
        destNode,
        bsls::TimeInterval(10));

    if (bmqt::GenericResult::e_SUCCESS != status) {
        EventData failedEventDataVec;
        failedEventDataVec.emplace_back(destNode,
                                        -1,  // placeholder responseId
                                        partitionId,
                                        1);

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_PULL,
            failedEventDataVec);
    }
}

void StorageManager::do_replicaDataResponsePull(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbnet::ClusterNode* destNode = eventData.source();

    BSLS_ASSERT_SAFE(destNode);
    BSLS_ASSERT_SAFE(destNode == d_partitionInfoVec[partitionId].primary());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::ReplicaDataResponse& response =
        controlMsg.choice()
            .makeClusterMessage()
            .choice()
            .makePartitionMessage()
            .choice()
            .makeReplicaDataResponse();

    response.replicaDataType() = bmqp_ctrlmsg::ReplicaDataType::E_PULL;
    response.partitionId()     = partitionId;
    response.beginSequenceNumber() =
        eventData.partitionSeqNumDataRange().first;
    response.endSequenceNumber() = eventData.partitionSeqNumDataRange().second;

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << ": Sent response " << controlMsg
                  << " to ReplicaDataRequestPull from primary node "
                  << destNode->nodeDescription() << ".";

    dispatcher()->execute(bdlf::BindUtil::bind(&StorageManager::sendMessage,
                                               this,
                                               controlMsg,
                                               eventData.source()),
                          d_cluster_p);
}

void StorageManager::do_failureReplicaDataResponsePull(
    const BSLS_ANNOTATION_UNUSED PartitionFSMArgsSp& args)
{
    // TODO: Complete Impl
}

void StorageManager::do_failureReplicaDataResponsePush(
    const BSLS_ANNOTATION_UNUSED PartitionFSMArgsSp& args)
{
    // TODO: Complete Impl
}

void StorageManager::do_bufferLiveData(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         source      = eventData.source();

    bmqp::Event rawEvent(eventData.storageEvent().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isStorageEvent());

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(pinfo.primary() == source);

    bool skipAlarm = partitionHealthState(partitionId) ==
                     PartitionFSM::State::e_UNKNOWN;
    if (!StorageUtil::validateStorageEvent(
            rawEvent,
            partitionId,
            source,
            pinfo.primary(),
            pinfo.primaryStatus(),
            d_clusterData_p->identity().description(),
            skipAlarm,
            true)) {  // isFSMWorkflow
        return;       // RETURN
    }

    d_recoveryManager_mp->bufferStorageEvent(partitionId,
                                             eventData.storageEvent(),
                                             source);
}

void StorageManager::do_processBufferedLiveData(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode* primary = d_partitionInfoVec[partitionId].primary();

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "buffered live data.";
        return;  // RETURN
    }

    bsl::vector<bsl::shared_ptr<bdlbb::Blob> > bufferedStorageEvents;
    int rc = d_recoveryManager_mp->loadBufferedStorageEvents(
        &bufferedStorageEvents,
        primary,
        partitionId);
    if (rc != 0) {
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Opened successfully, applying "
                  << bufferedStorageEvents.size()
                  << " buffered storage events to the partition.";

    mqbs::FileStore* fs = d_fileStores[static_cast<size_t>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs && fs->isOpen());
    for (bsl::vector<bsl::shared_ptr<bdlbb::Blob> >::const_iterator cit =
             bufferedStorageEvents.cbegin();
         cit != bufferedStorageEvents.cend();
         ++cit) {
        rc = fs->processRecoveryEvent(*cit);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description() << " Partition ["
                << partitionId << "]: "
                << "Failed to apply buffered storage event, rc: " << rc
                << ". Closing the partition." << BMQTSK_ALARMLOG_END;
            fs->close(d_clusterConfig.partitionConfig().flushAtShutdown());

            EventData eventDataVecLocal;
            eventDataVecLocal.emplace_back(primary,
                                           -1,  // placeholder requestId
                                           partitionId,
                                           1);

            args->eventsQueue()->emplace(
                PartitionFSM::Event::e_ISSUE_LIVESTREAM,
                eventDataVecLocal);
            break;  // BREAK
        }
    }
}

void StorageManager::do_processBufferedPrimaryStatusAdvisories(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "primary status advisory.";
        return;  // RETURN
    }

    BALL_LOG_INFO
        << d_clusterData_p->identity().description() << " Partition ["
        << partitionId << "]: " << "Processing "
        << d_bufferedPrimaryStatusAdvisoryInfosVec[partitionId].size()
        << " buffered primary status advisory.";

    for (PrimaryStatusAdvisoryInfosCIter cit =
             d_bufferedPrimaryStatusAdvisoryInfosVec[partitionId].cbegin();
         cit != d_bufferedPrimaryStatusAdvisoryInfosVec[partitionId].cend();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->first.partitionId() == partitionId);

        PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
        if (cit->second->nodeId() != pinfo.primary()->nodeId() ||
            cit->first.primaryLeaseId() != pinfo.primaryLeaseId()) {
            BALL_LOG_INFO << d_clusterData_p->identity().description()
                          << " Partition [" << partitionId
                          << "]: " << "Ignoring primary status advisory "
                          << cit->first
                          << " because primary node or leaseId is invalid. "
                          << "Self-perceived [prmary, leaseId] is: ["
                          << pinfo.primary()->nodeDescription() << ","
                          << pinfo.primaryLeaseId() << "]";
            continue;  // CONTINUE
        }
        pinfo.setPrimaryStatus(cit->first.status());
        if (bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE == cit->first.status()) {
            d_fileStores[partitionId]->setActivePrimary(
                pinfo.primary(),
                pinfo.primaryLeaseId());

            // Note: We don't check if all partitions are fully healed and have
            // an active active primary here, because we will do the check soon
            // after when we transition the replica to healed (see
            // `onPartitionRecovery()` method).
        }
    }

    d_bufferedPrimaryStatusAdvisoryInfosVec[partitionId].clear();
}

void StorageManager::do_processLiveData(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         source      = eventData.source();

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "live data.";
        return;  // RETURN
    }

    bmqp::Event rawEvent(eventData.storageEvent().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isStorageEvent());

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(pinfo.primary() == source);

    bool skipAlarm = partitionHealthState(partitionId) ==
                     PartitionFSM::State::e_UNKNOWN;
    if (!StorageUtil::validateStorageEvent(
            rawEvent,
            partitionId,
            source,
            pinfo.primary(),
            pinfo.primaryStatus(),
            d_clusterData_p->identity().description(),
            skipAlarm,
            true)) {  // isFSMWorkflow
        return;       // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[static_cast<size_t>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs && fs->isOpen());

    fs->processStorageEvent(eventData.storageEvent(),
                            false /* isPartitionSyncEvent */,
                            source);
}

void StorageManager::do_processPut(
    const BSLS_ANNOTATION_UNUSED PartitionFSMArgsSp& args)
{
    // TODO: Complete Impl
}

void StorageManager::do_nackPut(
    const BSLS_ANNOTATION_UNUSED PartitionFSMArgsSp& args)
{
    // TODO: Complete Impl
}

void StorageManager::do_cleanupSeqnums(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    d_nodeToSeqNumCtxMapVec[partitionId].clear();
}

void StorageManager::do_startSendDataChunks(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    if (d_partitionFSMVec.at(partitionId)->isSelfPrimary() &&
        d_recoveryManager_mp->expectedDataChunks(partitionId)) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not sending data chunks to replicas because self is "
                      << "still expecting recovery data chunks.";

        return;  // RETURN
    }

    // Note that 'eventData.partitionSeqNumDataRange()' is only used when this
    // action is performed by the replica.  If self is primary, we use
    // `d_nodeToSeqNumCtxMapVec` to determine data range for each replica
    // instead.
    bsl::function<void(int, mqbnet::ClusterNode*, int)> f =
        bdlf::BindUtil::bind(&StorageManager::onPartitionDoneSendDataChunksCb,
                             this,
                             bdlf::PlaceHolders::_1,  // partitionId
                             eventData.requestId(),
                             eventData.partitionSeqNumDataRange(),
                             bdlf::PlaceHolders::_2,   // source
                             bdlf::PlaceHolders::_3);  // status

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();
    if (eventWithData.first == PartitionFSM::Event::e_REPLICA_DATA_RQST_PULL) {
        // Self Replica is sending data to Destination Primary.

        mqbnet::ClusterNode* destNode = eventData.source();
        BSLS_ASSERT_SAFE(destNode->nodeId() != selfNode->nodeId());
        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                         eventData.source());

        bmqp_ctrlmsg::PartitionSequenceNumber beginSeqNum =
            eventData.partitionSeqNumDataRange().first;
        bmqp_ctrlmsg::PartitionSequenceNumber endSeqNum =
            eventData.partitionSeqNumDataRange().second;
        BSLS_ASSERT_SAFE(endSeqNum ==
                         d_nodeToSeqNumCtxMapVec[partitionId][selfNode].first);

        d_recoveryManager_mp->processSendDataChunks(
            partitionId,
            destNode,
            beginSeqNum,
            endSeqNum,
            *(d_fileStores[partitionId].get()),
            f);
    }
    else {
        // Self Primary is sending recovery data to outdated replicas.
        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary()->nodeId() ==
                         selfNode->nodeId());

        NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
            d_nodeToSeqNumCtxMapVec[partitionId];

        // End Sequence number is primary's latest sequence number.
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum =
            nodeToSeqNumCtxMap[selfNode].first;

        for (NodeToSeqNumCtxMapCIter cit = nodeToSeqNumCtxMap.cbegin();
             cit != nodeToSeqNumCtxMap.cend();
             cit++) {
            if (cit->first->nodeId() == selfNode->nodeId() ||
                cit->second.second) {
                continue;
            }
            mqbnet::ClusterNode*                         destNode = cit->first;
            const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum =
                cit->second.first;

            if (beginSeqNum > endSeqNum) {
                // Replica is ahead: we already sent ReplicaDataRequestDrop
                continue;
            }
            else if (beginSeqNum == endSeqNum) {
                // Replica in-sync with primary: no need to send data chunks
                nodeToSeqNumCtxMap.at(destNode).second = true;
                continue;
            }

            const int rc = d_recoveryManager_mp->processSendDataChunks(
                partitionId,
                destNode,
                beginSeqNum,
                endSeqNum,
                *(d_fileStores[partitionId].get()),
                f);
            if (rc != 0) {
                BALL_LOG_ERROR << d_clusterData_p->identity().description()
                               << " Partition [" << partitionId << "]: "
                               << "Failure while sending data chunks to "
                               << destNode << ", beginSeqNum = " << beginSeqNum
                               << ", endSeqNum = " << endSeqNum
                               << ", rc = " << rc;
            }
            else {
                nodeToSeqNumCtxMap.at(destNode).second = true;
            }
        }
    }
}

void StorageManager::do_setExpectedDataChunkRange(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode* highestSeqNumNode   = eventData.highestSeqNumNode();

    const mqbs::FileStore& fs = fileStore(partitionId);

    if (eventWithData.first == PartitionFSM::Event::e_REPLICA_HIGHEST_SEQ) {
        // Self Primary is expecting data from highest seq num replica.

        const bmqp_ctrlmsg::PartitionSequenceNumber selfSeqNum =
            d_nodeToSeqNumCtxMapVec.at(partitionId)
                .at(d_clusterData_p->membership().selfNode())
                .first;

        d_recoveryManager_mp->setExpectedDataChunkRange(
            partitionId,
            fs,
            highestSeqNumNode,
            selfSeqNum,
            eventData.partitionSequenceNumber());
    }
    else if (eventWithData.first ==
             PartitionFSM::Event::e_REPLICA_DATA_RQST_PUSH) {
        // Self Replica is expecting data from the primary.

        if (eventData.partitionSeqNumDataRange().first ==
            eventData.partitionSeqNumDataRange().second) {
            // Self Replica is up-to-date with the primary, thus not expecting
            // data chunks.

            EventData eventDataVecOut;
            eventDataVecOut.emplace_back(highestSeqNumNode,
                                         eventData.requestId(),
                                         partitionId,
                                         1,  // incrementCount
                                         eventData.partitionSeqNumDataRange());
            args->eventsQueue()->emplace(
                PartitionFSM::Event::e_DONE_RECEIVING_DATA_CHUNKS,
                eventDataVecOut);

            return;  // RETURN
        }

        d_recoveryManager_mp->setExpectedDataChunkRange(
            partitionId,
            fs,
            highestSeqNumNode,
            eventData.partitionSeqNumDataRange().first,
            eventData.partitionSeqNumDataRange().second,
            eventData.requestId());
    }
    else {
        bmqu::MemOutStream out;
        out << "Partition [" << partitionId << "]'s FSM "
            << "(state = '" << d_partitionFSMVec[partitionId]->state() << "')"
            << ": Unexpected event '" << eventWithData.first
            << "' caused action 'do_setExpectedDataChunkRange'.";
        BSLS_ASSERT_SAFE(false && out.str().data());
    }
}

void StorageManager::do_resetReceiveDataCtx(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    d_recoveryManager_mp->resetReceiveDataCtx(eventDataVec[0].partitionId());
}

void StorageManager::do_openStorage(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());
    BSLS_ASSERT_SAFE(d_isQueueKeyInfoMapVecInitialized);

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    BSLS_ASSERT_SAFE(!d_recoveryManager_mp->expectedDataChunks(partitionId));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(fs->config().partitionId() == partitionId);

    if (fs->isOpen()) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Not opening FileStore because it's already opened.";
        return;  // RETURN
    }

    const int rc = fs->open(d_queueKeyInfoMapVec.at(partitionId));
    if (0 != rc) {
        BMQTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description() << " Partition ["
            << partitionId << "]: "
            << "Failed to open FileStore after recovery was finished, "
            << "rc: " << rc << BMQTSK_ALARMLOG_END;
    }
}

void StorageManager::do_updateStorage(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         source      = eventData.source();

    bmqp::Event rawEvent(eventData.storageEvent().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isPartitionSyncEvent());

    // A partition-sync event is received in one of the following scenarios:
    // 1) The chosen syncing peer ('source') sends missing storage events to
    //    the newly chosen primary (self).
    // 2) A newly chosen primary ('source') sends missing storage events to
    //    replica (self).

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    if (!StorageUtil::validatePartitionSyncEvent(rawEvent,
                                                 partitionId,
                                                 source,
                                                 pinfo,
                                                 *d_clusterData_p,
                                                 true)) {  // isFSMWorkflow
        return;                                            // RETURN
    }

    if (pinfo.primary()->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        // If self is primary for this partition, self must be passive.

        if (bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE != pinfo.primaryStatus()) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " Partition [" << partitionId << "]: "
                           << "Received a partition sync event from: "
                           << source->nodeDescription()
                           << ", while self is ACTIVE replica.";
            return;  // RETURN
        }
    }
    else if (pinfo.primary()->nodeId() != source->nodeId()) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Received a partition sync event from: "
                       << source->nodeDescription()
                       << ", but neither self is primary nor the sender is "
                       << "perceived as the primary.";
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<unsigned int>(partitionId));

    mqbs::FileStore* fs =
        d_fileStores[static_cast<unsigned int>(partitionId)].get();
    BSLS_ASSERT_SAFE(fs);

    const int rc = d_recoveryManager_mp->processReceiveDataChunks(
        eventData.storageEvent(),
        source,
        fs,
        partitionId);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received status " << rc
                  << " for receiving data chunks from Recovery Manager.";

    EventData eventDataVecOut;
    eventDataVecOut.emplace_back(source,
                                 -1,  // placeholder requestId
                                 partitionId,
                                 1);

    // As replica, we will send either success or failure
    // ReplicaDataResponsePush/Drop depending on 'rc'.
    if (rc == 1) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_DONE_RECEIVING_DATA_CHUNKS,
            eventDataVecOut);
    }
    else if (rc != 0) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_ERROR_RECEIVING_DATA_CHUNKS,
            eventDataVecOut);
    }
    // Note: 'rc == 0' means that we have received some data chunks, but more
    // is coming, and there is no need to trigger FSM event.
}

void StorageManager::do_removeStorage(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (fs->isOpen()) {
        fs->close(false);  // flush
        fs->deprecateFileSet();
    }
    else {
        d_recoveryManager_mp->deprecateFileSet(partitionId);
    }

    BMQTSK_ALARMLOG_ALARM("REPLICATION")
        << d_clusterData_p->identity().description() << " Partition ["
        << partitionId << "]: "
        << "self's storage is out of sync with primary and cannot be healed "
        << "trivially. Removing entire storage and aborting broker."
        << BMQTSK_ALARMLOG_END;

    mqbu::ExitUtil::terminate(mqbu::ExitCode::e_STORAGE_OUT_OF_SYNC);  // EXIT
}

void StorageManager::do_incrementNumRplcaDataRspn(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData      = eventDataVec[0];
    const int                    partitionId    = eventData.partitionId();
    const int                    incrementCount = eventData.incrementCount();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());
    BSLS_ASSERT_SAFE(incrementCount >= 1);

    d_numReplicaDataResponsesReceivedVec[partitionId] += incrementCount;
}

void StorageManager::do_checkQuorumRplcaDataRspn(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());

    if (d_numReplicaDataResponsesReceivedVec[partitionId] >= d_seqNumQuorum) {
        // If we have a quorum of replica data responses (including self)
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "A quorum ("
                      << d_numReplicaDataResponsesReceivedVec[partitionId]
                      << ") of cluster nodes now has healed partitions. "
                      << "Transitiong self to healed primary";

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_QUORUM_REPLICA_DATA_RSPN,
            eventWithData.second);
    }
}

void StorageManager::do_clearRplcaDataRspnCnt(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->state() ==
                     PartitionFSM::State::e_UNKNOWN);

    d_numReplicaDataResponsesReceivedVec[partitionId] = 0;
}

void StorageManager::do_reapplyEvent(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << " Re-apply event: " << eventWithData.first
                  << " in the Partition FSM.";

    args->eventsQueue()->push(eventWithData);
}

void StorageManager::do_checkQuorumSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());

    if (d_nodeToSeqNumCtxMapVec[partitionId].size() >= d_seqNumQuorum) {
        // If we have a quorum of Replica Sequence numbers (including self Seq)

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Achieved a quorum of SeqNums with a count of "
                      << d_nodeToSeqNumCtxMapVec[partitionId].size();

        args->eventsQueue()->emplace(PartitionFSM::Event::e_QUORUM_REPLICA_SEQ,
                                     eventWithData.second);
    }
}

void StorageManager::do_findHighestSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());
    BSLS_ASSERT_SAFE(d_nodeToSeqNumCtxMapVec[partitionId].size() >=
                     d_seqNumQuorum);

    const NodeToSeqNumCtxMap& nodeToSeqNumCtxMap =
        d_nodeToSeqNumCtxMapVec[partitionId];

    // Initialize highest sequence number with self/primary sequence number.
    mqbnet::ClusterNode* highestSeqNumNode =
        d_clusterData_p->membership().selfNode();
    bmqp_ctrlmsg::PartitionSequenceNumber highestPartitionSeqNum(
        nodeToSeqNumCtxMap.at(highestSeqNumNode).first);

    // Find out highest sequence number and number of up-to-date nodes.
    for (NodeToSeqNumCtxMapCIter cit = nodeToSeqNumCtxMap.cbegin();
         cit != nodeToSeqNumCtxMap.cend();
         cit++) {
        if (cit->second.first > highestPartitionSeqNum) {
            highestSeqNumNode      = cit->first;
            highestPartitionSeqNum = cit->second.first;
        }
    }

    const unsigned int primaryLeaseId =
        d_partitionInfoVec[partitionId].primaryLeaseId();
    const bool selfHighestSeq = highestSeqNumNode ==
                                d_clusterData_p->membership().selfNode();

    EventData newEventDataVec;
    newEventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                                 -1,  // placeholder requestId
                                 partitionId,
                                 1,  // incrementCount
                                 d_clusterData_p->membership().selfNode(),
                                 primaryLeaseId,
                                 highestPartitionSeqNum,
                                 highestSeqNumNode);

    if (selfHighestSeq) {
        args->eventsQueue()->emplace(PartitionFSM::Event::e_SELF_HIGHEST_SEQ,
                                     newEventDataVec);
    }
    else {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_REPLICA_HIGHEST_SEQ,
            newEventDataVec);
    }
}

void StorageManager::do_flagFailedReplicaSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode()->nodeId() ==
                     d_partitionInfoVec[partitionId].primary()->nodeId());
    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode()->nodeId() !=
                     eventData.source()->nodeId());

    d_nodeToSeqNumCtxMapVec[partitionId].erase(eventData.source());
}

void StorageManager::do_transitionToActivePrimary(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);
    const int partitionId = eventDataVec[0].partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->state() ==
                     PartitionFSM::State::e_PRIMARY_HEALED);

    StorageUtil::onPartitionPrimarySync(d_fileStores[partitionId].get(),
                                        &d_partitionInfoVec[partitionId],
                                        d_clusterData_p,
                                        d_partitionPrimaryStatusCb,
                                        partitionId,
                                        0);  // status
}

void StorageManager::do_reapplyDetectSelfPrimary(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);
    const int partitionId = eventDataVec[0].partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->state() ==
                     PartitionFSM::State::e_UNKNOWN);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Re-apply transition to primary in the Partition FSM.";

    EventData eventDataVecOut;
    eventDataVecOut.emplace_back(
        d_clusterData_p->membership().selfNode(),
        -1,  // placeholder responseId
        partitionId,
        1,
        d_partitionInfoVec[partitionId].primary(),
        d_partitionInfoVec[partitionId].primaryLeaseId());
    args->eventsQueue()->emplace(PartitionFSM::Event::e_DETECT_SELF_PRIMARY,
                                 eventDataVecOut);
}

void StorageManager::do_reapplyDetectSelfReplica(
    const PartitionFSMArgsSp& args)
{
    // executed by the *QUEUE DISPATCHER* thread associated with the paritionId
    // contained in 'args'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);
    const int partitionId = eventDataVec[0].partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->state() ==
                     PartitionFSM::State::e_UNKNOWN);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Re-apply transition to replica in the Partition FSM.";

    EventData eventDataVecOut;
    eventDataVecOut.emplace_back(
        d_clusterData_p->membership().selfNode(),
        -1,  // placeholder responseId
        partitionId,
        1,
        d_partitionInfoVec[partitionId].primary(),
        d_partitionInfoVec[partitionId].primaryLeaseId());
    args->eventsQueue()->emplace(PartitionFSM::Event::e_DETECT_SELF_REPLICA,
                                 eventDataVecOut);
}

// PRIVATE ACCESSORS
bool StorageManager::allPartitionsAvailable() const
{
    // executed by *QUEUE_DISPATCHER* thread associated with *ANY* partition

    if (d_numPartitionsRecoveredFully !=
        static_cast<int>(d_fileStores.size())) {
        return false;  // RETURN
    }

    return bsl::all_of(d_partitionInfoVec.cbegin(),
                       d_partitionInfoVec.cend(),
                       &isPrimaryActive);
}

// CREATORS
StorageManager::StorageManager(
    const mqbcfg::ClusterDefinition& clusterConfig,
    mqbi::Cluster*                   cluster,
    mqbc::ClusterData*               clusterData,
    const mqbc::ClusterState&        clusterState,
    mqbi::DomainFactory*             domainFactory,
    mqbi::Dispatcher*                dispatcher,
    bsls::Types::Int64               watchDogTimeoutDuration,
    const RecoveryStatusCb&          recoveryStatusCb,
    const PartitionPrimaryStatusCb&  partitionPrimaryStatusCb,
    bslma::Allocator*                allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_watchDogEventHandles(allocator)
, d_watchDogTimeoutInterval(watchDogTimeoutDuration)
, d_lowDiskspaceWarning(false)
, d_unrecognizedDomainsLock()
, d_unrecognizedDomains(allocator)
, d_blobSpPool_p(&clusterData->blobSpPool())
, d_domainFactory_p(domainFactory)
, d_dispatcher_p(dispatcher)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_clusterState(clusterState)
, d_clusterConfig(clusterConfig)
, d_fileStores(allocator)
, d_miscWorkThreadPool(1, 100, allocator)
, d_recoveryStatusCb(recoveryStatusCb)
, d_partitionPrimaryStatusCb(partitionPrimaryStatusCb)
, d_storagesLock()
, d_storages(allocator)
, d_appKeysLock()
, d_appKeysVec(allocator)
, d_partitionInfoVec(allocator)
, d_partitionFSMVec(allocator)
, d_bufferedPrimaryStatusAdvisoryInfosVec(allocator)
, d_numPartitionsRecoveredFully(0)
, d_numPartitionsRecoveredQueues(0)
, d_recoveryStartTimes(allocator)
, d_nodeToSeqNumCtxMapVec(allocator)
, d_seqNumQuorum((d_clusterConfig.nodes().size() / 2) + 1)  // TODO: Config??
, d_numReplicaDataResponsesReceivedVec(allocator)
, d_isQueueKeyInfoMapVecInitialized(false)
, d_queueKeyInfoMapVec(allocator)
, d_minimumRequiredDiskSpace(
      StorageUtil::findMinReqDiskSpace(d_clusterConfig.partitionConfig()))
, d_storageMonitorEventHandle()
, d_gcMessagesEventHandle()
, d_recoveryManager_mp()
, d_replicationFactor(0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(allocator);
    BSLS_ASSERT_SAFE(d_clusterData_p);
    BSLS_ASSERT_SAFE(d_cluster_p);
    BSLS_ASSERT_SAFE(d_recoveryStatusCb);
    BSLS_ASSERT_SAFE(d_partitionPrimaryStatusCb);

    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();

    d_watchDogEventHandles.resize(partitionCfg.numPartitions());
    d_unrecognizedDomains.resize(partitionCfg.numPartitions());
    d_fileStores.resize(partitionCfg.numPartitions());
    d_storages.resize(partitionCfg.numPartitions());
    d_appKeysVec.resize(partitionCfg.numPartitions());
    d_partitionInfoVec.resize(partitionCfg.numPartitions());
    d_bufferedPrimaryStatusAdvisoryInfosVec.resize(
        partitionCfg.numPartitions(),
        PrimaryStatusAdvisoryInfos(allocator));
    d_recoveryStartTimes.resize(partitionCfg.numPartitions());
    d_nodeToSeqNumCtxMapVec.resize(partitionCfg.numPartitions());
    d_numReplicaDataResponsesReceivedVec.resize(partitionCfg.numPartitions());
    d_queueKeyInfoMapVec.resize(partitionCfg.numPartitions());

    for (int p = 0; p < partitionCfg.numPartitions(); p++) {
        d_partitionFSMVec.emplace_back(new (*allocator)
                                           PartitionFSM(*this, allocator),
                                       allocator);
        d_partitionFSMVec.back()->registerObserver(this);
    }

    // Set the default replication-factor to one more than half of the cluster.
    // Do this here (rather than in the initializer-list) to avoid accessing
    // 'd_cluster_p' before the above non-nullness check.
    d_replicationFactor = (d_cluster_p->netCluster().nodes().size() / 2) + 1;
}

StorageManager::~StorageManager()
{
    // NOTHING FOR NOW
}

// MANIPULATORS
//   (virtual: mqbc::PartitionFSMObserver)
void StorageManager::onTransitionToPrimaryHealed(
    int                    partitionId,
    BSLS_ANNOTATION_UNUSED PartitionStateTableState::Enum oldState)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    onPartitionRecovery(partitionId);
}

void StorageManager::onTransitionToReplicaHealed(
    int                    partitionId,
    BSLS_ANNOTATION_UNUSED mqbc::PartitionStateTableState::Enum oldState)
{
    // executed by *QUEUE_DISPATCHER* thread associated with 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    onPartitionRecovery(partitionId);
}

// MANIPULATORS
int StorageManager::start(bsl::ostream& errorDescription)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                        = 0,
        rc_PARTITION_LOCATION_NONEXISTENT = -1,
        rc_NOT_ENOUGH_DISK_SPACE          = -2,
        rc_THREAD_POOL_START_FAILURE      = -3,
        rc_RECOVERY_MANAGER_FAILURE       = -4
    };

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Starting StorageManager";

    // For convenience:
    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();

    int rc = StorageUtil::validatePartitionDirectory(partitionCfg,
                                                     errorDescription);
    if (rc != rc_SUCCESS) {
        return rc * 10 + rc_PARTITION_LOCATION_NONEXISTENT;  // RETURN
    }

    rc = StorageUtil::validateDiskSpace(partitionCfg,
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
        bdlf::BindUtil::bind(&StorageUtil::storageMonitorCb,
                             &d_lowDiskspaceWarning,
                             &d_isStarted,
                             d_minimumRequiredDiskSpace,
                             d_clusterData_p->identity().description(),
                             d_clusterConfig.partitionConfig()));

    rc = StorageUtil::assignPartitionDispatcherThreads(
        &d_miscWorkThreadPool,
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
                             bdlf::PlaceHolders::_1,    // partitionId
                             bdlf::PlaceHolders::_2));  // queueKeyUriMap));
    if (rc != rc_SUCCESS) {
        return rc * 10 + rc_THREAD_POOL_START_FAILURE;  // RETURN
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

    d_recoveryManager_mp.load(new (*recoveryManagerAllocator) RecoveryManager(
                                  &d_clusterData_p->bufferFactory(),
                                  d_clusterConfig,
                                  *d_clusterData_p,
                                  dsCfg,
                                  recoveryManagerAllocator),
                              recoveryManagerAllocator);

    rc = d_recoveryManager_mp->start(errorDescription);

    if (rc != rc_SUCCESS) {
        return rc * 10 + rc_RECOVERY_MANAGER_FAILURE;  // RETURN
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
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    for (size_t pid = 0; pid < d_fileStores.size(); ++pid) {
        EventData eventDataVec;
        eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                                  -1,  // placeholder requestId
                                  pid,
                                  1);

        dispatchEventToPartition(d_fileStores[pid].get(),
                                 PartitionFSM::Event::e_RST_UNKNOWN,
                                 eventDataVec);
    }

    for (int p = 0; p < d_clusterConfig.partitionConfig().numPartitions();
         p++) {
        d_fileStores[p]->execute(
            bdlf::BindUtil::bind(&PartitionFSM::unregisterObserver,
                                 d_partitionFSMVec[p].get(),
                                 this));
    }

    d_clusterData_p->scheduler().cancelEventAndWait(&d_gcMessagesEventHandle);
    d_clusterData_p->scheduler().cancelEventAndWait(
        &d_storageMonitorEventHandle);
    d_recoveryManager_mp->stop();

    StorageUtil::stop(
        &d_fileStores,
        d_clusterData_p->identity().description(),
        bdlf::BindUtil::bind(&StorageManager::shutdownCb,
                             this,
                             bdlf::PlaceHolders::_1,    // partitionId
                             bdlf::PlaceHolders::_2));  // latch
}

void StorageManager::initializeQueueKeyInfoMap(
    const mqbc::ClusterState& clusterState)
{
    // executed by the *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    if (d_isQueueKeyInfoMapVecInitialized) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Queue key info map should only be initialized "
                      << "once, but the initalization method is called more "
                      << "than once.  This can happen if the node goes "
                      << "back-and-forth between healing and healed FSM "
                      << "states.  Please check.";

        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        bsl::all_of(d_queueKeyInfoMapVec.cbegin(),
                    d_queueKeyInfoMapVec.cend(),
                    bdlf::MemFnUtil::memFn(&QueueKeyInfoMap::empty)));

    // Populate 'd_queueKeyInfoMapVec' from cluster state
    for (DomainStatesCIter dscit = clusterState.domainStates().cbegin();
         dscit != clusterState.domainStates().cend();
         ++dscit) {
        for (UriToQueueInfoMapCIter cit = dscit->second->queuesInfo().cbegin();
             cit != dscit->second->queuesInfo().cend();
             ++cit) {
            BSLS_ASSERT_SAFE(cit->second);
            const ClusterStateQueueInfo& csQinfo = *(cit->second);

            mqbs::DataStoreConfigQueueInfo qinfo;
            qinfo.setCanonicalQueueUri(csQinfo.uri().asString());
            qinfo.setPartitionId(csQinfo.partitionId());
            for (AppIdInfosCIter appIdCit = csQinfo.appIdInfos().cbegin();
                 appIdCit != csQinfo.appIdInfos().cend();
                 ++appIdCit) {
                qinfo.addAppIdKeyPair(*appIdCit);
            }

            d_queueKeyInfoMapVec.at(csQinfo.partitionId())
                .insert(bsl::make_pair(csQinfo.key(), qinfo));
        }
    }

    d_isQueueKeyInfoMapVecInitialized = true;
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

    StorageUtil::registerQueue(d_cluster_p,
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
            bdlf::BindUtil::bind(&StorageUtil::unregisterQueueDispatched,
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

    return StorageUtil::updateQueuePrimary(
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
        true);  // isCSLMode
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

    // This routine is executed at follower nodes upon commit callback of Queue
    // Assignment Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(
            bdlf::BindUtil::bind(&StorageUtil::registerQueueReplicaDispatched,
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
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    // This routine is executed at follower nodes upon commit callback of Queue
    // Unassigned Advisory or Queue Update Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(bdlf::BindUtil::bind(
            &StorageUtil::unregisterQueueReplicaDispatched,
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
            true));  // isCSLMode

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

    // This routine is executed at follower nodes upon commit callback of Queue
    // Queue Update Advisory from the leader.

    mqbi::DispatcherEvent* queueEvent = d_dispatcher_p->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent)
        .makeDispatcherEvent()
        .setCallback(
            bdlf::BindUtil::bind(&StorageUtil::updateQueueReplicaDispatched,
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
                                 true,  // isCSLMode
                                 domain,
                                 allowDuplicate));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

mqbu::StorageKey StorageManager::generateAppKey(const bsl::string& appId,
                                                int                partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'
    // or by *CLUSTER DISPATCHER* thread.

    return StorageUtil::generateAppKey(&d_appKeysVec[partitionId],
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
            bdlf::BindUtil::bind(&StorageUtil::setQueueDispatched,
                                 &d_storages[partitionId],
                                 &d_storagesLock,
                                 bdlf::PlaceHolders::_1,  // processor
                                 d_clusterData_p->identity().description(),
                                 partitionId,
                                 uri,
                                 queue));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

void StorageManager::setQueueRaw(mqbi::Queue*     queue,
                                 const bmqt::Uri& uri,
                                 int              partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue);
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(queue));

    StorageUtil::setQueueDispatched(&d_storages[partitionId],
                                    &d_storagesLock,
                                    processorForPartition(partitionId),
                                    d_clusterData_p->identity().description(),
                                    partitionId,
                                    uri,
                                    queue);
}

void StorageManager::setPrimaryForPartition(int                  partitionId,
                                            mqbnet::ClusterNode* primaryNode,
                                            unsigned int primaryLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(primaryNode);

    if (primaryNode->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        processPrimaryDetect(partitionId, primaryNode, primaryLeaseId);
    }
    else {
        processReplicaDetect(partitionId, primaryNode, primaryLeaseId);
    }
}

void StorageManager::clearPrimaryForPartition(int                  partitionId,
                                              mqbnet::ClusterNode* primary)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(primary);
    // We always clear the primary info from ClusterState first
    BSLS_ASSERT_SAFE(
        !d_clusterState.partitionsInfo().at(partitionId).primaryNode());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Self Transition back to Unknown in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(
        d_clusterData_p->membership().selfNode(),
        -1,  // placeholder requestId
        partitionId,
        1,
        primary,
        d_clusterState.partitionsInfo().at(partitionId).primaryLeaseId());

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_RST_UNKNOWN,
                             eventDataVec);
}

void StorageManager::setPrimaryStatusForPartition(
    int                                partitionId,
    bmqp_ctrlmsg::PrimaryStatus::Value value)
{
    // executed by cluster *DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(
        d_dispatcher_p->inDispatcherThread(&d_clusterData_p->cluster()));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageManager::setPrimaryStatusForPartitionDispatched,
        this,
        partitionId,
        value));
}

// MANIPULATORS
void StorageManager::processPrimaryStateRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isPrimaryStateRequestValue());

    const bmqp_ctrlmsg::PrimaryStateRequest& primaryStateRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .primaryStateRequest();

    int partitionId = primaryStateRequest.partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received PrimaryStateRequest: " << message << " from "
                  << source->nodeDescription() << ".";

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "PrimaryStateRequest.";
        return;  // RETURN
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(source,
                              message.rId().isNull() ? -1
                                                     : message.rId().value(),
                              partitionId,
                              1,
                              primaryStateRequest.sequenceNumber());

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_PRIMARY_STATE_RQST,
                             eventDataVec);
}

void StorageManager::processReplicaStateRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaStateRequestValue());

    const bmqp_ctrlmsg::ReplicaStateRequest& replicaStateRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaStateRequest();

    int partitionId = replicaStateRequest.partitionId();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Received ReplicaStateRequest: " << message << " from "
                  << source->nodeDescription() << ".";

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaStateRequest.";
        return;  // RETURN
    }

    EventData eventDataVec;
    eventDataVec.emplace_back(source,
                              message.rId().isNull() ? -1
                                                     : message.rId().value(),
                              partitionId,
                              1,
                              replicaStateRequest.sequenceNumber());

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_STATE_RQST,
                             eventDataVec);
}

void StorageManager::processReplicaDataRequest(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(
        message.choice().clusterMessage().choice().isPartitionMessageValue());
    BSLS_ASSERT_SAFE(message.choice()
                         .clusterMessage()
                         .choice()
                         .partitionMessage()
                         .choice()
                         .isReplicaDataRequestValue());

    const bmqp_ctrlmsg::ReplicaDataRequest& replicaDataRequest =
        message.choice()
            .clusterMessage()
            .choice()
            .partitionMessage()
            .choice()
            .replicaDataRequest();
    switch (replicaDataRequest.replicaDataType()) {
    case bmqp_ctrlmsg::ReplicaDataType::E_PULL: {
        processReplicaDataRequestPull(message, source);
    } break;  // BREAK
    case bmqp_ctrlmsg::ReplicaDataType::E_PUSH: {
        processReplicaDataRequestPush(message, source);
    } break;  // BREAK
    case bmqp_ctrlmsg::ReplicaDataType::E_DROP: {
        processReplicaDataRequestDrop(message, source);
    } break;  // BREAK
    case bmqp_ctrlmsg::ReplicaDataType::E_UNKNOWN: BSLS_ANNOTATION_FALLTHROUGH;
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << d_clusterData_p->identity().description()
            << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

int StorageManager::makeStorage(bsl::ostream& errorDescription,
                                bslma::ManagedPtr<mqbi::Storage>* out,
                                const bmqt::Uri&                  uri,
                                const mqbu::StorageKey&           queueKey,
                                int                               partitionId,
                                const bsls::Types::Int64          messageTtl,
                                int maxDeliveryAttempts,
                                const mqbconfm::StorageDefinition& storageDef)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return StorageUtil::makeStorage(errorDescription,
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
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster().isLocal());
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

    const unsigned int pid = StorageUtil::extractPartitionId<false>(rawEvent);

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << pid << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "storage event.";
        return;  // RETURN
    }

    // Ensure that 'pid' is valid.
    if (pid >= d_clusterState.partitions().size()) {
        BMQTSK_ALARMLOG_ALARM("STORAGE")
            << d_cluster_p->description() << " Partition [" << pid
            << "]: " << "Received "
            << (rawEvent.isStorageEvent() ? "storage " : "partition-sync ")
            << "event from node " << source->nodeDescription() << " with "
            << "invalid Partition Id [" << pid << "]. Ignoring "
            << "entire storage event." << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    EventData eventDataVec;
    eventDataVec.emplace_back(event.clusterNode(), pid, 1, event.blob());

    mqbs::FileStore* fs = d_fileStores[pid].get();
    BSLS_ASSERT_SAFE(fs);

    if (rawEvent.isStorageEvent()) {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_LIVE_DATA,
                                 eventDataVec);
    }
    else {
        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_RECOVERY_DATA,
                                 eventDataVec);
    }
}

void StorageManager::processStorageSyncRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method can only be invoked in non-CSL mode");
}

void StorageManager::processPartitionSyncStateRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method can only be invoked in non-CSL mode");
}

void StorageManager::processPartitionSyncDataRequest(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method can only be invoked in non-CSL mode");
}

void StorageManager::processPartitionSyncDataRequestStatus(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::ControlMessage& message,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method can only be invoked in non-CSL mode");
}

void StorageManager::processRecoveryEvent(
    BSLS_ANNOTATION_UNUSED const mqbi::DispatcherRecoveryEvent& event)
{
    BSLS_ASSERT_SAFE(false &&
                     "This method can only be invoked in non-CSL mode");
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
    const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    mqbnet::ClusterNode*                       source)
{
    // executed by *ANY* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(d_fileStores.size() >
                     static_cast<size_t>(advisory.partitionId()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_isStarted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << advisory.partitionId() << "]: "
                      << " Not buffering primary status advisory as StorageMgr"
                      << " is not started.";
        return;  // RETURN
    }

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << advisory.partitionId() << "]: "
                      << " Not buffering primary status advisory as cluster"
                      << " is stopping.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[advisory.partitionId()].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageManager::bufferPrimaryStatusAdvisoryDispatched,
        this,
        advisory,
        source));
}

void StorageManager::processPrimaryStatusAdvisory(
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::PrimaryStatusAdvisory& advisory,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITION
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    BSLS_ASSERT_OPT(false &&
                    "This method should only be invoked in non-FSM mode");
}

void StorageManager::processReplicaStatusAdvisory(
    int                             partitionId,
    mqbnet::ClusterNode*            source,
    bmqp_ctrlmsg::NodeStatus::Value status)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(source);
    BSLS_ASSERT_SAFE(d_fileStores.size() > static_cast<size_t>(partitionId));

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << " Partition [" << partitionId << "]: "
                      << "Cluster is stopping; skipping processing of "
                      << "ReplicaStatusAdvisory.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageUtil::processReplicaStatusAdvisoryDispatched,
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
    // partitions for which self is replica, self will issue a syncPt and a
    // 'passive' replica status advisory.

    for (size_t i = 0; i < d_fileStores.size(); ++i) {
        mqbs::FileStore* fs = d_fileStores[i].get();

        fs->execute(bdlf::BindUtil::bind(
            &StorageManager::processShutdownEventDispatched,
            this,
            i));  // partitionId
    }
}

void StorageManager::applyForEachQueue(int                 partitionId,
                                       const QueueFunctor& functor) const
{
    // executed by the *QUEUE DISPATCHER* thread associated with 'paritionId'

    // PRECONDITIONS
    const mqbs::FileStore& fs = fileStore(partitionId);
    BSLS_ASSERT_SAFE(fs.inDispatcherThread());

    if (!fs.isOpen()) {
        return;  // RETURN
    }

    fs.applyForEachQueue(functor);
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

    return StorageUtil::processCommand(
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

    StorageUtil::gcUnrecognizedDomainQueues(&d_fileStores,
                                            &d_unrecognizedDomainsLock,
                                            d_unrecognizedDomains);
}

mqbs::FileStore& StorageManager::fileStore(int partitionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return *d_fileStores[partitionId].get();
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

    return StorageUtil::isStorageEmpty(&d_storagesLock,
                                       d_storages[partitionId],
                                       uri,
                                       partitionId);
}

const mqbs::FileStore& StorageManager::fileStore(int partitionId) const
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    return *d_fileStores[partitionId].get();
}

}  // close package namespace
}  // close enterprise namespace
