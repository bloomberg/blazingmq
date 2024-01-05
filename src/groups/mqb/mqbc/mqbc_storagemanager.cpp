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

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_blob.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_algorithm.h>
#include <bsl_utility.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbc {

namespace {
const int k_GC_MESSAGES_INTERVAL_SECONDS = 60;
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
        mqbc::StorageUtil::printRecoveryPhaseOneBanner(
            BALL_LOG_OUTPUT_STREAM,
            d_clusterData_p->identity().description(),
            partitionId);
    }

    if (d_cluster_p->isStopping()) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": PartitionId [" << partitionId
                      << "] cluster is stopping; skipping partition recovery.";
        return;  // RETURN
    }

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(!fs->isOpen());
    BSLS_ASSERT_SAFE(fs->config().partitionId() == partitionId);

    d_recoveryStartTimes[partitionId] = mwcsys::Time::highResolutionTimer();

    mwcu::MemOutStream errorDesc;
    int rc = d_recoveryManager_mp->openRecoveryFileSet(errorDesc, partitionId);
    if (rc == 1) {
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << " PartitionId [" << partitionId << "]: "
                      << "No recovery file set is found.  Create new set.";

        // Creating file set will populate the file headers.  We need to do
        // this before receiving data chunks from the recovery peer.
        rc = d_recoveryManager_mp->createRecoveryFileSet(errorDesc,
                                                         fs,
                                                         partitionId);
        if (rc != 0) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " PartitionId [" << partitionId << "]: "
                           << "Error while creating recovery file set, rc :"
                           << rc << ", error: " << errorDesc.str();
        }

        return;  // RETURN
    }

    if (rc != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " PartitionId [" << partitionId << "]: "
                       << "Error while recovering file set from storage, rc: "
                       << rc << ", error: " << errorDesc.str();
        return;  // RETURN
    }
}

void StorageManager::shutdownCb(int partitionId, bslmt::Latch* latch)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'
    StorageUtil::shutdown(partitionId,
                          latch,
                          &d_fileStores,
                          d_clusterData_p,
                          d_clusterConfig);
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

    BALL_LOG_WARN << d_clusterData_p->identity().description()
                  << " Partition [" << partitionId << "]: "
                  << "Watch dog triggered because partition startup healing "
                  << "sequence was not completed in the configured time of "
                  << d_watchDogTimeoutInterval.totalSeconds() << " seconds.";

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId);

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
                  << ": Received status " << status
                  << " for Sending Data Chunks from Recovery Manager for"
                  << " partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(destination, requestId, partitionId, range);

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
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mwcu::MemOutStream out;
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

        if (success) {
            // All partitions have opened successfully.  Schedule a recurring
            // event in StorageMgr, which will in turn enqueue an event in each
            // partition's dispatcher thread for GC'ing expired messages as
            // well as cleaning history.

            d_clusterData_p->scheduler()->scheduleRecurringEvent(
                &d_gcMessagesEventHandle,
                bsls::TimeInterval(k_GC_MESSAGES_INTERVAL_SECONDS),
                bdlf::BindUtil::bind(&StorageManager::forceFlushFileStores,
                                     this));

            d_recoveryStatusCb(0);
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

void StorageManager::processPrimaryStateResponseDispatched(
    const RequestManagerType::RequestSp& context,
    mqbnet::ClusterNode*                 responder)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    BSLS_ASSERT_SAFE(context->response().rId() != NULL);
    BSLS_ASSERT_SAFE(context->request().rId().value() ==
                     context->response().rId().value());

    const int responseId  = context->response().rId().value();
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

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (context->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received FAIL_PrmryStateRspn event "
                      << context->response().choice().status()
                      << " from primary " << responder->nodeDescription();

        EventData eventDataVec;
        eventDataVec.emplace_back(responder, responseId, partitionId);

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

    EventData eventDataVec;
    eventDataVec.emplace_back(responder,
                              responseId,
                              partitionId,
                              response.sequenceNumber());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": received PrimaryStateResponse " << response << " from "
                  << responder->nodeDescription();

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
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    const NodeResponsePairs& pairs = requestContext->response();
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

    EventData        eventDataVec(d_allocator_p);
    EventData        failedEventDataVec(d_allocator_p);
    mqbs::FileStore* fs = d_fileStores[requestPartitionId].get();

    for (NodeResponsePairsCIter cit = pairs.cbegin(); cit != pairs.cend();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->first);

        if (cit->second.choice().isStatusValue()) {
            BALL_LOG_WARN << d_clusterData_p->identity().description()
                          << ": received failed ReplicaStateResponse "
                          << cit->second.choice().status() << " from "
                          << cit->first->nodeDescription()
                          << ". Skipping this node's response.";
            failedEventDataVec.emplace_back(cit->first,
                                            cit->second.rId().value(),
                                            requestPartitionId);

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

        int responseId = cit->second.rId().value();

        const bmqp_ctrlmsg::ReplicaStateResponse& response =
            cit->second.choice()
                .clusterMessage()
                .choice()
                .partitionMessage()
                .choice()
                .replicaStateResponse();

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": received ReplicaStateResponse " << response
                      << " from " << cit->first->nodeDescription();

        BSLS_ASSERT_SAFE(
            d_partitionInfoVec[response.partitionId()].primary() ==
            d_clusterData_p->membership().selfNode());

        unsigned int primaryLeaseId =
            d_partitionInfoVec[response.partitionId()].primaryLeaseId();

        eventDataVec.emplace_back(cit->first,
                                  responseId,
                                  response.partitionId(),
                                  d_clusterData_p->membership().selfNode(),
                                  primaryLeaseId,
                                  response.sequenceNumber());

        BSLS_ASSERT_SAFE(requestPartitionId == response.partitionId());
    }

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
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());

    BSLS_ASSERT_SAFE(context->response().rId() != NULL);
    BSLS_ASSERT_SAFE(context->request().rId().value() ==
                     context->response().rId().value());

    const int responseId = context->response().rId().value();

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

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (context->result() != bmqt::GenericResult::e_SUCCESS) {
        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": received FAIL_ReplicaDataResponse event "
                      << context->response().choice().status()
                      << " from replica " << responder->nodeDescription();

        EventData eventDataVec;
        eventDataVec.emplace_back(responder, responseId, partitionId);

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

    EventData eventDataVec;
    eventDataVec.emplace_back(
        responder,
        responseId,
        partitionId,
        PartitionSeqNumDataRange(response.beginSequenceNumber(),
                                 response.endSequenceNumber()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " PartitionId [" << partitionId
                  << "]: received ReplicaDataResponse " << response
                  << " of type [" << dataType << "] from "
                  << responder->nodeDescription();

    switch (dataType) {
    case bmqp_ctrlmsg::ReplicaDataType::E_UNKNOWN: {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " PartitionId [" << partitionId
                       << "]: ReplicaDataResponse has an unknown data type, "
                       << "ignoring.";
    } break;
    case bmqp_ctrlmsg::ReplicaDataType::E_PULL: {
        if (d_recoveryManager_mp->expectedDataChunks(partitionId)) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " PartitionId [" << partitionId
                           << "]: Ignoring premature ReplicaDataResponse_PULL "
                           << "because self is still expecting data chunks.";

            return;  // RETURN
        }

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

void StorageManager::processShutdownEventDispatched(int partitionId)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << " PartitionId [" << partitionId
                  << "]: received shutdown event.";

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    if (!d_partitionFSMVec[partitionId]->isSelfHealed()) {
        EventData eventDataVec;
        eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                                  -1,  // placeholder requestId
                                  partitionId);

        dispatchEventToPartition(fs,
                                 PartitionFSM::Event::e_RST_UNKNOWN,
                                 eventDataVec);
    }

    mqbc::StorageUtil::processShutdownEventDispatched(
        d_clusterData_p,
        &d_partitionInfoVec[partitionId],
        fs,
        partitionId);
}

void StorageManager::forceFlushFileStores()
{
    // executed by scheduler's dispatcher thread

    mqbc::StorageUtil::forceFlushFileStores(&d_fileStores);
}

void StorageManager::do_startWatchDog(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    d_clusterData_p->scheduler()->scheduleEvent(
        &d_watchDogEventHandles[partitionId],
        d_clusterData_p->scheduler()->now() + d_watchDogTimeoutInterval,
        bdlf::BindUtil::bind(&StorageManager::onWatchDog, this, partitionId));
}

void StorageManager::do_stopWatchDog(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const int partitionId = eventDataVec[0].partitionId();

    if (d_clusterData_p->scheduler()->cancelEvent(
            d_watchDogEventHandles[partitionId]) != 0) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " Partition [" << partitionId << "]: "
                       << "Failed to cancel WatchDog.";
    }
}

void StorageManager::do_populateQueueKeyInfoMap(
    BSLS_ANNOTATION_UNUSED const PartitionFSMArgsSp& args)
{
    // Populate 'd_queueKeyInfoMap' from cluster state
    for (DomainStatesCIter dscit = d_clusterState.domainStates().cbegin();
         dscit != d_clusterState.domainStates().cend();
         ++dscit) {
        for (UriToQueueInfoMapCIter cit = dscit->second->queuesInfo().cbegin();
             cit != dscit->second->queuesInfo().cend();
             ++cit) {
            const ClusterStateQueueInfo& csQinfo = *(cit->second);

            mqbs::DataStoreConfigQueueInfo qinfo;
            qinfo.setCanonicalQueueUri(csQinfo.uri().asString());
            qinfo.setPartitionId(csQinfo.partitionId());
            for (AppIdInfosCIter appIdCit = csQinfo.appIdInfos().cbegin();
                 appIdCit != csQinfo.appIdInfos().cend();
                 ++appIdCit) {
                qinfo.addAppIdKeyPair(*appIdCit);
            }

            d_queueKeyInfoMap.insert(bsl::make_pair(csQinfo.key(), qinfo));
        }
    }
}

void StorageManager::do_storeSelfSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData&    eventData   = eventDataVec[0];
    int                             partitionId = eventData.partitionId();
    const PartitionSeqNumDataRange& dataRange =
        eventData.partitionSeqNumDataRange();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode() ==
                     eventData.source());
    BSLS_ASSERT_SAFE(dataRange.first <= dataRange.second);

    if (dataRange.second > bmqp_ctrlmsg::PartitionSequenceNumber()) {
        d_nodeToSeqNumMapPartitionVec[partitionId][eventData.source()] =
            dataRange.second;
    }
    else {
        d_recoveryManager_mp->recoverSeqNum(
            &d_nodeToSeqNumMapPartitionVec[partitionId][eventData.source()],
            partitionId);
    }
}

void StorageManager::do_storePrimarySeq(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    PartitionInfo& partitionInfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(partitionInfo.primary() == eventData.source());

    d_nodeToSeqNumMapPartitionVec[partitionId][eventData.source()] = seqNum;
}

void StorageManager::do_storeReplicaSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    for (EventDataCIter cit = eventDataVec.cbegin();
         cit != eventDataVec.cend();
         cit++) {
        int partitionId = cit->partitionId();
        const bmqp_ctrlmsg::PartitionSequenceNumber& seqNum =
            cit->partitionSequenceNumber();

        BSLS_ASSERT_SAFE(0 <= partitionId &&
                         partitionId < static_cast<int>(d_fileStores.size()));

        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                         d_clusterData_p->membership().selfNode());

        d_nodeToSeqNumMapPartitionVec[partitionId][cit->source()] = seqNum;
    }
}

void StorageManager::do_storePartitionInfo(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData      = eventDataVec[0];
    int                          partitionId    = eventData.partitionId();
    mqbnet::ClusterNode*         primaryNode    = eventData.primary();
    unsigned int                 primaryLeaseId = eventData.primaryLeaseId();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    pinfo.setPrimary(primaryNode);
    pinfo.setPrimaryLeaseId(primaryLeaseId);
}

void StorageManager::do_clearPartitionInfo(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;
    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();
    mqbnet::ClusterNode*         primaryNode = eventData.primary();
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbs::FileStore* fs    = d_fileStores[partitionId].get();
    PartitionInfo&   pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(fs->inDispatcherThread());

    StorageUtil::clearPrimaryForPartition(fs,
                                          &pinfo,
                                          *d_clusterData_p,
                                          partitionId,
                                          primaryNode);
}

void StorageManager::do_replicaStateRequest(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(
        d_nodeToSeqNumMapPartitionVec[partitionId].find(selfNode) !=
        d_nodeToSeqNumMapPartitionVec[partitionId].end());

    replicaStateRequest.sequenceNumber() =
        d_nodeToSeqNumMapPartitionVec[partitionId][selfNode];

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
    // executed by the *DISPATCHER* thread

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

    response.partitionId()    = partitionId;
    response.sequenceNumber() = d_nodeToSeqNumMapPartitionVec
        [partitionId][d_clusterData_p->membership().selfNode()];

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      eventData.source());

    BSLS_ASSERT_SAFE(eventData.source() ==
                     d_partitionInfoVec[partitionId].primary());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to ReplicaStateRequest from primary node "
                  << eventData.source()->nodeDescription();
}

void StorageManager::do_failureReplicaStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      eventData.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Self not replica! Sent failure response " << controlMsg
                  << " to ReplicaStateRequest from node claiming to be primary"
                  << eventData.source()->nodeDescription()
                  << "for partitionId [" << partitionId << "]";
}

void StorageManager::do_logFailureReplicaStateResponse(
    const PartitionFSMArgsSp& args)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    for (EventDataCIter cit = eventDataVec.cbegin();
         cit != eventDataVec.cend();
         cit++) {
        int                        partitionId = cit->partitionId();
        const mqbnet::ClusterNode* sourceNode  = cit->source();

        BSLS_ASSERT_SAFE(0 <= partitionId &&
                         partitionId < static_cast<int>(d_fileStores.size()));

        BALL_LOG_WARN << d_clusterData_p->identity().description()
                      << ": Received failure ReplicaStateResponse from node "
                      << sourceNode->nodeDescription() << "for partitionId ["
                      << partitionId << "]";
    }
}

void StorageManager::do_logFailurePrimaryStateResponse(
    const PartitionFSMArgsSp& args)
{
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
                  << ": Received failure PrimaryStateResponse from node "
                  << sourceNode->nodeDescription() << " for partitionId ["
                  << partitionId << "]";
}

void StorageManager::do_primaryStateRequest(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    primaryStateRequest.partitionId()    = partitionId;
    primaryStateRequest.sequenceNumber() = d_nodeToSeqNumMapPartitionVec
        [partitionId][d_clusterData_p->membership().selfNode()];

    mqbnet::ClusterNode* destNode = eventData.primary();

    BSLS_ASSERT_SAFE(destNode);

    request->setResponseCb(
        bdlf::BindUtil::bind(&StorageManager::processPrimaryStateResponse,
                             this,
                             bdlf::PlaceHolders::_1,
                             destNode));

    bmqt::GenericResult::Enum status = d_clusterData_p->cluster()->sendRequest(
        request,
        destNode,
        bsls::TimeInterval(10));

    if (bmqt::GenericResult::e_SUCCESS != status) {
        EventData failedEventDataVec;
        failedEventDataVec.emplace_back(destNode,
                                        -1,  // placeholder responseId
                                        partitionId);

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_PRIMARY_STATE_RSPN,
            failedEventDataVec);
    }
}

void StorageManager::do_primaryStateResponse(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    bmqp_ctrlmsg::PrimaryStateResponse& response =
        partitionMessage.choice().makePrimaryStateResponse();

    response.partitionId()    = partitionId;
    response.sequenceNumber() = d_nodeToSeqNumMapPartitionVec
        [partitionId][d_clusterData_p->membership().selfNode()];

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      eventData.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": sent response " << controlMsg
                  << " to PrimaryStateRequest from replica node "
                  << eventData.source()->nodeDescription();
}

void StorageManager::do_failurePrimaryStateResponse(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode() !=
                     d_partitionInfoVec[partitionId].primary());
    BSLS_ASSERT_SAFE(!d_partitionFSMVec[partitionId]->isSelfPrimary());
    // Primary should never send an explicit failure PrimaryStateResponse.

    bmqp_ctrlmsg::ControlMessage controlMsg;
    controlMsg.rId() = eventData.requestId();

    bmqp_ctrlmsg::Status& response = controlMsg.choice().makeStatus();
    response.category()            = bmqp_ctrlmsg::StatusCategory::E_REFUSED;
    response.code()                = mqbi::ClusterErrorCode::e_NOT_PRIMARY;
    response.message()             = "Not a primary";

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      eventData.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Self not primary! Sent failure response " << controlMsg
                  << " to PrimaryStateRequest from node who thinks otherwise "
                  << eventData.source()->nodeDescription()
                  << "for partitionId [" << partitionId << "]";
}

void StorageManager::do_replicaDataRequestPush(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(d_partitionFSMVec.at(partitionId)->isSelfPrimary());

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();

    const NodeToSeqNumMap& nodeToSeqNumMap =
        d_nodeToSeqNumMapPartitionVec[partitionId];
    BSLS_ASSERT_SAFE(nodeToSeqNumMap.find(selfNode) != nodeToSeqNumMap.end());
    const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum =
        nodeToSeqNumMap.at(selfNode);

    // Determine the outdated replicas
    ClusterNodeVec outdatedReplicas;
    for (NodeToSeqNumMapCIter cit = nodeToSeqNumMap.cbegin();
         cit != nodeToSeqNumMap.cend();
         cit++) {
        if (cit->second < selfSeqNum) {
            outdatedReplicas.emplace_back(cit->first);
        }
    }

    // Send ReplicaDataRequestPush to outdated replicas
    EventData failedEventDataVec;
    for (ClusterNodeVecCIter cit = outdatedReplicas.cbegin();
         cit != outdatedReplicas.cend();
         ++cit) {
        mqbnet::ClusterNode* destNode = *cit;
        BSLS_ASSERT_SAFE(destNode != d_clusterData_p->membership().selfNode());

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
        replicaDataRqst.partitionId()         = partitionId;
        replicaDataRqst.beginSequenceNumber() = nodeToSeqNumMap.at(destNode);
        replicaDataRqst.endSequenceNumber()   = selfSeqNum;

        request->setResponseCb(
            bdlf::BindUtil::bind(&StorageManager::processReplicaDataResponse,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 destNode));

        const bmqt::GenericResult::Enum status =
            d_clusterData_p->cluster()->sendRequest(request,
                                                    destNode,
                                                    bsls::TimeInterval(10));

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Sent ReplicaDataRequestPush: " << replicaDataRqst
                      << " to " << destNode->nodeDescription()
                      << " for Partition [" << partitionId << "].";

        if (bmqt::GenericResult::e_SUCCESS != status) {
            failedEventDataVec.emplace_back(destNode,
                                            -1,  // placeholder responseId
                                            partitionId);
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
    // executed by the *DISPATCHER* thread

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

    BSLS_ASSERT_SAFE(destNode);
    BSLS_ASSERT_SAFE(destNode == d_partitionInfoVec[partitionId].primary());

    bmqp_ctrlmsg::ControlMessage controlMsg;
    d_recoveryManager_mp->loadReplicaDataResponsePush(&controlMsg,
                                                      partitionId);

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg, destNode);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Sent response " << controlMsg
                  << " to ReplicaDataRequestPush from primary node "
                  << destNode->nodeDescription() << " for Partition ["
                  << partitionId << "].";
}

void StorageManager::do_replicaDataRequestDrop(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(d_partitionFSMVec.at(partitionId)->isSelfPrimary());

    mqbnet::ClusterNode* selfNode = d_clusterData_p->membership().selfNode();

    const NodeToSeqNumMap& nodeToSeqNumMap =
        d_nodeToSeqNumMapPartitionVec[partitionId];
    BSLS_ASSERT_SAFE(nodeToSeqNumMap.find(selfNode) != nodeToSeqNumMap.end());
    const bmqp_ctrlmsg::PartitionSequenceNumber& selfSeqNum =
        nodeToSeqNumMap.at(selfNode);

    // Determine the replicas with obsolete data to be dropped
    ClusterNodeVec obsoleteDataReplicas;
    for (NodeToSeqNumMapCIter cit = nodeToSeqNumMap.cbegin();
         cit != nodeToSeqNumMap.cend();
         cit++) {
        if (cit->second > selfSeqNum) {
            obsoleteDataReplicas.emplace_back(cit->first);
        }
    }

    // Send ReplicaDataRequestDrop to replicas with obsolete data
    EventData failedEventDataVec;
    for (ClusterNodeVecCIter cit = obsoleteDataReplicas.cbegin();
         cit != obsoleteDataReplicas.cend();
         ++cit) {
        mqbnet::ClusterNode* destNode = *cit;
        BSLS_ASSERT_SAFE(destNode != d_clusterData_p->membership().selfNode());

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
            d_clusterData_p->cluster()->sendRequest(request,
                                                    destNode,
                                                    bsls::TimeInterval(10));

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": Sent ReplicaDataRequestDrop: " << replicaDataRqst
                      << " to " << destNode->nodeDescription()
                      << " for Partition [" << partitionId << "].";

        if (bmqt::GenericResult::e_SUCCESS != status) {
            failedEventDataVec.emplace_back(destNode,
                                            -1,  // placeholder responseId
                                            partitionId);
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
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(destNode != d_clusterData_p->membership().selfNode());

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
    replicaDataRequest.partitionId()         = partitionId;
    replicaDataRequest.beginSequenceNumber() = d_nodeToSeqNumMapPartitionVec
        [partitionId][d_clusterData_p->membership().selfNode()];
    replicaDataRequest.endSequenceNumber() =
        d_nodeToSeqNumMapPartitionVec[partitionId][destNode];

    request->setResponseCb(
        bdlf::BindUtil::bind(&StorageManager::processReplicaDataResponse,
                             this,
                             bdlf::PlaceHolders::_1,
                             destNode));

    bmqt::GenericResult::Enum status = d_clusterData_p->cluster()->sendRequest(
        request,
        destNode,
        bsls::TimeInterval(10));

    if (bmqt::GenericResult::e_SUCCESS != status) {
        EventData failedEventDataVec;
        failedEventDataVec.emplace_back(destNode,
                                        -1,  // placeholder responseId
                                        partitionId);

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_FAIL_REPLICA_DATA_RSPN_PULL,
            failedEventDataVec);
    }
}

void StorageManager::do_replicaDataResponsePull(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    d_clusterData_p->messageTransmitter().sendMessage(controlMsg,
                                                      eventData.source());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Sent response " << controlMsg
                  << " to ReplicaDataRequestPull from primary node "
                  << destNode->nodeDescription() << " for Partition ["
                  << partitionId << "].";
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
    // executed by the *DISPATCHER* thread

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

    bool skipAlarm = partitionHealthState(partitionId) ==
                     PartitionFSM::State::e_UNKNOWN;
    if (!StorageUtil::validateStorageEvent(rawEvent,
                                           partitionId,
                                           source,
                                           d_clusterState,
                                           *d_clusterData_p,
                                           skipAlarm)) {
        return;  // RETURN
    }

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(pinfo.primary() == source);
    BSLS_ASSERT_SAFE(pinfo.primaryStatus() ==
                     bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);
    (void)pinfo;  // silence compiler warning

    d_recoveryManager_mp->bufferStorageEvent(partitionId,
                                             eventData.storageEvent(),
                                             source);
}

void StorageManager::do_processBufferedLiveData(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode* primary = d_partitionInfoVec[partitionId].primary();

    bsl::vector<bsl::shared_ptr<bdlbb::Blob> > bufferedStorageEvents;
    int rc = d_recoveryManager_mp->loadBufferedStorageEvents(
        &bufferedStorageEvents,
        primary,
        partitionId);
    if (rc != 0) {
        return;  // RETURN
    }

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": PartitionId [" << partitionId
                  << "] opened successfully, applying "
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
            MWCTSK_ALARMLOG_ALARM("RECOVERY")
                << d_clusterData_p->identity().description()
                << ": PartitionId [" << partitionId
                << "] failed to apply buffered storage event, rc: " << rc
                << ". Closing the partition." << MWCTSK_ALARMLOG_END;
            fs->close();

            EventData eventDataVecLocal;
            eventDataVecLocal.emplace_back(primary,
                                           -1,  // placeholder requestId
                                           partitionId);

            args->eventsQueue()->emplace(
                PartitionFSM::Event::e_ISSUE_LIVESTREAM,
                eventDataVecLocal);
            break;  // BREAK
        }
    }
}

void StorageManager::do_processLiveData(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    bool skipAlarm = partitionHealthState(partitionId) ==
                     PartitionFSM::State::e_UNKNOWN;
    if (!StorageUtil::validateStorageEvent(rawEvent,
                                           partitionId,
                                           source,
                                           d_clusterState,
                                           *d_clusterData_p,
                                           skipAlarm)) {
        return;  // RETURN
    }

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    BSLS_ASSERT_SAFE(pinfo.primary() == source);
    BSLS_ASSERT_SAFE(pinfo.primaryStatus() ==
                     bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE);
    (void)pinfo;  // silence compiler warning

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
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    mqbc::StorageUtil::cleanSeqNums(
        d_partitionInfoVec[partitionId],
        d_nodeToSeqNumMapPartitionVec[partitionId]);
}

void StorageManager::do_startSendDataChunks(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    // Note that 'eventData.partitionSeqNumDataRange()' is only used when this
    // action is performed by the replica.  If self is primary, we use
    // `d_nodeToSeqNumMapPartitionVec` to determine data range for each replica
    // instead.
    bsl::function<void(int, mqbnet::ClusterNode*, int)> f =
        bdlf::BindUtil::bind(&StorageManager::onPartitionDoneSendDataChunksCb,
                             this,
                             bdlf::PlaceHolders::_1,  // partitionId
                             eventData.requestId(),
                             eventData.partitionSeqNumDataRange(),
                             bdlf::PlaceHolders::_2,   // source
                             bdlf::PlaceHolders::_3);  // status

    if (eventWithData.first == PartitionFSM::Event::e_REPLICA_DATA_RQST_PULL) {
        // Self Replica is sending data to Destination Primary.

        mqbnet::ClusterNode* destNode = eventData.source();
        BSLS_ASSERT_SAFE(destNode != d_clusterData_p->membership().selfNode());
        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                         eventData.source());

        bmqp_ctrlmsg::PartitionSequenceNumber beginSeqNum =
            eventData.partitionSeqNumDataRange().first;
        bmqp_ctrlmsg::PartitionSequenceNumber endSeqNum =
            eventData.partitionSeqNumDataRange().second;
        BSLS_ASSERT_SAFE(
            endSeqNum ==
            d_nodeToSeqNumMapPartitionVec
                [partitionId][d_clusterData_p->membership().selfNode()]);

        d_recoveryManager_mp->processSendDataChunks(
            partitionId,
            destNode,
            beginSeqNum,
            endSeqNum,
            *(d_fileStores[partitionId].get()),
            f);
    }
    else if (eventWithData.first ==
                 PartitionFSM::Event::e_PRIMARY_STATE_RQST ||
             eventWithData.first ==
                 PartitionFSM::Event::e_REPLICA_STATE_RSPN) {
        // Self Primary is sending data to Destination replica.

        mqbnet::ClusterNode* destNode = eventData.source();
        BSLS_ASSERT_SAFE(destNode != d_clusterData_p->membership().selfNode());
        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                         d_clusterData_p->membership().selfNode());

        const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum =
            d_nodeToSeqNumMapPartitionVec[partitionId][destNode];
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum =
            d_nodeToSeqNumMapPartitionVec
                [partitionId][d_clusterData_p->membership().selfNode()];
        if (beginSeqNum >= endSeqNum) {
            return;  // RETURN
        }

        d_recoveryManager_mp->processSendDataChunks(
            partitionId,
            destNode,
            beginSeqNum,
            endSeqNum,
            *(d_fileStores[partitionId].get()),
            f);
    }
    else {
        // Self Primary is sending latest data to all outdated replicas.

        NodeToSeqNumMap& nodeToSeqNumMap =
            d_nodeToSeqNumMapPartitionVec[partitionId];

        BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                         d_clusterData_p->membership().selfNode());

        // End Sequence number is primary's latest sequence number.
        const bmqp_ctrlmsg::PartitionSequenceNumber& endSeqNum =
            nodeToSeqNumMap[d_clusterData_p->membership().selfNode()];

        for (NodeToSeqNumMap::const_iterator cit = nodeToSeqNumMap.cbegin();
             cit != nodeToSeqNumMap.cend();
             cit++) {
            if (cit->first == d_clusterData_p->membership().selfNode()) {
                continue;
            }
            mqbnet::ClusterNode*                         destNode = cit->first;
            const bmqp_ctrlmsg::PartitionSequenceNumber& beginSeqNum =
                nodeToSeqNumMap[destNode];

            if (beginSeqNum >= endSeqNum) {
                // Two cases:
                // 1. Replica in-sync with primary: no need to send data chunks
                // 2. Replica is ahead: we already sent ReplicaDataRequestDrop
                continue;
            }

            d_recoveryManager_mp->processSendDataChunks(
                partitionId,
                destNode,
                beginSeqNum,
                endSeqNum,
                *(d_fileStores[partitionId].get()),
                f);
        }
    }
}

void StorageManager::do_setExpectedDataChunkRange(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();
    mqbnet::ClusterNode* highestSeqNumNode   = eventData.highestSeqNumNode();

    int rc = 0;
    if (eventWithData.first == PartitionFSM::Event::e_REPLICA_HIGHEST_SEQ) {
        // Self Primary is expecting data from highest seq num replica.

        const bmqp_ctrlmsg::PartitionSequenceNumber selfSeqNum =
            d_nodeToSeqNumMapPartitionVec.at(partitionId)
                .at(d_clusterData_p->membership().selfNode());

        rc = d_recoveryManager_mp->setExpectedDataChunkRange(
            partitionId,
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
                                         partitionId);
            args->eventsQueue()->emplace(
                PartitionFSM::Event::e_DONE_RECEIVING_DATA_CHUNKS,
                eventDataVecOut);
        }

        rc = d_recoveryManager_mp->setExpectedDataChunkRange(
            partitionId,
            highestSeqNumNode,
            eventData.partitionSeqNumDataRange().first,
            eventData.partitionSeqNumDataRange().second,
            eventData.requestId());
    }

    if (rc != 0) {
        EventData eventDataVecOut;
        eventDataVecOut.emplace_back(highestSeqNumNode,
                                     eventData.requestId(),
                                     partitionId);
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_ERROR_RECEIVING_DATA_CHUNKS,
            eventDataVecOut);
    }
}

void StorageManager::do_resetReceiveDataCtx(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    // executed by the *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() == 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    const int                    partitionId = eventData.partitionId();

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);
    BSLS_ASSERT_SAFE(!fs->isOpen());
    BSLS_ASSERT_SAFE(fs->config().partitionId() == partitionId);

    const int rc = fs->open(d_queueKeyInfoMap);
    if (0 != rc) {
        MWCTSK_ALARMLOG_ALARM("FILE_IO")
            << d_clusterData_p->identity().description()
            << ": Failed to open PartitionId [" << partitionId
            << "] after recovery was finished, rc: " << rc
            << MWCTSK_ALARMLOG_END;
    }
}

void StorageManager::do_updateStorage(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    if (!StorageUtil::validatePartitionSyncEvent(rawEvent,
                                                 partitionId,
                                                 source,
                                                 d_clusterState,
                                                 *d_clusterData_p,
                                                 true)) {  // isFSMWorkflow
        return;                                            // RETURN
    }

    const PartitionInfo& pinfo = d_partitionInfoVec[partitionId];
    if (pinfo.primary() == d_clusterData_p->membership().selfNode()) {
        // If self is primary for this partition, self must be passive.

        if (bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE != pinfo.primaryStatus()) {
            BALL_LOG_ERROR << d_clusterData_p->identity().description()
                           << " PartitionId [" << partitionId
                           << "]: received a partition sync event from: "
                           << source->nodeDescription()
                           << ", while self is ACTIVE replica.";
            return;  // RETURN
        }
    }
    else if (pinfo.primary() != source) {
        BALL_LOG_ERROR << d_clusterData_p->identity().description()
                       << " PartitionId [" << partitionId
                       << "]: received a partition sync event from: "
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
                  << ": Received status " << rc
                  << " for Receiving Data Chunks from Recovery Manager for"
                  << " partitionId [" << partitionId << "]";

    EventData eventDataVecOut;
    eventDataVecOut.emplace_back(source,
                                 -1,  // placeholder requestId
                                 partitionId);

    // As replica, we will send either success or failure
    // ReplicaDataResponsePush/Drop depending on 'rc'.
    if (rc != 0) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_ERROR_RECEIVING_DATA_CHUNKS,
            eventDataVecOut);
    }
    else if (rc == 1) {
        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_DONE_RECEIVING_DATA_CHUNKS,
            eventDataVecOut);
    }
    // Note: 'rc == 0' means that we have received some data chunks, but more
    // is coming, and there is no need to trigger FSM event.
}

void StorageManager::do_removeStorage(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    MWCTSK_ALARMLOG_ALARM("REPLICATION")
        << d_clusterData_p->identity().description() << " Partition ["
        << partitionId << "]: "
        << "self's storage is out of sync with primary and cannot be healed "
        << "trivially. Removing entire storage and aborting broker."
        << MWCTSK_ALARMLOG_END;

    mqbu::ExitUtil::terminate(mqbu::ExitCode::e_STORAGE_OUT_OF_SYNC);  // EXIT
}

void StorageManager::do_checkQuorumRplcaDataRspn(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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

    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());

    if (++d_numReplicaDataResponsesReceivedVec[partitionId] + 1 >=
        d_seqNumQuorum) {
        // If we have a quorum of replica data responses (+1 to include self)
        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": For partition [" << partitionId << "]"
                      << " A quorum ("
                      << d_numReplicaDataResponsesReceivedVec[partitionId] + 1
                      << ") of cluster nodes now has healed partitions. "
                      << "Transitiong self to healed primary";

        args->eventsQueue()->emplace(
            PartitionFSM::Event::e_QUORUM_REPLICA_DATA_RSPN,
            eventWithData.second);
    }
}

void StorageManager::do_clearRplcaDataRspnCnt(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->state() ==
                     PartitionFSM::State::e_UNKNOWN);

    d_numReplicaDataResponsesReceivedVec[partitionId] = 0;
}

void StorageManager::do_reapplyEvent(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());

    if (d_nodeToSeqNumMapPartitionVec[partitionId].size() >= d_seqNumQuorum) {
        // If we have a quorum of Replica Sequence numbers (including self Seq)

        BALL_LOG_INFO << d_clusterData_p->identity().description()
                      << ": For partition [" << partitionId << "]"
                      << " Achieved a quorum of SeqNums with a count of "
                      << d_nodeToSeqNumMapPartitionVec[partitionId].size();

        args->eventsQueue()->emplace(PartitionFSM::Event::e_QUORUM_REPLICA_SEQ,
                                     eventWithData.second);
    }
}

void StorageManager::do_findHighestSeq(const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!args->eventsQueue()->empty());

    const PartitionFSM::EventWithData& eventWithData =
        args->eventsQueue()->front();
    const EventData& eventDataVec = eventWithData.second;

    BSLS_ASSERT_SAFE(eventDataVec.size() >= 1);

    const PartitionFSMEventData& eventData   = eventDataVec[0];
    int                          partitionId = eventData.partitionId();

    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_partitionFSMVec[partitionId]->isSelfPrimary());
    BSLS_ASSERT_SAFE(d_nodeToSeqNumMapPartitionVec[partitionId].size() >=
                     d_seqNumQuorum);

    const NodeToSeqNumMap& nodeToSeqNumMap =
        d_nodeToSeqNumMapPartitionVec[partitionId];

    // Initialize highest sequence number with self/primary sequence number.
    mqbnet::ClusterNode* highestSeqNumNode =
        d_clusterData_p->membership().selfNode();
    bmqp_ctrlmsg::PartitionSequenceNumber highestPartitionSeqNum(
        nodeToSeqNumMap.find(highestSeqNumNode)->second);

    // Check for highest sequence number and update accordingly.
    for (NodeToSeqNumMapCIter cit = nodeToSeqNumMap.cbegin();
         cit != nodeToSeqNumMap.cend();
         cit++) {
        mqbnet::ClusterNode*                  node            = cit->first;
        bmqp_ctrlmsg::PartitionSequenceNumber partitionSeqNum = cit->second;

        if (partitionSeqNum > highestPartitionSeqNum) {
            highestSeqNumNode      = node;
            highestPartitionSeqNum = partitionSeqNum;
        }
    }

    EventData newEventDataVec;

    BSLS_ASSERT_SAFE(d_partitionInfoVec[partitionId].primary() ==
                     d_clusterData_p->membership().selfNode());

    const unsigned int primaryLeaseId =
        d_partitionInfoVec[partitionId].primaryLeaseId();

    newEventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                                 -1,  // placeholder requestId
                                 partitionId,
                                 d_clusterData_p->membership().selfNode(),
                                 primaryLeaseId,
                                 highestPartitionSeqNum,
                                 highestSeqNumNode);

    if (highestSeqNumNode == d_clusterData_p->membership().selfNode()) {
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
    // executed by the *DISPATCHER* thread

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
    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode() ==
                     d_partitionInfoVec[partitionId].primary());
    BSLS_ASSERT_SAFE(d_clusterData_p->membership().selfNode() !=
                     eventData.source());

    d_nodeToSeqNumMapPartitionVec[partitionId].erase(eventData.source());
}

void StorageManager::do_reapplyDetectSelfPrimary(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread
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
        d_partitionInfoVec[partitionId].primary(),
        d_partitionInfoVec[partitionId].primaryLeaseId());
    args->eventsQueue()->emplace(PartitionFSM::Event::e_DETECT_SELF_PRIMARY,
                                 eventDataVecOut);
}

void StorageManager::do_reapplyDetectSelfReplica(
    const PartitionFSMArgsSp& args)
{
    // executed by the *DISPATCHER* thread

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
        d_partitionInfoVec[partitionId].primary(),
        d_partitionInfoVec[partitionId].primaryLeaseId());
    args->eventsQueue()->emplace(PartitionFSM::Event::e_DETECT_SELF_REPLICA,
                                 eventDataVecOut);
}

// CREATORS
StorageManager::StorageManager(const mqbcfg::ClusterDefinition& clusterConfig,
                               mqbi::Cluster*                   cluster,
                               mqbc::ClusterData*               clusterData,
                               const mqbc::ClusterState&        clusterState,
                               mqbi::DomainFactory*             domainFactory,
                               PartitionFSMObserver*            fsmObserver,
                               mqbi::Dispatcher*                dispatcher,
                               bsls::Types::Int64      watchDogTimeoutDuration,
                               const RecoveryStatusCb& recoveryStatusCb,
                               bslma::Allocator*       allocator)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_watchDogEventHandles(allocator)
, d_watchDogTimeoutInterval(watchDogTimeoutDuration)
, d_lowDiskspaceWarning(false)
, d_blobSpPool_p(clusterData->blobSpPool())
, d_domainFactory_p(domainFactory)
, d_dispatcher_p(dispatcher)
, d_cluster_p(cluster)
, d_clusterData_p(clusterData)
, d_clusterState(clusterState)
, d_clusterConfig(clusterConfig)
, d_fileStores(allocator)
, d_miscWorkThreadPool(1, 100, allocator)
, d_recoveryStatusCb(recoveryStatusCb)
, d_storagesLock()
, d_storages(allocator)
, d_appKeysLock()
, d_appKeysVec(allocator)
, d_partitionInfoVec(allocator)
, d_partitionFSMVec(allocator)
, d_partitionFSMObserver_p(fsmObserver)
, d_numPartitionsRecoveredFully(0)
, d_recoveryStartTimes(allocator)
, d_nodeToSeqNumMapPartitionVec(allocator)
, d_seqNumQuorum((d_clusterConfig.nodes().size() / 2) + 1)  // TODO: Config??
, d_numReplicaDataResponsesReceivedVec(allocator)
, d_queueKeyInfoMap(allocator)
, d_minimumRequiredDiskSpace(0)
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
    BSLS_ASSERT_SAFE(d_partitionFSMObserver_p);

    const mqbcfg::PartitionConfig& partitionCfg =
        d_clusterConfig.partitionConfig();

    d_watchDogEventHandles.resize(partitionCfg.numPartitions());
    d_fileStores.resize(partitionCfg.numPartitions());
    d_storages.resize(partitionCfg.numPartitions());
    d_appKeysVec.resize(partitionCfg.numPartitions());
    d_partitionInfoVec.resize(partitionCfg.numPartitions());
    d_recoveryStartTimes.resize(partitionCfg.numPartitions());
    d_nodeToSeqNumMapPartitionVec.resize(partitionCfg.numPartitions());
    d_numReplicaDataResponsesReceivedVec.resize(partitionCfg.numPartitions());

    for (int p = 0; p < partitionCfg.numPartitions(); p++) {
        d_partitionFSMVec.emplace_back(new (*allocator)
                                           PartitionFSM(*this, allocator),
                                       allocator);
        d_partitionFSMVec.back()->registerObserver(this);
        d_partitionFSMVec.back()->registerObserver(d_partitionFSMObserver_p);
    }

    d_minimumRequiredDiskSpace = StorageUtil::findMinReqDiskSpace(
        partitionCfg);

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

    StorageUtil::transitionToActivePrimary(&d_partitionInfoVec[partitionId],
                                           d_clusterData_p,
                                           partitionId);

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
    d_clusterData_p->scheduler()->scheduleRecurringEvent(
        &d_storageMonitorEventHandle,
        bsls::TimeInterval(bdlt::TimeUnitRatio::k_SECONDS_PER_MINUTE),
        bdlf::BindUtil::bind(&StorageUtil::storageMonitorCb,
                             &d_lowDiskspaceWarning,
                             &d_isStarted,
                             d_minimumRequiredDiskSpace,
                             d_clusterData_p->identity().description(),
                             d_clusterConfig.partitionConfig()));

    rc = StorageUtil::assignPartitionDispatcherThreads(&d_miscWorkThreadPool,
                                                       d_clusterData_p,
                                                       *d_cluster_p,
                                                       d_dispatcher_p,
                                                       partitionCfg,
                                                       &d_fileStores,
                                                       d_blobSpPool_p,
                                                       &d_allocators,
                                                       errorDescription,
                                                       d_replicationFactor);
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

    // Get named allocator from associated mwcma::CountingAllocatorStore
    bslma::Allocator* recoveryManagerAllocator = d_allocators.get(
        "RecoveryManager");

    d_recoveryManager_mp.load(new (*recoveryManagerAllocator)
                                  RecoveryManager(d_clusterConfig,
                                                  d_clusterData_p,
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
                                  pid);

        dispatchEventToPartition(d_fileStores[pid].get(),
                                 PartitionFSM::Event::e_RST_UNKNOWN,
                                 eventDataVec);
    }

    for (int p = 0; p < d_clusterConfig.partitionConfig().numPartitions();
         p++) {
        d_partitionFSMVec[p]->unregisterObserver(this);
        d_partitionFSMVec[p]->unregisterObserver(d_partitionFSMObserver_p);
    }

    d_clusterData_p->scheduler()->cancelEventAndWait(&d_gcMessagesEventHandle);
    d_clusterData_p->scheduler()->cancelEventAndWait(
        &d_storageMonitorEventHandle);
    d_recoveryManager_mp->stop();

    StorageUtil::stop(
        d_clusterData_p,
        &d_fileStores,
        bdlf::BindUtil::bind(&StorageManager::shutdownCb,
                             this,
                             bdlf::PlaceHolders::_1,    // partitionId
                             bdlf::PlaceHolders::_2));  // latch
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
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
        .setCallback(
            bdlf::BindUtil::bind(&StorageUtil::unregisterQueueDispatched,
                                 bdlf::PlaceHolders::_1,  // processor
                                 d_fileStores[partitionId].get(),
                                 &d_storages[partitionId],
                                 &d_storagesLock,
                                 d_clusterData_p,
                                 partitionId,
                                 d_partitionInfoVec[partitionId],
                                 uri));

    d_fileStores[partitionId]->dispatchEvent(queueEvent);
}

int StorageManager::updateQueue(const bmqt::Uri&        uri,
                                const mqbu::StorageKey& queueKey,
                                int                     partitionId,
                                const AppIdKeyPairs&    addedIdKeyPairs,
                                const AppIdKeyPairs&    removedIdKeyPairs)
{
    // executed by *QUEUE_DISPATCHER* thread with the specified 'partitionId'

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(d_fileStores[partitionId]->inDispatcherThread());

    return StorageUtil::updateQueue(&d_storages[partitionId],
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
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
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
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
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
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
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
        .setType(mqbi::DispatcherEventType::e_DISPATCHER)
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
    ;
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
                                            mqbnet::ClusterNode* replicaNode,
                                            unsigned int replicaLeaseId)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(replicaNode);

    if (replicaNode->nodeId() ==
        d_clusterData_p->membership().selfNode()->nodeId()) {
        processPrimaryDetect(partitionId, replicaNode, replicaLeaseId);
    }
    else {
        processReplicaDetect(partitionId, replicaNode, replicaLeaseId);
    }
}

void StorageManager::clearPrimaryForPartition(int                  partitionId,
                                              mqbnet::ClusterNode* replica)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));
    BSLS_ASSERT_SAFE(replica);

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Self Transition back to Unknown for partitionId "
                  << partitionId << " in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
                              replica);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_RST_UNKNOWN,
                             eventDataVec);
}

// MANIPULATORS
void StorageManager::processPrimaryDetect(int                  partitionId,
                                          mqbnet::ClusterNode* primaryNode,
                                          unsigned int         primaryLeaseId)
{
    // executed by the cluster *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(d_cluster_p));
    BSLS_ASSERT_SAFE(primaryNode == d_clusterData_p->membership().selfNode());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Self Transition to Primary for partitionId "
                  << partitionId << " in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
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
    BSLS_ASSERT_SAFE(primaryNode != d_clusterData_p->membership().selfNode());
    BSLS_ASSERT_SAFE(0 <= partitionId &&
                     partitionId < static_cast<int>(d_fileStores.size()));

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Self Transition to Replica for partitionId "
                  << partitionId << " in the Partition FSM.";

    EventData eventDataVec;
    eventDataVec.emplace_back(d_clusterData_p->membership().selfNode(),
                              -1,  // placeholder requestId
                              partitionId,
                              primaryNode,
                              primaryLeaseId);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_DETECT_SELF_REPLICA,
                             eventDataVec);
}

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
                  << ": Received Primary State Request: " << message
                  << " from " << source->nodeDescription()
                  << " for partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(source,
                              message.rId().value(),
                              partitionId,
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
                  << ": Received Replica State Request: " << message
                  << " from " << source->nodeDescription()
                  << " for partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(source,
                              message.rId().value(),
                              partitionId,
                              replicaStateRequest.sequenceNumber());

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_STATE_RQST,
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
                  << ": Received Replica Data Request PULL: " << message
                  << " from " << source->nodeDescription()
                  << " for partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(
        source,
        message.rId().value(),
        partitionId,
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
    BSLS_ASSERT_SAFE(source == d_partitionInfoVec[partitionId].primary());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received Replica Data Request PUSH: " << message
                  << " from " << source->nodeDescription()
                  << " for partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(
        source,
        message.rId().value(),
        partitionId,
        source,
        d_partitionInfoVec[partitionId].primaryLeaseId(),
        d_nodeToSeqNumMapPartitionVec[partitionId][source],
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
    BSLS_ASSERT_SAFE(source == d_partitionInfoVec[partitionId].primary());

    BALL_LOG_INFO << d_clusterData_p->identity().description()
                  << ": Received Replica Data Request DROP: " << message
                  << " from " << source->nodeDescription()
                  << " for partitionId [" << partitionId << "]";

    EventData eventDataVec;
    eventDataVec.emplace_back(source, message.rId().value(), partitionId);

    mqbs::FileStore* fs = d_fileStores[partitionId].get();
    BSLS_ASSERT_SAFE(fs);

    dispatchEventToPartition(fs,
                             PartitionFSM::Event::e_REPLICA_DATA_RQST_DROP,
                             eventDataVec);
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
    BSLS_ASSERT_SAFE(!d_clusterData_p->cluster()->isLocal());
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

    // Ensure that 'pid' is valid.
    if (static_cast<unsigned int>(pid) >= d_clusterState.partitions().size()) {
        MWCTSK_ALARMLOG_ALARM("STORAGE")
            << d_cluster_p->description() << ": Received "
            << (rawEvent.isStorageEvent() ? "storage " : "partition-sync ")
            << "event from node " << source->nodeDescription() << " with "
            << "invalid PartitionId [" << pid << "]. Ignoring "
            << "entire storage event." << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }
    BSLS_ASSERT_SAFE(d_fileStores.size() > pid);

    EventData eventDataVec;
    eventDataVec.emplace_back(event.clusterNode(), pid, event.blob());

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

void StorageManager::processReceiptEvent(
    const mqbi::DispatcherReceiptEvent& event)
{
    // executed by *CLUSTER DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_dispatcher_p->inDispatcherThread(d_cluster_p));

    mwcu::BlobPosition position;
    const int          rc = mwcu::BlobUtil::findOffsetSafe(&position,
                                                  *event.blob(),
                                                  sizeof(bmqp::EventHeader));
    BSLS_ASSERT_SAFE(rc == 0);

    mwcu::BlobObjectProxy<bmqp::ReplicationReceipt> receipt(
        event.blob().get(),
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
                                     event.clusterNode()));
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

    mqbs::FileStore* fs = d_fileStores[advisory.partitionId()].get();
    BSLS_ASSERT_SAFE(fs);

    fs->execute(bdlf::BindUtil::bind(
        &StorageUtil::processPrimaryStatusAdvisoryDispatched,
        fs,
        &d_partitionInfoVec[advisory.partitionId()],
        advisory,
        d_clusterData_p->identity().description(),
        source));
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
        &StorageUtil::processReplicaStatusAdvisoryDispatched,
        d_clusterData_p,
        fs,
        partitionId,
        d_partitionInfoVec[partitionId],
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
    // executed by the *DISPATCHER* thread

    const mqbs::FileStore& fs = fileStore(partitionId);

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
        d_dispatcher_p->inDispatcherThread(d_clusterData_p->cluster()));

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
        d_dispatcher_p->inDispatcherThread(d_clusterData_p->cluster()));

    // NOTHING
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

bdlbb::BlobBufferFactory* StorageManager::blobBufferFactory() const
{
    return d_clusterData_p->bufferFactory();
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
