// Copyright 2015-2024 Bloomberg Finance L.P.
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

// mqbblp_cluster.cpp                                                 -*-C++-*-
#include <mqbblp_cluster.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_storagemanager.h>
#include <mqbc_clusterutil.h>
#include <mqbc_storagemanager.h>
#include <mqbc_storageutil.h>
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>
#include <mqbnet_elector.h>
#include <mqbnet_session.h>
#include <mqbs_datastore.h>
#include <mqbs_replicatedstorage.h>

// BMQ
#include <bmqp_ackmessageiterator.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_controlmessageutil.h>
#include <bmqp_event.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqp_rejectmessageiterator.h>
#include <bmqp_storagemessageiterator.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>

#include <bmqex_systemexecutor.h>
#include <bmqst_statcontext.h>
#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>  // for bsl::exit()
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslmt_latch.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const double k_LOG_SUMMARY_INTERVAL = 60.0 * 5;  // 5 minutes

const double k_QUEUE_GC_INTERVAL = 60.0;  // 1 minutes

/// Timeout duration for Partition FSM watchdog -- 5 minutes
const bsls::Types::Int64 k_PARTITION_FSM_WATCHDOG_TIMEOUT_DURATION = 60 * 5;

}  // close unnamed namespace

// -------------------------------------
// struct Cluster::ValidationResult
// -------------------------------------

bsl::ostream&
Cluster::ValidationResult::print(bsl::ostream&                   stream,
                                 Cluster::ValidationResult::Enum value,
                                 int                             level,
                                 int spacesPerLevel)
{
    stream << bmqu::PrintUtil::indent(level, spacesPerLevel)
           << Cluster::ValidationResult::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
Cluster::ValidationResult::toAscii(Cluster::ValidationResult::Enum value)
{
    switch (value) {
    case k_SUCCESS: return "SUCCESS";
    case k_UNKNOWN_QUEUE: return "message for unknown queue";
    case k_UNKNOWN_SUBQUEUE: return "message for unknown subqueue";
    case k_FINAL:
        return "message for which final closeQueue was already received";
    default: return "UNKNOWN";
    }
}

// -------------
// class Cluster
// -------------

// PRIVATE MANIPULATORS
void Cluster::startDispatched(bsl::ostream* errorDescription, int* rc)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "start() can only be called once on this object");

    enum {
        rc_SUCCESS             = 0,
        rc_STATE_MGR_FAILURE   = -1,
        rc_MISC_FAILURE        = -2,
        rc_STORAGE_MGR_FAILURE = -3
    };

    *rc = d_clusterOrchestrator.start(*errorDescription);
    if (0 != *rc) {
        *rc = *rc * 10 + rc_STATE_MGR_FAILURE;
        return;  // RETURN
    }

    d_clusterData.membership().setSelfNodeStatus(
        bmqp_ctrlmsg::NodeStatus::E_STARTING);

    // Send notification to peers about self's nodeStatus.  Notification must
    // be sent *before* starting the storageMgr.  Once storageMgr has completed
    // recovery, node will transition to 'E_AVAILABLE', at which point, another
    // advisory will be sent by the node.

    bmqp_ctrlmsg::ControlMessage  controlMsg;
    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::NodeStatusAdvisory& advisory =
        clusterMsg.choice().makeNodeStatusAdvisory();
    advisory.status() = bmqp_ctrlmsg::NodeStatus::E_STARTING;
    d_clusterData.messageTransmitter().broadcastMessage(controlMsg, true);

    // Start a StatMonitorSnapshotRecorder to track system stats during
    // recovery
    bmqsys::StatMonitorSnapshotRecorder statRecorder(description() + ": ",
                                                     d_allocator_p);

    // Get named allocator from associated bmqma::CountingAllocatorStore
    bslma::Allocator* storageManagerAllocator = d_allocators.get(
        "StorageManager");

    // Make a temporal pointer to the dispatcher to avoid a false-positive
    // "uninitialized variable" warning
    mqbi::Dispatcher* clusterDispatcher = dispatcher();

    // Start the StorageManager
    d_storageManager_mp.load(
        isFSMWorkflow()
            ? static_cast<mqbi::StorageManager*>(
                  new (*storageManagerAllocator) mqbc::StorageManager(
                      d_clusterData.clusterConfig(),
                      this,
                      &d_clusterData,
                      d_state,
                      d_clusterData.domainFactory(),
                      clusterDispatcher,
                      k_PARTITION_FSM_WATCHDOG_TIMEOUT_DURATION,
                      bdlf::BindUtil::bind(&Cluster::onRecoveryStatus,
                                           this,
                                           bdlf::PlaceHolders::_1,  // status
                                           bsl::vector<unsigned int>(),
                                           statRecorder),
                      bdlf::BindUtil::bind(
                          &ClusterOrchestrator::onPartitionPrimaryStatus,
                          &d_clusterOrchestrator,
                          bdlf::PlaceHolders::_1,   // partitionId
                          bdlf::PlaceHolders::_2,   // status
                          bdlf::PlaceHolders::_3),  // primary leaseId
                      storageManagerAllocator))
            : static_cast<mqbi::StorageManager*>(
                  new (*storageManagerAllocator) mqbblp::StorageManager(
                      d_clusterData.clusterConfig(),
                      this,
                      &d_clusterData,
                      d_state,
                      bdlf::BindUtil::bind(
                          &Cluster::onRecoveryStatus,
                          this,
                          bdlf::PlaceHolders::_1,  // status
                          bdlf::PlaceHolders::_2,  // vector<leaseId>
                          statRecorder),
                      bdlf::BindUtil::bind(
                          &ClusterOrchestrator::onPartitionPrimaryStatus,
                          &d_clusterOrchestrator,
                          bdlf::PlaceHolders::_1,   // partitionId
                          bdlf::PlaceHolders::_2,   // status
                          bdlf::PlaceHolders::_3),  // primary leaseId
                      d_clusterData.domainFactory(),
                      clusterDispatcher,
                      &d_clusterData.miscWorkThreadPool(),
                      storageManagerAllocator)),
        storageManagerAllocator);

    // Start the misc work thread pool
    *rc = d_clusterData.miscWorkThreadPool().start();
    if (*rc != 0) {
        d_clusterOrchestrator.stop();
        *rc = *rc * 10 + rc_MISC_FAILURE;
        return;  // RETURN
    }

    *rc = d_storageManager_mp->start(*errorDescription);
    if (*rc != 0) {
        d_clusterOrchestrator.stop();
        *rc = *rc * 10 + rc_STORAGE_MGR_FAILURE;
        return;  // RETURN
    }

    d_clusterOrchestrator.queueHelper().initialize();

    d_clusterOrchestrator.setStorageManager(d_storageManager_mp.get());
    d_clusterOrchestrator.queueHelper().setStorageManager(
        d_storageManager_mp.get());

    d_clusterData.electorInfo().registerObserver(this);
    d_state.registerObserver(this);

    mqbnet::Cluster* netCluster = d_clusterData.membership().netCluster();
    // Register as cluster observer.
    netCluster->registerObserver(this);

    // Iterate over the net cluster and explicitly issue a
    // 'nodeStateChanged' event for those nodes with which a channel is
    // established.  This is needed because a channel may have been already
    // established *before* cluster ('this') registered itself w/ the net
    // cluster, and may have missed the 'nodeStateChanged' event.  Since the
    // node/cluster synchronization relies on the fact that
    // 'nodeStateChanged' will always be fired, this logic is needed.  Note
    // that this also means that 'nodeStateChanged' event is now fired more
    // than once, but this is ok since this event is idempotent.

    mqbnet::Cluster::NodesList& nodes = netCluster->nodes();
    for (mqbnet::Cluster::NodesList::iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        mqbnet::ClusterNode* node = *it;
        if (node->nodeId() != netCluster->selfNodeId() &&
            node->isAvailable()) {
            d_clusterOrchestrator.processNodeStateChangeEvent(node, true);
        }
    }
    // Ready to read.
    netCluster->enableRead();

    d_clusterMonitor.start();
    d_clusterMonitor.registerObserver(this);

    // Start a recurring clock for summary print
    d_clusterData.scheduler().scheduleRecurringEvent(
        &d_logSummarySchedulerHandle,
        bsls::TimeInterval(k_LOG_SUMMARY_INTERVAL),
        bdlf::BindUtil::bind(&Cluster::logSummaryState, this));

    // Start a recurring clock for gc'ing expired queues.

    d_clusterData.scheduler().scheduleRecurringEvent(
        &d_queueGcSchedulerHandle,
        bsls::TimeInterval(k_QUEUE_GC_INTERVAL),
        bdlf::BindUtil::bind(&Cluster::gcExpiredQueues, this));

    d_isStarted = true;

    d_clusterOrchestrator.updateDatumStats();

    const mqbcfg::PartitionConfig& partitionConfig =
        d_clusterData.clusterConfig().partitionConfig();
    d_clusterData.stats().setPartitionCfgBytes(
        partitionConfig.maxDataFileSize(),
        partitionConfig.maxJournalFileSize());

    *rc = rc_SUCCESS;
}

void Cluster::stopDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_clusterMonitor.stop();

    // Cancel recurring events.

    d_clusterData.scheduler().cancelEventAndWait(&d_queueGcSchedulerHandle);
    d_clusterData.scheduler().cancelEventAndWait(&d_logSummarySchedulerHandle);
    // NOTE: The scheduler event does a dispatching to execute 'logSummary'
    //       from the scheduler thread to the dispatcher thread, but there is
    //       no race issue here because stop does a double synchronize, so it's
    //       guaranteed that this object will be kept alive.

    // Teardown all ClusterNodeSession
    for (ClusterNodeSessionMapIter it =
             d_clusterData.membership().clusterNodeSessionMap().begin();
         it != d_clusterData.membership().clusterNodeSessionMap().end();
         ++it) {
        it->second->teardown();
    }

    d_clusterMonitor.unregisterObserver(this);
    d_clusterData.membership().netCluster()->unregisterObserver(this);
    d_state.unregisterObserver(this);
    d_clusterData.electorInfo().unregisterObserver(this);

    d_clusterData.scheduler().cancelEventAndWait(
        d_clusterData.electorInfo().leaderSyncEventHandle());
    // Ignore rc

    d_clusterOrchestrator.queueHelper().teardown();
    d_storageManager_mp->stop();

    d_clusterData.membership().selfNodeSession()->removeAllPartitions();
    // TBD: perform above in initiateShutdownDispatched() ?

    d_clusterOrchestrator.stop();

    d_clusterData.miscWorkThreadPool().stop();

    // Notify peers before going down.  This should be the last message sent
    // out.

    bmqp_ctrlmsg::ControlMessage  controlMsg;
    bmqp_ctrlmsg::ClusterMessage& clusterMsg =
        controlMsg.choice().makeClusterMessage();
    bmqp_ctrlmsg::NodeStatusAdvisory& advisory =
        clusterMsg.choice().makeNodeStatusAdvisory();

    advisory.status() = bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE;

    d_clusterData.membership().setSelfNodeStatus(
        bmqp_ctrlmsg::NodeStatus::E_UNAVAILABLE);
    d_clusterData.messageTransmitter().broadcastMessage(controlMsg, true);

    // Close all channels of the associated cluster: when broadcasting the
    // state change, the leader, in response, will potentially elect new
    // primaries and broadcast new messages.  Once we return from this
    // 'stopDispatched' method, this cluster object gets destroyed, but not the
    // underlying channel, which will be destroyed later; so if a message is
    // received on the channel, it will try to invoke the 'processEvent' method
    // on that now destroyed cluster.  While we should revisit this, closing
    // the channel at this time here is a working *probably temporary* fix.
    //
    // NOTE: channel.close() will not flush the channel's buffer, so there is a
    //       risk that the last message (the broadcast of the e_UNAVAILABLE
    //       state) - or worse, more messages - may not be delivered.

    d_clusterData.membership().netCluster()->closeChannels();
}

void Cluster::sendAck(bmqt::AckResult::Enum     status,
                      int                       correlationId,
                      const bmqt::MessageGUID&  messageGUID,
                      int                       queueId,
                      const bslstl::StringRef&  source,
                      mqbc::ClusterNodeSession* nodeSession,
                      bool                      isSelfGenerated)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(nodeSession);

    // If it's a NACK, do a lookup of the queue from 'queueId' to retrieve the
    // uri, for logging (we don't leverage that the caller might already have
    // that queue looked up, because not all code path calling 'sendAck' have
    // it).
    bmqt::Uri uri;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(status !=
                                              bmqt::AckResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        bool                           found = false;
        QueueHandleMap::const_iterator it = nodeSession->queueHandles().find(
            queueId);
        if (it != nodeSession->queueHandles().end()) {
            // Note that depending upon the condition under which this routine
            // was called, it is possible for the 'queueId' to not exist in the
            // list of queue handles owned by this node session.

            uri   = it->second.d_handle_p->queue()->uri();
            found = true;

            // If queue exists, report self generated NACK
            if (isSelfGenerated) {
                it->second.d_handle_p->queue()->stats()->onEvent(
                    mqbstat::QueueStatsDomain::EventType::e_NACK,
                    1);
            }
        }
        else if (!isSelfGenerated) {
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedAckMessages,
                BALL_LOG_WARN
                    << description()
                    << ": ACK message for queue with unknown queueId ["
                    << queueId << ", guid: " << messageGUID << ", for node: "
                    << nodeSession->clusterNode()->nodeDescription(););
            return;  // RETURN
        }

        // Throttle error log if this is a 'failed Ack': note that we log at
        // INFO level in order not to overwhelm the dashboard, if a queue is
        // full, every post will nack, which could be a lot.
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedAckMessages,
            BALL_LOG_INFO << description() << ": failed Ack "
                          << "[status: " << status << ", source: '" << source
                          << "'" << ", correlationId: " << correlationId
                          << ", GUID: " << messageGUID << ", queue: '"
                          << (found ? uri : "** null **") << "' "
                          << "(id: " << queueId << ")] " << "to node "
                          << nodeSession->clusterNode()->nodeDescription(););
    }

    // Always print at trace level
    BALL_LOG_TRACE << description() << ": sending Ack "
                   << "[status: " << status << ", source: '" << source << "'"
                   << ", correlationId: " << correlationId
                   << ", GUID: " << messageGUID << ", queue: '" << uri
                   << "' (id: " << queueId << ")] to "
                   << "node " << nodeSession->clusterNode()->nodeDescription();

    // Update stats for the queue (or subStream of the queue)
    // TBD: We should collect all invalid stats (i.e. stats for queues that
    // were not found).  We could collect these under a new 'invalid queue'
    // stat context.
    QueueHandleMap::const_iterator cit = nodeSession->queueHandles().find(
        queueId);
    if (cit != nodeSession->queueHandles().end()) {
        StreamsMap::const_iterator subQueueCiter =
            cit->second.d_subQueueInfosMap.findBySubIdSafe(
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        if (subQueueCiter != cit->second.d_subQueueInfosMap.end()) {
            subQueueCiter->value().d_clientStats->onEvent(
                mqbstat::ClusterNodeStats::EventType::e_ACK,
                1);
        }
        // In the case of Strong Consistency, a Receipt can arrive and trigger
        // an ACK after Producer closes subStream.
    }

    bmqt::GenericResult::Enum rc =
        nodeSession->clusterNode()->channel().writeAck(
            bmqp::ProtocolUtil::ackResultToCode(status),
            correlationId,
            messageGUID,
            queueId);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // This is non-recoverable, so we drop the ACK msg and log it.

        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledDroppedAckMessages,
            BALL_LOG_ERROR << description() << ": dropping ACK message "
                           << "[status: " << status << ", source: '" << source
                           << "'" << ", correlationId: " << correlationId
                           << ", GUID: " << messageGUID
                           << ", queueId: " << queueId << "] to node "
                           << nodeSession->clusterNode()->nodeDescription()
                           << ", AckBuilder rc: " << rc << ".";);
    }
}

void Cluster::sendAck(bmqt::AckResult::Enum    status,
                      int                      correlationId,
                      const bmqt::MessageGUID& messageGUID,
                      int                      queueId,
                      const bslstl::StringRef& source,
                      mqbnet::ClusterNode*     destination,
                      bool                     isSelfGenerated)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(destination);

    mqbc::ClusterNodeSession* nodeSession =
        d_clusterData.membership().getClusterNodeSession(destination);
    BSLS_ASSERT(nodeSession);

    sendAck(status,
            correlationId,
            messageGUID,
            queueId,
            source,
            nodeSession,
            isSelfGenerated);
}

void Cluster::generateNack(bmqt::AckResult::Enum               status,
                           const bslstl::StringRef&            nackReason,
                           const bmqp::PutHeader&              putHeader,
                           mqbi::Queue*                        queue,
                           DispatcherClient*                   source,
                           const bsl::shared_ptr<bdlbb::Blob>& appData,
                           const bsl::shared_ptr<bdlbb::Blob>& options,
                           bool                                raiseAlarm)
{
    // executed by the *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    mqbc::ClusterUtil::generateNack(status,
                                    putHeader,
                                    source,
                                    dispatcher(),
                                    appData,
                                    options);

    // Report locally generated NACK
    queue->stats()->onEvent(mqbstat::QueueStatsDomain::EventType::e_NACK, 1);

    bmqu::MemOutStream os;
    os << description() << ": Failed to relay PUT message "
       << "[queueId: " << putHeader.queueId()
       << ", GUID: " << putHeader.messageGUID() << "]. "
       << "Reason: " << nackReason;
    BMQU_THROTTLEDACTION_THROTTLE(
        d_throttledFailedPutMessages,
        if (raiseAlarm) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << os.str() << BMQTSK_ALARMLOG_END;
        } else { BALL_LOG_WARN << os.str(); });
}

void Cluster::processCommandDispatched(mqbcmd::ClusterResult*        result,
                                       const mqbcmd::ClusterCommand& command)
{
    // executed by the *DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (command.isStatusValue()) {
        loadClusterStatus(result);
        return;  // RETURN
    }
    else if (command.isQueueHelperValue()) {
        d_clusterOrchestrator.queueHelper().loadState(
            &result->makeClusterQueueHelper());
        return;  // RETURN
    }
    else if (command.isForceGcQueuesValue()) {
        // 'true' implies immediate
        if (const int rc = d_clusterOrchestrator.queueHelper().gcExpiredQueues(
                true)) {
            BALL_LOG_ERROR << "Failed to execute force GC queues command (rc: "
                           << rc << ")";
            result->makeError().message() = "Failed to execute command (rc: " +
                                            bsl::to_string(rc) + ")";
        }
        else {
            // Otherwise the command succeeded.
            result->makeSuccess();
        }

        return;  // RETURN
    }
    else if (command.isStorageValue()) {
        mqbcmd::StorageResult storageResult;
        d_storageManager_mp->processCommand(&storageResult, command.storage());
        if (storageResult.isErrorValue()) {
            result->makeError(storageResult.error());
            return;  // RETURN
        }
        else if (storageResult.isSuccessValue()) {
            result->makeSuccess(storageResult.success());
            return;  // RETURN
        }

        result->makeStorageResult(storageResult);
        return;  // RETURN
    }
    else if (command.isStateValue()) {
        d_clusterOrchestrator.processCommand(command.state(), result);
        return;  // RETURN
    }

    bdlma::LocalSequentialAllocator<256> localAllocator(d_allocator_p);
    bmqu::MemOutStream                   os(&localAllocator);
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
}

void Cluster::initiateShutdownDispatched(const VoidFunctor& callback,
                                         bool               supportShutdownV2)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BALL_LOG_INFO << "Shutting down Cluster: [name: '" << name() << "']";

    d_isStopping = true;

    d_clusterData.membership().setSelfNodeStatus(
        bmqp_ctrlmsg::NodeStatus::E_STOPPING);

    if (supportShutdownV2) {
        d_clusterOrchestrator.queueHelper().requestToStopPushing();

        bsls::TimeInterval whenToStop(
            bsls::SystemTime::now(bsls::SystemClockType::e_MONOTONIC));
        whenToStop.addMilliseconds(d_clusterData.clusterConfig()
                                       .queueOperations()
                                       .shutdownTimeoutMs());

        d_shutdownChain.appendInplace(
            bdlf::BindUtil::bind(&ClusterQueueHelper::checkUnconfirmedV2,
                                 &d_clusterOrchestrator.queueHelper(),
                                 whenToStop,
                                 bdlf::PlaceHolders::_1));  // completionCb
    }
    else {
        // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest
        // V2.
        // Send StopRequest to all nodes and proxies.  The peers are expected
        // not to send any PUT msgs to this node after receiving StopRequest.
        // For each queue for which this node is the primary, peers (replicas
        // and proxies) will de-configure the queue, wait for configured
        // timeout, close the queue, and respond with StopResponse.  The peers
        // are expected not to send any PUT/PUSH/ACK/CONFIRM msgs to this node
        // after sending StopResponse.
        //
        // Call 'initiateShutdown' for all client sessions.

        bmqu::OperationChainLink link(d_shutdownChain.allocator());
        bsls::TimeInterval       shutdownTimeout;
        shutdownTimeout.addMilliseconds(d_clusterData.clusterConfig()
                                            .queueOperations()
                                            .shutdownTimeoutMs());

        SessionSpVec sessions;
        for (mqbnet::TransportManagerIterator sessIt(
                 &d_clusterData.transportManager());
             sessIt;
             ++sessIt) {
            bsl::shared_ptr<mqbnet::Session> sessionSp =
                sessIt.session().lock();
            if (!sessionSp) {
                continue;  // CONTINUE
            }

            const bmqp_ctrlmsg::NegotiationMessage& negoMsg =
                sessionSp->negotiationMessage();

            const bmqp_ctrlmsg::ClientIdentity& peerIdentity =
                negoMsg.isClientIdentityValue()
                    ? negoMsg.clientIdentity()
                    : negoMsg.brokerResponse().brokerIdentity();

            if (peerIdentity.clusterNodeId() ==
                d_clusterData.membership().netCluster()->selfNodeId()) {
                continue;  // CONTINUE
            }

            if (mqbnet::ClusterUtil::isClient(negoMsg)) {
                link.insert(bdlf::BindUtil::bind(
                    &mqbnet::Session::initiateShutdown,
                    sessionSp,
                    bdlf::PlaceHolders::_1,  // completion callback
                    shutdownTimeout,
                    false));

                continue;  // CONTINUE
            }

            if (peerIdentity.clusterName() == name()) {
                // Expect all proxies and nodes support this feature.
                if (!bmqp::ProtocolUtil::hasFeature(
                        bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
                        bmqp::HighAvailabilityFeatures::k_GRACEFUL_SHUTDOWN,
                        peerIdentity.features())) {
                    BALL_LOG_ERROR
                        << description() << ": Peer doesn't support "
                        << "GRACEFUL_SHUTDOWN. Skip sending stopRequest"
                        << " to [" << peerIdentity << "]";
                    continue;  // CONTINUE
                }
                sessions.push_back(sessionSp);
            }
        }

        link.insert(bdlf::BindUtil::bind(
            &Cluster::sendStopRequest,
            this,
            sessions,
            bdlf::PlaceHolders::_1));  // completion callback

        d_shutdownChain.append(&link);
    }

    // Also update self's status.  Note that this node does not explicitly
    // issue a close-queue request for each of the queues.

    d_shutdownChain.appendInplace(
        bdlf::BindUtil::bind(&Cluster::continueShutdown,
                             this,
                             bmqsys::Time::highResolutionTimer(),  // startTime
                             bdlf::PlaceHolders::_1),  // completionCb
        callback);

    d_shutdownChain.start();
}

void Cluster::sendStopRequest(const SessionSpVec&                  sessions,
                              const StopRequestCompletionCallback& stopCb)
{
    // Send a StopRequest to available cluster nodes and proxies connected to
    // the cluster
    StopRequestManagerType::RequestContextSp contextSp =
        d_stopRequestsManager_p->createRequestContext();
    bmqp_ctrlmsg::StopRequest& request = contextSp->request()
                                             .choice()
                                             .makeClusterMessage()
                                             .choice()
                                             .makeStopRequest();
    request.clusterName() = name();
    contextSp->setDestinationNodes(sessions);

    contextSp->setResponseCb(stopCb);

    const mqbcfg::QueueOperationsConfig& queueOpConfig =
        d_clusterData.clusterConfig().queueOperations();
    bsls::TimeInterval timeoutMs;
    timeoutMs.setTotalMilliseconds(queueOpConfig.shutdownTimeoutMs());

    BALL_LOG_INFO << "Sending StopRequest to " << sessions.size()
                  << " brokers; timeout is " << timeoutMs;

    d_stopRequestsManager_p->sendRequest(contextSp, timeoutMs);

    // continue after receipt of all StopResponses or the timeout
}

void Cluster::continueShutdown(bsls::Types::Int64        startTimeNs,
                               const CompletionCallback& completionCb)
{
    // executed by either the *DISPATCHER* thread or one of *CLIENT* threads

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::continueShutdownDispatched,
                             this,
                             startTimeNs,
                             completionCb),
        this);
}

void Cluster::continueShutdownDispatched(
    bsls::Types::Int64        startTimeNs,
    const CompletionCallback& completionCb)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    bdlb::ScopeExitAny guard(completionCb);

    BALL_LOG_INFO_BLOCK
    {
        bsls::Types::Int64 now = bmqsys::Time::highResolutionTimer();
        BALL_LOG_OUTPUT_STREAM
            << "Continuing shutting down Cluster: [name: '" << name()
            << "', elapsed time: "
            << bmqu::PrintUtil::prettyTimeInterval(now - startTimeNs) << " ("
            << (now - startTimeNs) << " nanoseconds)]";
    }

    // If leader, mark self as a passive leader.
    if (mqbnet::ElectorState::e_LEADER ==
        d_clusterData.electorInfo().electorState()) {
        d_clusterData.electorInfo().setLeaderStatus(
            mqbc::ElectorInfoLeaderStatus::e_PASSIVE);
    }

    // Cancel all requests with the 'CANCELED' category and the 'e_STOPPING'
    // code.
    bmqp_ctrlmsg::ControlMessage response;
    bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
    failure.message()  = "Node is being stopped";

    d_clusterData.requestManager().cancelAllRequests(response);
    // All downstreams have either disconnected (clients) or responded with
    // StopResponse
    // There could be outstanding close queue requests resulting from handle
    // drop resulting from 'ClientSession::tearDownImpl' but 'doDropHandle'
    // does not wait for responses.
    // Note that requests did go out (from the cluster thread) because
    //  1) 'ClientSession::tearDownImpl' waits for 'tearDownAllQueuesDone'
    //      before triggering 'Cluster::continueShutdown'
    //  2) 'Cluster::continueShutdown' executes 'continueShutdownDispatched'

    // Indicate the StorageMgr that self node is stopping.  For all the
    // partitions for which self is primary, self will issue last syncPt and
    // 'passive' primary status advisory.  Note that 'passive' primary advisory
    // this will reach the peers after the 'e_STOPPING' event, which is
    // broadcast above.  This is per design and the ordering should not be
    // changed.

    d_storageManager_mp->processShutdownEvent();

    // Also update primary status for those partitions in cluster state.

    const bsl::vector<int>& primaryPartitions =
        d_clusterData.membership().selfNodeSession()->primaryPartitions();
    for (size_t i = 0; i < primaryPartitions.size(); ++i) {
        const int pid = primaryPartitions[i];
        BSLS_ASSERT_SAFE(d_state.partition(pid).primaryNode() ==
                         d_clusterData.membership().selfNode());
        d_state.setPartitionPrimaryStatus(
            pid,
            bmqp_ctrlmsg::PrimaryStatus::E_PASSIVE);
    }

    // Indicate the QueueHelper that self node is stopping.  QueueHelper will
    // delete and unregister all queues which have no handles from the domain.
    // Such queues will eventually be GC'ed but that may take a while,
    // something that we don't want when shutting down.

    d_clusterOrchestrator.queueHelper().processShutdownEvent();
}

void Cluster::onRelayPutEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(mqbi::DispatcherEventType::e_PUT == event.type());
    BSLS_ASSERT_SAFE(event.asPutEvent()->isRelay());

    const mqbi::DispatcherPutEvent* realEvent = event.asPutEvent();

    // This relay-PUT message is enqueued by the RemoteQueue on either cluster
    // (in case of replica) or clusterProxy (in case of proxy).  This is a
    // replica so this node just needs to forward the message to queue's
    // partition's primary node (after appropriate checks).
    //
    // Note: Due to internal knowledge of the RemoteQueue component, we can be
    //       sure that there is exactly one PUT message contained in 'event'.

    const bmqp::PutHeader& ph       = realEvent->putHeader();
    const int              pid      = realEvent->partitionId();
    bsls::Types::Uint64    genCount = realEvent->genCount();
    bsls::Types::Uint64    leaseId  = d_state.partition(pid).primaryLeaseId();

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(genCount != leaseId)) {
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledSkippedPutMessages,
            BALL_LOG_WARN << description()
                          << ": skipping relay-PUT message [ queueId: "
                          << ph.queueId() << ", GUID: " << ph.messageGUID()
                          << "], genCount: " << genCount << " vs " << leaseId
                          << ".";);
        return;  // RETURN
    }

    mqbi::Queue* queue = d_clusterOrchestrator.queueHelper().lookupQueue(
        ph.queueId());
    BSLS_ASSERT_SAFE(queue);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            mqbs::DataStore::k_INVALID_PARTITION_ID == pid)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Inform event's source of relay-PUT failure
        bmqu::MemOutStream os;
        os << "invalid partition [" << pid << "]";
        generateNack(bmqt::AckResult::e_INVALID_ARGUMENT,
                     os.str(),
                     ph,
                     queue,
                     event.source(),
                     0,
                     0,
                     true);  // raiseAlarm

        return;  // RETURN
    }

    // DO process PUTs in the E_STOPPING state.  This is for broadcast PUTs
    // that "cross" StopRequest.  Since 'RemoteQueue' does not buffer broadcast
    // PUTs data, "crossed" PUTs will be lost.
    // But do NOT send to E_STOPPING upstream.  See below.
    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Inform event's source of relay-PUT failure
        bmqu::MemOutStream os;
        os << "self (replica node) not available. Self status: " << selfStatus;
        generateNack(bmqt::AckResult::e_NOT_READY,
                     os.str(),
                     ph,
                     queue,
                     event.source(),
                     0,
                     0,
                     false);  // raiseAlarm

        return;  // RETURN
    }

    BSLS_ASSERT(pid < static_cast<int>(d_state.partitions().size()));
    const ClusterStatePartitionInfo& pinfo = d_state.partition(pid);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            0 == pinfo.primaryNode() ||
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        bmqu::MemOutStream os;
        os << "no or non-active primary for partition [" << pid << "]";
        generateNack(bmqt::AckResult::e_NOT_READY,
                     os.str(),
                     ph,
                     queue,
                     event.source(),
                     realEvent->blob(),
                     realEvent->options(),
                     false);  // raiseAlarm

        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            pinfo.primaryNode()->nodeId() ==
            d_clusterData.membership().netCluster()->selfNodeId())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // This should not occur
        bmqu::MemOutStream os;
        os << "self is primary for partition [" << pid << "]";
        generateNack(bmqt::AckResult::e_UNKNOWN,
                     os.str(),
                     ph,
                     queue,
                     event.source(),
                     0,
                     0,
                     false);  // raiseAlarm

        return;  // RETURN
    }

    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(pinfo.primaryNode());
    BSLS_ASSERT_SAFE(ns);

    bmqp_ctrlmsg::NodeStatus::Value primaryStatus = ns->nodeStatus();

    // Do not send to E_STOPPING upstream.  Self-NACK with data.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != primaryStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        bmqu::MemOutStream os;
        os << "primary not available. Primary status: " << primaryStatus;
        generateNack(bmqt::AckResult::e_NOT_READY,
                     os.str(),
                     ph,
                     queue,
                     event.source(),
                     realEvent->blob(),
                     realEvent->options(),
                     false);  // raiseAlarm

        return;  // RETURN
    }

    bmqt::GenericResult::Enum rc = ns->clusterNode()->channel().writePut(
        realEvent->putHeader(),
        realEvent->blob(),
        realEvent->state());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // This is non-recoverable, so we drop the PUT msg and log it.

        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledSkippedPutMessages,
            BALL_LOG_ERROR << description() << ": skipping relay-PUT message ["
                           << "queueId: " << ph.queueId() << ", GUID: "
                           << ph.messageGUID() << "] to primary node: "
                           << ns->clusterNode()->nodeDescription()
                           << ", rc: " << rc << ".";);
    }
}

void Cluster::onPutEvent(const mqbi::DispatcherPutEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!event.isRelay());

    // This PUT event arrives from a replica node to this (primary) node, and
    // it needs to be forwarded to the queue after appropriate checks.  The
    // replica source node is event.clusterNode().  This routine is similar to
    // that of ClientSession.

    mqbnet::ClusterNode*      source = event.clusterNode();
    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);

    bmqp::Event              rawEvent(event.blob().get(), d_allocator_p);
    bmqp::PutMessageIterator putIt(&d_clusterData.bufferFactory(),
                                   d_allocator_p);

    BSLS_ASSERT_SAFE(rawEvent.isPutEvent());
    rawEvent.loadPutMessageIterator(&putIt);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!putIt.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        bmqu::MemOutStream out;
        out << description() << ": received an invalid PUT event from node "
            << source->nodeDescription() << "\n";
        putIt.dumpBlob(out);
        BMQTSK_ALARMLOG_ALARM("CLUSTER") << out.str() << BMQTSK_ALARMLOG_END;
        BSLS_ASSERT_SAFE(false && "Invalid putMessage");
        return;  // RETURN
    }

    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // This node is either starting, in maintenance mode, or shut down.
        // Inform event's source of PUT failure.
        while (putIt.next() == 1) {
            sendAck(bmqt::AckResult::e_NOT_READY,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    putIt.header().queueId(),
                    "Node unavailable",
                    ns,
                    true);  // isSelfGenerated
        };
        return;  // RETURN
    }

    int rc     = 0;
    int msgNum = 0;

    while ((rc = putIt.next()) == 1) {
        const bmqp::QueueId queueId(putIt.header().queueId(),
                                    bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
        QueueHandleMapIter  queueIt = ns->queueHandles().find(queueId.id());

        // Check if queueId represents a valid queue
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queueIt ==
                                                  ns->queueHandles().end())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPutMessages,
                BALL_LOG_ERROR << description()
                               << ": PUT message for queue with unknown"
                               << " Id (" << queueId.id() << ") from "
                               << source->nodeDescription(););

            sendAck(bmqt::AckResult::e_INVALID_ARGUMENT,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    queueId.id(),
                    "putEvent::UnknownQueue",
                    ns,
                    true);  // isSelfGenerated
            continue;       // CONTINUE
        }

        const QueueState& queueState = queueIt->second;

        // Ensure that queueId exists in the QueueState
        BSLS_ASSERT_SAFE(queueState.d_subQueueInfosMap.findBySubIdSafe(
                             bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) !=
                         queueState.d_subQueueInfosMap.end());

        // Ensure that queue is opened in WRITE mode
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !bmqt::QueueFlagsUtil::isWriter(
                    queueState.d_handle_p->handleParameters().flags()))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPutMessages,
                BALL_LOG_WARN << description() << ": PUT message for queue ["
                              << queueState.d_handle_p->queue()->uri()
                              << "] not opened in WRITE mode.";);

            sendAck(bmqt::AckResult::e_REFUSED,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    queueId.id(),
                    "putEvent::notWritable",
                    ns,
                    true);  // isSelfGenerated
            continue;       // CONTINUE
        }

        // Ensure that final closeQueue request has not been sent.

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                queueState.d_isFinalCloseQueueReceived)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPutMessages,
                BALL_LOG_WARN << description()
                              << ": rejecting PUT message for queue ["
                              << queueState.d_handle_p->queue()->uri()
                              << "] as final closeQueue request was already "
                              << "received for this queue.";);

            sendAck(bmqt::AckResult::e_REFUSED,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    queueId.id(),
                    "putEvent::notWritable",
                    ns,
                    true);  // isSelfGenerated
            continue;       // CONTINUE
        }

        // Retrieve the payload of that message
        bsl::shared_ptr<bdlbb::Blob> appDataSp =
            d_clusterData.blobSpPool().getObject();
        rc = putIt.loadApplicationData(appDataSp.get());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPutMessages,
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << description() << ": Failed to load PUT message payload"
                    << " [queue: " << queueState.d_handle_p->queue()->uri()
                    << ", guid: " << putIt.header().messageGUID()
                    << ", from node " << source->nodeDescription()
                    << ", rc: " << rc << "]" << BMQTSK_ALARMLOG_END;);

            sendAck(bmqt::AckResult::e_INVALID_ARGUMENT,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    queueId.id(),
                    "putEvent::failedLoadApplicationData",
                    ns,
                    true);  // isSelfGenerated

            continue;  // CONTINUE
        }

        StreamsMap::const_iterator subQueueCiter =
            queueState.d_subQueueInfosMap.findBySubId(
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        subQueueCiter->value().d_clientStats->onEvent(
            mqbstat::ClusterNodeStats::EventType::e_PUT,
            appDataSp->length());

        // TBD: groupId: Similar to 'appDataSp' above, load 'optionsSp' here,
        // using something like PutMessageIterator::loadOptions().
        bsl::shared_ptr<bdlbb::Blob> optionsSp;

        // All good.  Post message to the queue.
        BALL_LOG_TRACE << description() << ": PUT message #" << ++msgNum
                       << " [queue: " << queueState.d_handle_p->queue()->uri()
                       << ", GUID: " << putIt.header().messageGUID()
                       << ", message size: " << putIt.applicationDataSize()
                       << "]:\n"
                       << bmqu::BlobStartHexDumper(appDataSp.get(), 64);

        // If at-most-once queue and ack is requested, send an ack
        // immediately regardless of whether this is first hop or not.
        const bool isAtMostOnce =
            queueState.d_handle_p->queue()->isAtMostOnce();
        int        flags          = putIt.header().flags();
        const bool isAckRequested = bmqp::PutHeaderFlagUtil::isSet(
            putIt.header().flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);

        if (isAtMostOnce) {
            if (isAckRequested) {
                sendAck(bmqt::AckResult::e_SUCCESS,
                        bmqp::AckMessage::k_NULL_CORRELATION_ID,
                        putIt.header().messageGUID(),
                        queueId.id(),
                        "putEvent::auto-ACK",
                        ns,
                        true);  // isSelfGenerated
            }
            bmqp::PutHeaderFlagUtil::unsetFlag(
                &flags,
                bmqp::PutHeaderFlags::e_ACK_REQUESTED);
            const_cast<bmqp::PutHeader&>(putIt.header()).setFlags(flags);
        }
        else {
            BSLS_ASSERT_SAFE(isAckRequested);
        }

        queueState.d_handle_p->postMessage(putIt.header(),
                                           appDataSp,
                                           optionsSp);
    }

    // Check if the PUT event was valid
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPutMessages,
            BALL_LOG_ERROR_BLOCK {
                BALL_LOG_OUTPUT_STREAM << description()
                                       << ": Invalid Put event [rc: " << rc
                                       << "]\n";
                putIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
            });
        BSLS_ASSERT_SAFE(false && "Invalid PUT event");
    }
}

void Cluster::onAckEvent(const mqbi::DispatcherAckEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!event.isRelay());

    // This ACK message is enqueued by mqbblp::Queue on this node, and needs to
    // be forwarded to 'event.clusterNode()' (the replica node).

    // NOTE: we do not log anything here, all logging is done in 'sendAck'.

    const bmqp::AckMessage&         ackMessage = event.ackMessage();
    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Drop ACK coz self is unavailable
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedAckMessages,
            BALL_LOG_WARN << "Dropping an ACK for queue [queueId: "
                          << ackMessage.queueId()
                          << ", guid: " << ackMessage.messageGUID()
                          << ", status: " << ackMessage.status()
                          << "] for node "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: self (primary node) not available. "
                          << "Node status: " << selfStatus;);
        return;  // RETURN
    }

    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(event.clusterNode());
    BSLS_ASSERT_SAFE(ns);

    bmqp_ctrlmsg::NodeStatus::Value downstreamStatus = ns->nodeStatus();

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != downstreamStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != downstreamStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Drop the ACK because downstream node is either starting, in the
        // maintenance mode, or shut down.
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedAckMessages,
            BALL_LOG_WARN << "Dropping an ACK for queue [queueId: "
                          << ackMessage.queueId()
                          << ", guid: " << ackMessage.messageGUID()
                          << ", status: " << ackMessage.status()
                          << "] for node "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: target node not available. "
                          << "Node status: " << downstreamStatus;);
        return;  // RETURN
    }

    sendAck(bmqp::ProtocolUtil::ackResultFromCode(ackMessage.status()),
            ackMessage.correlationId(),
            ackMessage.messageGUID(),
            ackMessage.queueId(),
            "onAckEvent",
            event.clusterNode(),
            false);  // isSelfGenerated
}

void Cluster::onRelayAckEvent(const mqbi::DispatcherAckEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay());

    // This relay-ACK event is sent by primary (event.clusterNode()) to replica
    // (this) node.  Iterate over each message in the event and forward it to
    // appropriate remote queue.

    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Drop ACK messages coz self is down
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedAckMessages,
            BALL_LOG_WARN << "Dropping relay ACK messages from node "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: self (replica node) not available."
                          << " Self node status: " << selfStatus;);
        return;  // RETURN
    }

    bmqp::Event rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isAckEvent());

    bmqp::AckMessageIterator ackIt;
    rawEvent.loadAckMessageIterator(&ackIt);
    BSLS_ASSERT_SAFE(ackIt.isValid());

    int rc = 0;
    while ((rc = ackIt.next()) == 1) {
        const bmqp::AckMessage& ackMessage = ackIt.message();

        mqbi::Queue* queue = d_clusterOrchestrator.queueHelper().lookupQueue(
            ackMessage.queueId());
        if (queue == 0) {
            // TBD: Logging at INFO level at the moment, until queue close
            //      sequence is properly handled, this is a (non-fatal)
            //      situation which can happen.
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedAckMessages,
                BALL_LOG_INFO << "[CLUSTER] " << description()
                              << ": Received an ACK for unknown queue "
                              << "[queueId: " << ackMessage.queueId()
                              << ", guid: " << ackMessage.messageGUID()
                              << ", status: " << ackMessage.status()
                              << "] from node "
                              << event.clusterNode()->nodeDescription(););

            continue;  // CONTINUE
        }

        queue->onAckMessage(ackMessage);
    }
}

void Cluster::onConfirmEvent(const mqbi::DispatcherConfirmEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!event.isRelay());

    // This CONFIRM event arrives from a replica node (event.clusterNode()) to
    // this (primary) node, and it needs to be forwarded to the queue after
    // appropriate checks.  Iterate over each CONFIRM message in the event and
    // forward it to the queue handle.

    mqbnet::ClusterNode*      source = event.clusterNode();
    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);

    bmqp::Event                  rawEvent(event.blob().get(), d_allocator_p);
    bmqp::ConfirmMessageIterator confIt;

    BSLS_ASSERT_SAFE(rawEvent.isConfirmEvent());
    rawEvent.loadConfirmMessageIterator(&confIt);
    if (!confIt.isValid()) {
        bmqu::MemOutStream out;
        out << description() << ": received invalid CONFIRM event from node "
            << source->nodeDescription() << "\n";
        confIt.dumpBlob(out);
        BMQTSK_ALARMLOG_ALARM("CLUSTER") << out.str() << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }
    bmqp_ctrlmsg::NodeStatus::Value status =
        d_clusterData.membership().selfNodeStatus();
    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != status &&
        bmqp_ctrlmsg::NodeStatus::E_STOPPING != status) {
        // This node is going down.
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedConfirmMessages,
            BALL_LOG_WARN << "Failed to apply CONFIRM event to queue from "
                          << source->nodeDescription()
                          << ". Reason: self (primary node) is not available. "
                          << "Self node status: "
                          << d_clusterData.membership().selfNodeStatus(););
        return;  // RETURN
    }

    int msgNum = 0;
    int rc     = 0;

    while ((rc = confIt.next() == 1)) {
        const int          id    = confIt.message().queueId();
        const unsigned int subId = static_cast<unsigned int>(
            confIt.message().subQueueId());
        const bmqp::QueueId queueId(id, subId);
        mqbi::QueueHandle*  queueHandle = 0;

        ValidationResult::Enum result = validateMessage(
            &queueHandle,
            queueId,
            ns,
            bmqp::EventType::e_CONFIRM);
        if (result == ValidationResult::k_SUCCESS) {
            BSLS_ASSERT_SAFE(queueHandle);

            BALL_LOG_TRACE << description() << ": CONFIRM "
                           << " message #" << ++msgNum
                           << " [queue: " << queueHandle->queue()->uri()
                           << "', queueId: " << queueId
                           << ", GUID: " << confIt.message().messageGUID()
                           << "] from node " << source->nodeDescription();
            queueHandle->confirmMessage(confIt.message().messageGUID(),
                                        queueId.subId());
        }
        else {
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedRejectMessages,
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << ": CONFIRM " << ValidationResult::toAscii(result)
                    << " [queue: '"
                    << (queueHandle ? queueHandle->queue()->uri()
                                    : "<UNKNOWN>")
                    << "', queueId: " << queueId << ", GUID: "
                    << confIt.message().messageGUID() << "] from "
                    << source->nodeDescription() << BMQTSK_ALARMLOG_END;);
        }
    }

    if (rc < 0) {
        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << description()
                                   << ": Invalid ConfirmMessage event "
                                   << " [rc: " << rc << "]\n";
            confIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
        BSLS_ASSERT_SAFE(false && "Invalid ConfirmMessage");
    }
}

void Cluster::onRejectEvent(const mqbi::DispatcherRejectEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!event.isRelay());

    // This REJECT event arrives from a replica node (event.clusterNode()) to
    // this (primary) node, and it needs to be forwarded to the queue after
    // appropriate checks.  Iterate over each REJECT message in the event and
    // forward it to the queue handle.

    mqbnet::ClusterNode*      source = event.clusterNode();
    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(source);
    BSLS_ASSERT_SAFE(ns);

    bmqp::Event                 rawEvent(event.blob().get(), d_allocator_p);
    bmqp::RejectMessageIterator rejectIt;
    BSLS_ASSERT_SAFE(rawEvent.isRejectEvent());
    rawEvent.loadRejectMessageIterator(&rejectIt);

    if (!rejectIt.isValid()) {
        bmqu::MemOutStream out;
        out << description() << ": received invalid REJECT event from node "
            << source->nodeDescription() << "\n";
        rejectIt.dumpBlob(out);
        BMQTSK_ALARMLOG_ALARM("CLUSTER") << out.str() << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }
    bmqp_ctrlmsg::NodeStatus::Value status =
        d_clusterData.membership().selfNodeStatus();
    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != status &&
        bmqp_ctrlmsg::NodeStatus::E_STOPPING != status) {
        // This node is going down.
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedRejectMessages,
            BALL_LOG_WARN << "Failed to apply REJECT event to queue from "
                          << source->nodeDescription()
                          << ". Reason: self (primary node) is not available. "
                          << "Self node status: "
                          << d_clusterData.membership().selfNodeStatus(););
        return;  // RETURN
    }

    int msgNum = 0;
    int rc     = 0;

    while ((rc = rejectIt.next() == 1)) {
        const int          id    = rejectIt.message().queueId();
        const unsigned int subId = static_cast<unsigned int>(
            rejectIt.message().subQueueId());
        const bmqp::QueueId queueId(id, subId);
        mqbi::QueueHandle*  queueHandle = 0;

        ValidationResult::Enum result = validateMessage(
            &queueHandle,
            queueId,
            ns,
            bmqp::EventType::e_REJECT);
        if (result == ValidationResult::k_SUCCESS) {
            BSLS_ASSERT_SAFE(queueHandle);

            BALL_LOG_TRACE << description() << ": REJECT "
                           << " message #" << ++msgNum
                           << " [queue: " << queueHandle->queue()->uri()
                           << "', queueId: " << queueId
                           << ", GUID: " << rejectIt.message().messageGUID()
                           << "] from " << source->nodeDescription();
            queueHandle->rejectMessage(rejectIt.message().messageGUID(),
                                       queueId.subId());
        }
        else {
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedRejectMessages,
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << ": REJECT " << ValidationResult::toAscii(result)
                    << " [queue: '"
                    << (queueHandle ? queueHandle->queue()->uri()
                                    : "<UNKNOWN>")
                    << "', queueId: " << queueId << ", GUID: "
                    << rejectIt.message().messageGUID() << "] from "
                    << source->nodeDescription() << BMQTSK_ALARMLOG_END;);
        }
    }

    if (rc < 0) {
        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << description()
                                   << ": Invalid RejectMessage event "
                                   << " [rc: " << rc << "]\n";
            rejectIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }

        BSLS_ASSERT_SAFE(false && "Invalid RejectMessage");
    }
}

Cluster::ValidationResult::Enum
Cluster::validateMessage(mqbi::QueueHandle**       queueHandle,
                         const bmqp::QueueId&      queueId,
                         mqbc::ClusterNodeSession* ns,
                         bmqp::EventType::Enum     eventType)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE((eventType == bmqp::EventType::e_CONFIRM ||
                      eventType == bmqp::EventType::e_REJECT) &&
                     "Unsupported eventType");
    BSLS_ASSERT_SAFE(queueHandle);

    QueueHandleMap&    queueHandles = ns->queueHandles();
    QueueHandleMapIter queueIt      = queueHandles.find(queueId.id());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queueIt == queueHandles.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        return ValidationResult::k_UNKNOWN_QUEUE;  // RETURN
    }

    const QueueState&          queueState = queueIt->second;
    StreamsMap::const_iterator subQueueIt =
        queueState.d_subQueueInfosMap.findBySubIdSafe(queueId.subId());

    *queueHandle = queueState.d_handle_p;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            subQueueIt == queueState.d_subQueueInfosMap.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        return ValidationResult::k_UNKNOWN_SUBQUEUE;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            queueState.d_isFinalCloseQueueReceived)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        return ValidationResult::k_FINAL;  // RETURN
    }

    if (eventType == bmqp::EventType::e_CONFIRM) {
        // Update client stats
        subQueueIt->value().d_clientStats->onEvent(
            mqbstat::ClusterNodeStats::EventType::e_CONFIRM,
            1);
    }

    return ValidationResult::k_SUCCESS;
}

void Cluster::onRelayRejectEvent(const mqbi::DispatcherRejectEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay());

    // This relay-REJECT message is enqueued by the RemoteQueue on either
    // cluster (in case of replica) or clusterProxy (in case of proxy).  This
    // is a replica so this node just needs to forward the message to queue's
    // partition's primary node (after appropriate checks).

    const bmqp::RejectMessage& rejectMessage = event.rejectMessage();
    const int                  pid           = event.partitionId();

    const int          id    = rejectMessage.queueId();
    const unsigned int subId = static_cast<unsigned int>(
        rejectMessage.subQueueId());
    const bmqp::QueueId       queueId(id, subId);
    mqbc::ClusterNodeSession* ns = 0;

    bdlma::LocalSequentialAllocator<256> localAllocator(d_allocator_p);
    bmqu::MemOutStream                   errorStream(&localAllocator);

    bool isValid = validateRelayMessage(&ns, &errorStream, pid);
    if (isValid) {
        bmqt::GenericResult::Enum rc =
            ns->clusterNode()->channel().writeReject(
                queueId.id(),
                queueId.subId(),
                rejectMessage.messageGUID());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                rc != bmqt::GenericResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // This is non-recoverable, so we drop the REJECT msg and log it.
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledDroppedRejectMessages,
                BALL_LOG_ERROR << description() << ": dropping REJECT message "
                               << "[queueId: " << queueId.id()
                               << ", subQueuId: " << queueId.subId()
                               << ", GUID: " << rejectMessage.messageGUID()
                               << "] to node "
                               << ns->clusterNode()->nodeDescription()
                               << ", RejectBuilder rc: " << rc << ".";);
        }
    }
    else {
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedRejectMessages,
            BALL_LOG_WARN << "Failed to relay REJECT message "
                          << "[queueId: " << queueId.id()
                          << ", subQueueId: " << queueId.subId()
                          << ", GUID: " << rejectMessage.messageGUID() << "]. "
                          << errorStream.str(););
    }
}

void Cluster::onRelayConfirmEvent(const mqbi::DispatcherConfirmEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay());

    // This relay-CONFIRM message is enqueued by the RemoteQueue on either
    // cluster (in case of replica) or clusterProxy (in case of proxy).  This
    // is a replica so this node just needs to forward the message to queue's
    // partition's primary node (after appropriate checks).

    const bmqp::ConfirmMessage& confirmMsg = event.confirmMessage();
    const int                   pid        = event.partitionId();

    const int          id    = confirmMsg.queueId();
    const unsigned int subId = static_cast<unsigned int>(
        confirmMsg.subQueueId());
    const bmqp::QueueId       queueId(id, subId);
    mqbc::ClusterNodeSession* ns = 0;

    bdlma::LocalSequentialAllocator<256> localAllocator(d_allocator_p);
    bmqu::MemOutStream                   errorStream(&localAllocator);

    bool isValid = validateRelayMessage(&ns, &errorStream, pid);
    if (isValid) {
        bmqt::GenericResult::Enum rc =
            ns->clusterNode()->channel().writeConfirm(
                queueId.id(),
                queueId.subId(),
                confirmMsg.messageGUID());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                rc != bmqt::GenericResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // This is non-recoverable, so we drop the CONFIRM msg and log it.
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledDroppedConfirmMessages,
                BALL_LOG_ERROR << description() << ": dropping CONFIRM message"
                               << " [queueId: " << queueId.id()
                               << ", subQueuId: " << queueId.subId()
                               << ", GUID: " << confirmMsg.messageGUID()
                               << "] to node "
                               << ns->clusterNode()->nodeDescription()
                               << ", ConfirmBuilder rc: " << rc << ".";);
        }
    }
    else {
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedRejectMessages,
            BALL_LOG_WARN << "Failed to relay CONFIRM message "
                          << "[queueId: " << queueId.id()
                          << ", subQueueId: " << queueId.subId()
                          << ", GUID: " << confirmMsg.messageGUID() << "]. "
                          << errorStream.str(););
    }
}

bool Cluster::validateRelayMessage(mqbc::ClusterNodeSession** ns,
                                   bsl::ostream*              errorStream,
                                   const int                  pid)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(ns);

    if (mqbs::DataStore::k_INVALID_PARTITION_ID == pid) {
        *errorStream << "Reason: invalid partition.";

        return false;  // RETURN
    }

    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus &&
            bmqp_ctrlmsg::NodeStatus::E_STOPPING != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Drop this relay-REJECT event
        *errorStream << "Reason: self (replica node) not available. "
                     << "Node status: " << selfStatus;

        return false;  // RETURN
    }

    BSLS_ASSERT(pid < static_cast<int>(d_state.partitions().size()));
    const ClusterStatePartitionInfo& pinfo = d_state.partition(pid);

    if (0 == pinfo.primaryNode() ||
        bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE != pinfo.primaryStatus()) {
        *errorStream << "Reason: no or non-active primary for partition"
                     << " [" << pid << "]";

        return false;  // RETURN
    }

    if (pinfo.primaryNode()->nodeId() ==
        d_clusterData.membership().netCluster()->selfNodeId()) {
        *errorStream << "Reason: self is primary for partition [" << pid
                     << "]";

        return false;  // RETURN
    }

    *ns = d_clusterData.membership().getClusterNodeSession(
        pinfo.primaryNode());
    BSLS_ASSERT_SAFE(*ns);

    bmqp_ctrlmsg::NodeStatus::Value status = (*ns)->nodeStatus();
    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != status &&
        bmqp_ctrlmsg::NodeStatus::E_STOPPING != status) {
        *errorStream << "Reason: primary not available. Primary status: "
                     << (*ns)->nodeStatus();

        return false;  // RETURN
    }

    return true;
}

void Cluster::onPushEvent(const mqbi::DispatcherPushEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!event.isRelay());

    // This PUSH message is enqueued by mqbblp::Queue/QueueHandle on this node,
    // and needs to be forwarded to 'event.clusterNode()' (the replica node,
    // which is the client).  Note that replica is already expected to have the
    // payload, and so, primary (this node) sends only the guid and, if
    // applicable, the associated subQueueIds.

    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Drop PUSH coz self is going down
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPushMessages,
            BALL_LOG_WARN << "Dropping a PUSH for queue [queueId: "
                          << event.queueId() << ", guid: " << event.guid()
                          << "] for node "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: self (primary node) not available."
                          << " Node status: " << selfStatus;);
        return;  // RETURN
    }

    mqbc::ClusterNodeSession* ns =
        d_clusterData.membership().getClusterNodeSession(event.clusterNode());
    BSLS_ASSERT_SAFE(ns);

    if (bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != ns->nodeStatus()) {
        // Target node is not AVAILABLE, so we don't send this PUSH msg to it.
        // Note that this PUSH msg was dispatched by the queue handle
        // representing the target node, and will be in its 'pending list'.

        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPushMessages,
            BALL_LOG_WARN << description()
                          << ": Failed to send PUSH message [queueId: "
                          << event.queueId() << ", GUID: " << event.guid()
                          << "] to target node: "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: node not available. "
                          << "Target node status: " << ns->nodeStatus(););

        return;  // RETURN
    }

    QueueHandleMap&    queueHandles = ns->queueHandles();
    QueueHandleMapIter queueIt      = queueHandles.find(event.queueId());
    if (queueIt == queueHandles.end()) {
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPushMessages,
            BALL_LOG_WARN << description()
                          << ": PUSH message for queue with unknown queueId ["
                          << event.queueId() << ", guid: " << event.guid()
                          << "] to target node: "
                          << event.clusterNode()->nodeDescription(););

        return;  // RETURN
    }

    // Build push event using PushEventBuilder.
    const QueueState& queueState = queueIt->second;

    // Update stats
    // TODO: Extract this and the version from 'mqba::ClientSession' to a
    //       function
    for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
         i < event.subQueueInfos().size();
         ++i) {
        StreamsMap::const_iterator subQueueCiter =
            queueState.d_subQueueInfosMap.findBySubscriptionId(
                event.subQueueInfos()[i].id());

        subQueueCiter->value().d_clientStats->onEvent(
            mqbstat::ClusterNodeStats::EventType::e_PUSH,
            event.blob() ? event.blob()->length() : 0);
    }

    bmqt::GenericResult::Enum rc = bmqt::GenericResult::e_SUCCESS;
    // TBD: groupId: also pass options to the 'PushEventBuilder::packMessage'
    // routine below.

    if (queueState.d_handle_p->queue()->isAtMostOnce()) {
        // If it's at most once, then we explicitly send the payload since it's
        // in-mem mode and there's been no replication (i.e. no preceding
        // STORAGE message).
        BSLS_ASSERT_SAFE(event.blob());
        rc = ns->clusterNode()->channel().writePush(
            event.blob(),
            event.queueId(),
            event.guid(),
            0,
            event.compressionAlgorithmType(),
            event.messagePropertiesInfo(),
            event.subQueueInfos());
    }
    else {
        int flags = 0;

        if (event.isOutOfOrderPush()) {
            bmqp::PushHeaderFlagUtil::setFlag(
                &flags,
                bmqp::PushHeaderFlags::e_OUT_OF_ORDER);
        }

        rc = ns->clusterNode()->channel().writePush(
            event.queueId(),
            event.guid(),
            flags,
            event.compressionAlgorithmType(),
            event.messagePropertiesInfo(),
            event.subQueueInfos());
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // This is non-recoverable, so we drop the PUSH msg and log it.

        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledDroppedPushMessages,
            BALL_LOG_ERROR << description() << ": dropping PUSH message "
                           << "[queueId: " << event.queueId() << ", guid: "
                           << event.guid() << "] to target node: "
                           << event.clusterNode()->nodeDescription()
                           << ", PushBuilder rc: " << rc << ".";);
    }
}

void Cluster::onRelayPushEvent(const mqbi::DispatcherPushEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay());

    // This relay-PUSH event is sent by primary (event.clusterNode()) to
    // replica (this) node.  Iterate over each message in the event and forward
    // it to appropriate remote queue.  Note that these PUSH msgs won't have
    // payloads.

    bmqp_ctrlmsg::NodeStatus::Value selfStatus =
        d_clusterData.membership().selfNodeStatus();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqp_ctrlmsg::NodeStatus::E_AVAILABLE != selfStatus)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Drop relay-PUSH messages coz self is going down
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPushMessages,
            BALL_LOG_WARN << "Dropping relay PUSH messages from node "
                          << event.clusterNode()->nodeDescription()
                          << ". Reason: self (replica node) not available."
                          << " Self node status: " << selfStatus;);
        return;  // RETURN
    }

    bmqp::Event rawEvent(event.blob().get(), d_allocator_p);
    BSLS_ASSERT_SAFE(rawEvent.isPushEvent());
    bdlma::LocalSequentialAllocator<1024> lsa(d_allocator_p);
    bmqp::PushMessageIterator pushIt(&d_clusterData.bufferFactory(), &lsa);
    rawEvent.loadPushMessageIterator(&pushIt, false);
    BSLS_ASSERT_SAFE(pushIt.isValid());

    int rc = 0;
    while (1 == (rc = pushIt.next())) {
        const bmqp::PushHeader& pushHeader = pushIt.header();

        mqbi::Queue* queue = d_clusterOrchestrator.queueHelper().lookupQueue(
            pushHeader.queueId());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queue == 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPushMessages,
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << description() << ": Received a relay-PUSH for unknown "
                    << "queue [queueId: " << pushHeader.queueId()
                    << ", guid: " << pushHeader.messageGUID()
                    << ", flags: " << pushHeader.flags() << "] from node "
                    << event.clusterNode()->nodeDescription()
                    << BMQTSK_ALARMLOG_END;);

            continue;  // CONTINUE
        }

        const bool atMostOnce      = queue->isAtMostOnce();
        const bool implicitPayload = bmqp::PushHeaderFlagUtil::isSet(
            pushHeader.flags(),
            bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);
        // We expect that when we're in at-most-once mode, the payload will be
        // explicit and when it's not in at-most-once mode, it will be
        // implicit.  We show an error message if that isn't the case.
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(implicitPayload ==
                                                  atMostOnce)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedPushMessages,
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << description()
                    << ": Received relay-PUSH with unexpected flag: "
                    << bsl::boolalpha << implicitPayload
                    << " == " << bsl::boolalpha << atMostOnce
                    << " for [queueId: " << pushHeader.queueId()
                    << ", guid: " << pushHeader.messageGUID()
                    << ", flags: " << pushHeader.flags() << "] from node "
                    << event.clusterNode()->nodeDescription()
                    << BMQTSK_ALARMLOG_END;);

            continue;  // CONTINUE
        }

        bsl::shared_ptr<bdlbb::Blob> appDataSp;
        bsl::shared_ptr<bdlbb::Blob> optionsSp;
        if (atMostOnce) {
            // If it's at-most-once delivery, forward the blob too.
            appDataSp = d_clusterData.blobSpPool().getObject();
            rc        = pushIt.loadApplicationData(appDataSp.get());
            BSLS_ASSERT_SAFE(rc == 0);
        }
        else if (pushIt.hasOptions()) {
            optionsSp = d_clusterData.blobSpPool().getObject();
            rc        = pushIt.loadOptions(optionsSp.get());
            BSLS_ASSERT_SAFE(0 == rc);
        }

        // TBD: groupId: conditionally load 'optionsSp' using something like
        // 'PushMessageIterator::loadOptions(&options.get()).

        queue->onPushMessage(pushHeader.messageGUID(),
                             appDataSp,
                             optionsSp,
                             bmqp::MessagePropertiesInfo(pushHeader),
                             pushHeader.compressionAlgorithmType(),
                             bmqp::PushHeaderFlagUtil::isSet(
                                 pushHeader.flags(),
                                 bmqp::PushHeaderFlags::e_OUT_OF_ORDER));
        // Note that passing the correct value of MessageProperties flag
        // above is not really needed, because self node (replica) will
        // retrieve the value of this flag from the storage when forwarding
        // this PUSH message downstream.
    }
}

void Cluster::onRecoveryStatus(
    int                                        status,
    const bsl::vector<unsigned int>&           primaryLeaseIds,
    const bmqsys::StatMonitorSnapshotRecorder& statRecorder)

{
    // exected by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::onRecoveryStatusDispatched,
                             this,
                             status,
                             primaryLeaseIds,
                             statRecorder),
        this);
}

void Cluster::onRecoveryStatusDispatched(
    int                                        status,
    const bsl::vector<unsigned int>&           primaryLeaseIds,
    const bmqsys::StatMonitorSnapshotRecorder& statRecorder)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (0 != status) {
        BALL_LOG_ERROR << description() << ": Stopping cluster as recovery "
                       << "failed with status: " << status;

        bmqex::SystemExecutor().post(
            bdlf::BindUtil::bind(&Cluster::terminate,
                                 this,
                                 mqbu::ExitCode::e_RECOVERY_FAILURE));
        return;  // RETURN
    }

    if (isFSMWorkflow()) {
        BSLS_ASSERT_SAFE(primaryLeaseIds.empty());
    }
    else {
        BSLS_ASSERT_SAFE(primaryLeaseIds.size() ==
                         d_state.partitions().size());
    }

    // Log recovery stats
    BALL_LOG_INFO_BLOCK
    {
        statRecorder.print(BALL_LOG_OUTPUT_STREAM, "RECOVERY");
    }

    BALL_LOG_INFO << description() << ": Recovery succeeded. Transitioning "
                  << "self node to AVAILABLE.";

    if (!isFSMWorkflow()) {
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << description()
                                   << ": retrieved primary leaseIds: \n";
            BALL_LOG_OUTPUT_STREAM << "    PartitionId    PrimaryLeaseId";
            for (size_t i = 0; i < primaryLeaseIds.size(); ++i) {
                BALL_LOG_OUTPUT_STREAM << "\n         " << i << "             "
                                       << primaryLeaseIds[i];
            }
        }

        // Set the current primaryLeaseId with the ones retrieved.  This is
        // necessary so that in case this node becomes the leader, it can use
        // appropriate leaseId values.

        for (size_t pid = 0; pid < primaryLeaseIds.size(); ++pid) {
            if (0 == d_state.partition(pid).primaryLeaseId()) {
                // This node hasn't heard from the leader yet.  Update
                // ClusterState partition info with the retrieved
                // primaryLeaseId.

                d_state.setPartitionPrimary(pid,
                                            primaryLeaseIds[pid],
                                            0);  // primaryNode
            }
            else if (d_state.partition(pid).primaryLeaseId() <
                     primaryLeaseIds[pid]) {
                BMQTSK_ALARMLOG_ALARM("CLUSTER")
                    << description() << " Partition [" << pid
                    << "]: self has higher retrieved leaseId ("
                    << primaryLeaseIds[pid]
                    << ") than the one notified by leader ("
                    << d_state.partition(pid).primaryLeaseId()
                    << "). Stopping the node as this node seems  to have more "
                    << "advanced view of the storage." << BMQTSK_ALARMLOG_END;

                bmqex::SystemExecutor().post(bdlf::BindUtil::bind(
                    &Cluster::terminate,
                    this,
                    mqbu::ExitCode::e_STORAGE_OUT_OF_SYNC));
                return;  // RETURN
            }
            // else: all good.
        }

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << description()
                                   << ": current primary leaseIds: \n";
            BALL_LOG_OUTPUT_STREAM << "    PartitionId    PrimaryLeaseId";
            for (size_t pid = 0; pid < d_state.partitions().size(); ++pid) {
                BALL_LOG_OUTPUT_STREAM
                    << "\n         " << pid << "             "
                    << d_state.partition(pid).primaryLeaseId();
            }
        }

        // Iterate over each recovered storage for each partition in the
        // StorageMgr and register the recovered queue uri/key info with
        // 'ClusterOrchestrator'.
        for (size_t pid = 0; pid < d_state.partitions().size(); ++pid) {
            bslma::ManagedPtr<mqbi::StorageManagerIterator> itMp;
            itMp = d_storageManager_mp->getIterator(pid);
            while (itMp && *itMp) {
                const bmqt::Uri uri(itMp->uri().canonical());
                BSLS_ASSERT_SAFE(itMp->storage()->partitionId() ==
                                 static_cast<int>(pid));

                AppInfos appIdInfos;
                itMp->storage()->loadVirtualStorageDetails(&appIdInfos);

                d_clusterOrchestrator.registerQueueInfo(
                    uri,
                    pid,
                    itMp->storage()->queueKey(),
                    appIdInfos,
                    false);  // Force-update?

                ++(*itMp);
            }
        }

        d_clusterOrchestrator.validateClusterStateLedger();
    }

    // Indicate queue helper to apply any buffered queue assignment advisories.
    // This must be done before setting self status to AVAILABLE, which will
    // proceed with any pending queue-open requests etc.

    BALL_LOG_INFO << description() << ": Transitioning to AVAILABLE, applying "
                  << "buffered queue [un]assignment advisories, if any.";

    d_clusterOrchestrator.processBufferedQueueAdvisories();

    d_clusterOrchestrator.transitionToAvailable();

    if (!isFSMWorkflow()) {
        // Indicate cluster state manager with leader sync if self is a passive
        // leader.  Note that this must be done after buffered queue advisories
        // have been applied.
        d_clusterOrchestrator.onRecoverySuccess();
    }
}

void Cluster::gcExpiredQueues()
{
    // executed by the *SCHEDULER* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::gcExpiredQueuesDispatched, this),
        this);
}

void Cluster::gcExpiredQueuesDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_clusterOrchestrator.queueHelper().gcExpiredQueues();
}

void Cluster::logSummaryState()
{
    // executed by the *SCHEDULER* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::logSummaryStateDispatched, this),
        this);
}

void Cluster::logSummaryStateDispatched() const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqu::MemOutStream                    os(&localAllocator);

    // Cluster Name
    os << description() << " summary: ";

    // Leader information
    os << "Leader is ";
    if (d_clusterData.electorInfo().leaderNodeId() ==
        mqbnet::Cluster::k_INVALID_NODE_ID) {
        os << "** NA **";
    }
    else {
        os << d_clusterData.membership()
                  .netCluster()
                  ->lookupNode(d_clusterData.electorInfo().leaderNodeId())
                  ->hostName()
           << "#"
           << d_clusterData.electorInfo().leaderMessageSequence().electorTerm()
           << ":"
           << d_clusterData.electorInfo()
                  .leaderMessageSequence()
                  .sequenceNumber()
           << " (status: " << d_clusterData.electorInfo().leaderStatus()
           << ")";
    }

    // Partitions information
    os << ", Partitions: [";
    const mqbc::ClusterState::PartitionsInfo& partitionsInfo =
        d_state.partitions();

    for (unsigned int i = 0; i < partitionsInfo.size(); ++i) {
        const ClusterStatePartitionInfo& pi = partitionsInfo[i];
        if (i != 0) {
            os << ", ";
        }
        os << i << ": ";
        if (!pi.primaryNode()) {
            os << "** NONE **";
        }
        else {
            os << pi.primaryNode()->hostName() << "#" << pi.primaryLeaseId();
        }
    }
    os << "]";

    BALL_LOG_INFO << os.str();
}

void Cluster::onProxyConnectionUpDispatched(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const bmqp_ctrlmsg::ClientIdentity&    identity,
    const bsl::string&                     description)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // Send node status advisory to just connected proxy
    if (bmqp::ProtocolUtil::hasFeature(
            bmqp::HighAvailabilityFeatures::k_FIELD_NAME,
            bmqp::HighAvailabilityFeatures::k_BROADCAST_TO_PROXIES,
            identity.features())) {
        // Send self's status to the node.
        bmqp_ctrlmsg::ControlMessage      controlMsg;
        bmqp_ctrlmsg::NodeStatusAdvisory& advisory =
            controlMsg.choice()
                .makeClusterMessage()
                .choice()
                .makeNodeStatusAdvisory();

        advisory.status() = d_clusterData.membership().selfNodeStatus();
        d_clusterData.messageTransmitter().sendMessage(controlMsg,
                                                       channel,
                                                       description);
    }
}

void Cluster::processResponseDispatched(
    const bmqp_ctrlmsg::ControlMessage& response,
    mqbnet::ClusterNode*                source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    int rc = d_clusterData.requestManager().processResponse(response);
    if (rc != 0 && response.choice().isOpenQueueResponseValue()) {
        // We received an openQueue *SUCCESS* response but we already timed
        // the request out; in order to keep consistent state across all
        // brokers, we must send a closeQueue (as far as upstream brokers
        // are concerned, this client has the queue opened, which is not
        // true, so roll it back).

        // Prepare the request.  We don't need to bind any response since
        // we don't need to do any processing, this request is really just
        // to inform upstream brokers; the internal state in the client was
        // already updated when the request time out was processed.

        RequestManagerType::RequestSp request =
            d_clusterData.requestManager().createRequest();

        bmqp_ctrlmsg::CloseQueue& closeQueue =
            request->request().choice().makeCloseQueue();

        closeQueue.handleParameters() = response.choice()
                                            .openQueueResponse()
                                            .originalRequest()
                                            .handleParameters();

        sendRequest(request, source, bsls::TimeInterval(60));
    }
}

void Cluster::loadNodesInfo(mqbcmd::NodeStatuses* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    ClusterNodeSessionMapConstIter cit =
        d_clusterData.membership().clusterNodeSessionMap().begin();
    bsl::vector<mqbcmd::ClusterNodeInfo>& nodes = out->nodes();
    nodes.reserve(d_clusterData.membership().clusterNodeSessionMap().size());
    for (; cit != d_clusterData.membership().clusterNodeSessionMap().end();
         ++cit) {
        BSLS_ASSERT_SAFE(cit->second);
        const mqbc::ClusterNodeSession& nodeSession = *(cit->second);
        nodes.resize(nodes.size() + 1);
        mqbcmd::ClusterNodeInfo& node = nodes.back();

        node.description() = nodeSession.clusterNode()->nodeDescription();
        if (nodeSession.clusterNode()->nodeId() !=
            d_clusterData.membership().netCluster()->selfNodeId()) {
            node.isAvailable().makeValue(
                nodeSession.clusterNode()->isAvailable());
        }

        BSLA_MAYBE_UNUSED const int rc = mqbcmd::NodeStatus::fromInt(
            &node.status(),
            nodeSession.nodeStatus());
        BSLS_ASSERT_SAFE(!rc && "Unsupported node status");

        const bsl::vector<int>& partitionVec = nodeSession.primaryPartitions();
        node.primaryForPartitionIds().resize(partitionVec.size());
        for (unsigned int i = 0; i < partitionVec.size(); ++i) {
            node.primaryForPartitionIds()[i] = partitionVec[i];
        }
    }
}

void Cluster::loadElectorInfo(mqbcmd::ElectorInfo* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BSLA_MAYBE_UNUSED int rc = mqbcmd::ElectorState::fromInt(
        &out->electorState(),
        d_clusterData.electorInfo().electorState());
    BSLS_ASSERT_SAFE(!rc && "Unsupported elector state");

    if (0 == d_clusterData.electorInfo().leaderNode()) {
        bmqu::MemOutStream os;
        os << "[ ** NA **, " << mqbnet::Cluster::k_INVALID_NODE_ID << "]";
        out->leaderNode() = os.str();
    }
    else {
        out->leaderNode() =
            d_clusterData.electorInfo().leaderNode()->nodeDescription();
    }

    mqbcmd::LeaderMessageSequence& leaderMessageSequence =
        out->leaderMessageSequence();
    leaderMessageSequence.electorTerm() =
        d_clusterData.electorInfo().leaderMessageSequence().electorTerm();
    leaderMessageSequence.sequenceNumber() =
        d_clusterData.electorInfo().leaderMessageSequence().sequenceNumber();
    rc = mqbcmd::LeaderStatus::fromInt(
        &out->leaderStatus(),
        d_clusterData.electorInfo().leaderStatus());
    BSLS_ASSERT_SAFE(!rc && "Unsupported leader status");
}

void Cluster::loadPartitionsInfo(mqbcmd::PartitionsInfo* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    const mqbc::ClusterState::PartitionsInfo& pInfo = d_state.partitions();
    bsl::vector<mqbcmd::PartitionInfo>&       partitions = out->partitions();
    partitions.resize(pInfo.size());
    for (unsigned int i = 0; i < pInfo.size(); ++i) {
        const ClusterStatePartitionInfo& pi = pInfo[i];
        partitions[i].numQueuesMapped()     = pi.numQueuesMapped();
        partitions[i].numActiveQueues()     = pi.numActiveQueues();
        if (pi.primaryNode()) {
            partitions[i].primaryNode().makeValue(
                pi.primaryNode()->nodeDescription());
        }
        partitions[i].primaryLeaseId() = pi.primaryLeaseId();
        BSLA_MAYBE_UNUSED const int rc = mqbcmd::PrimaryStatus::fromInt(
            &partitions[i].primaryStatus(),
            pi.primaryStatus());
        BSLS_ASSERT_SAFE(!rc && "Unsupported primary status");
    }
}

void Cluster::loadQueuesInfo(mqbcmd::StorageContent* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_clusterOrchestrator.queueHelper().loadQueuesInfo(out);
}

// CREATORS
Cluster::Cluster(const bslstl::StringRef&           name,
                 const mqbcfg::ClusterDefinition&   clusterConfig,
                 bslma::ManagedPtr<mqbnet::Cluster> netCluster,
                 const StatContextsMap&             statContexts,
                 mqbi::DomainFactory*               domainFactory,
                 mqbi::Dispatcher*                  dispatcher,
                 mqbnet::TransportManager*          transportManager,
                 StopRequestManagerType*            stopRequestsManager,
                 const mqbi::ClusterResources&      resources,
                 bslma::Allocator*                  allocator,
                 const mqbnet::Session::AdminCommandEnqueueCb& adminCb)
: d_allocator_p(allocator)
, d_allocators(d_allocator_p)
, d_isStarted(false)
, d_isStopping(false)
, d_clusterData(name,
                resources,
                clusterConfig,
                mqbcfg::ClusterProxyDefinition(allocator),
                netCluster,
                this,
                domainFactory,
                transportManager,
                statContexts.find("clusters")->second,
                statContexts,
                allocator)
, d_state(this, clusterConfig.partitionConfig().numPartitions(), allocator)
, d_storageManager_mp()
, d_clusterOrchestrator(d_clusterData.clusterConfig(),
                        this,
                        &d_clusterData,
                        &d_state,
                        d_allocators.get("ClusterOrchestrator"))
, d_clusterMonitor(&d_clusterData, &d_state, d_allocator_p)
, d_throttledFailedPutMessages(5000, 5)       // 5 logs per 5s interval
, d_throttledSkippedPutMessages(5000, 5)      // 5 logs per 5s interval
, d_throttledFailedAckMessages(5000, 5)       // 5 logs per 5s interval
, d_throttledDroppedAckMessages(5000, 5)      // 5 logs per 5s interval
, d_throttledFailedConfirmMessages(5000, 5)   // 5 logs per 5s interval
, d_throttledDroppedConfirmMessages(5000, 5)  // 5 logs per 5s interval
, d_throttledFailedPushMessages(5000, 5)      // 5 logs per 5s interval
, d_throttledDroppedPushMessages(5000, 5)     // 5 logs per 5s interval
, d_logSummarySchedulerHandle()
, d_queueGcSchedulerHandle()
, d_stopRequestsManager_p(stopRequestsManager)
, d_shutdownChain(allocator)
, d_adminCb(adminCb)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_allocator_p);
    mqbnet::Cluster* netCluster_p = d_clusterData.membership().netCluster();
    BSLS_ASSERT(netCluster_p && "NetCluster not set !");
    BSLS_ASSERT(resources.scheduler()->clockType() ==
                bsls::SystemClockType::e_MONOTONIC);
    BSLS_ASSERT_SAFE(d_clusterData.membership().selfNode() &&
                     "SelfNode not found in cluster!");

    d_clusterData.stats().setIsMember(true);

    // Create and populate meta data for each cluster node
    mqbc::ClusterMembership::ClusterNodeSessionMap& nodeSessionMap =
        d_clusterData.membership().clusterNodeSessionMap();

    NodeListIter nodeIter = netCluster_p->nodes().begin();
    NodeListIter endIter  = netCluster_p->nodes().end();

    for (; nodeIter != endIter; ++nodeIter) {
        mqbc::ClusterMembership::ClusterNodeSessionSp nodeSessionSp;
        nodeSessionSp.createInplace(d_allocator_p,
                                    this,
                                    *nodeIter,
                                    d_clusterData.identity().name(),
                                    d_clusterData.identity().identity(),
                                    d_allocator_p);
        nodeSessionSp->setNodeStatus(bmqp_ctrlmsg::NodeStatus::E_UNKNOWN);

        // Create stat context for each cluster node
        bmqst::StatContextConfiguration config((*nodeIter)->hostName());

        StatContextMp statContextMp =
            d_clusterData.clusterNodesStatContext()->addSubcontext(config);
        StatContextSp statContextSp(statContextMp, d_allocator_p);
        nodeSessionSp->statContext() = statContextSp;

        nodeSessionMap.insert(bsl::make_pair(*nodeIter, nodeSessionSp));

        if (netCluster_p->selfNodeId() == (*nodeIter)->nodeId()) {
            d_clusterData.membership().setSelfNodeSession(nodeSessionSp.get());
        }
    }

    d_clusterOrchestrator.updateDatumStats();

    BSLS_ASSERT_SAFE(d_clusterData.membership().selfNode());
    BSLS_ASSERT_SAFE(d_clusterData.membership().selfNodeSession());

    // Register to the dispatcher
    mqbi::Dispatcher::ProcessorHandle processor = dispatcher->registerClient(
        this,
        mqbi::DispatcherClientType::e_CLUSTER);

    d_clusterData.requestManager().setExecutor(dispatcher->executor(this));

    BALL_LOG_INFO << "Created Cluster: "
                  << "[name: '" << d_clusterData.identity().name()
                  << "', dispatcherProcessor: " << processor << "]";
}

Cluster::~Cluster()
{
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "stop() must be called before destruction");

    BALL_LOG_INFO << "Cluster '" << description() << "': destructor";

    // Unregister from the dispatcher
    dispatcher()->unregisterClient(this);

    // Explicitly clear cluster state object to avoid any object lifetime issue
    // at shutdown.
    d_state.clear();

    d_shutdownChain.stop();
    d_shutdownChain.join();
}

// MANIPULATORS
int Cluster::start(bsl::ostream& errorDescription)
{
    int rc;
    dispatcher()->execute(bdlf::BindUtil::bind(&Cluster::startDispatched,
                                               this,
                                               &errorDescription,
                                               &rc),
                          this);

    // We synchronize twice here.  1st synchronize() is needed to ensure that
    // 'startDispatched' has been executed.  The 2nd synchronize() is needed to
    // ensure that any event that 'startDispatched' may have enqueued back to
    // cluster's dispatcher thread has also been executed.  'startDispatched'
    // was (indirectly) enqueuing an event back in cluster's dispatcher thread
    // when it was stopping the elector (as a result of a failure of some part
    // of 'startDispatched' routine).  While stopping, elector invokes elector-
    // state-callback in elector's internal scheduler thread.  But cluster (or
    // ClusterStateMgr) ends up enqueuing an event back to the dispatcher
    // thread in this elector-state-callback.  The 2nd call to 'synchronize'
    // below guarantees that this event has been executed.

    // Another way to think of this is that 'startDispatched' is async, which
    // itself performs some async operations, so we need to synchronize twice.

    dispatcher()->synchronize(this);

    dispatcher()->synchronize(this);

    return rc;
}

void Cluster::initiateShutdown(const VoidFunctor& callback,
                               bool               supportShutdownV2)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!dispatcher()->inDispatcherThread(this));
    // Deadlock detection (because of the 'synchronize' call below, we
    // can't execute from any of the cluster's dispatcher thread).

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::initiateShutdownDispatched,
                             this,
                             callback,
                             supportShutdownV2),
        this);

    // Wait for above event to complete.  This is needed because
    // 'initiateShutdownDispatched()' notifies its peers that this node is now
    // stopping.  'initiateShutdown()' is immediately followed by a call to
    // Cluster::close(), which closes local storage, drops connections with
    // peers, etc.  We want to ensure that events in
    // 'initiateShutdownDispatched()' are completed before those in
    // Cluster::stop().
    dispatcher()->synchronize(this);
}

void Cluster::stop()
{
    // executed by *ANY* thread

    if (!d_isStarted) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Stopping Cluster: [name: '" << name() << "']";

    d_isStarted = false;

    dispatcher()->execute(bdlf::BindUtil::bind(&Cluster::stopDispatched, this),
                          this);

    // We need to synchronize twice.  See notes in 'start' for explanation.

    dispatcher()->synchronize(this);

    dispatcher()->synchronize(this);
}

void Cluster::terminate(mqbu::ExitCode::Enum reason)
{
    // executed by *ANY* NON-DISPATCHER thread
    BSLS_ASSERT_SAFE(!dispatcher()->inDispatcherThread(this));

    bslmt::Latch latch(1);
    initiateShutdown(bdlf::BindUtil::bind(&bslmt::Latch::arrive, &latch));

    latch.wait();

    stop();

    mqbu::ExitUtil::shutdown(reason);  // EXIT
}

void Cluster::registerStateObserver(mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbc::ClusterState::registerObserver,
                             &d_state,
                             observer),
        this);
}

void Cluster::unregisterStateObserver(mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&mqbc::ClusterState::unregisterObserver,
                             &d_state,
                             observer),
        this);
}

void Cluster::openQueue(
    const bmqt::Uri&                                          uri,
    mqbi::Domain*                                             domain,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const mqbi::Cluster::OpenQueueCallback&                   callback)
{
    // executed by *ANY* THREAD

    // Enqueue an event to process in the dispatcher thread
    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterQueueHelper::openQueue,
                             &d_clusterOrchestrator.queueHelper(),
                             uri,
                             domain,
                             handleParameters,
                             clientContext,
                             callback),
        this);
}

void Cluster::configureQueue(
    mqbi::Queue*                                       queue,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    unsigned int                                       upstreamSubQueueId,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(queue));

    d_clusterOrchestrator.queueHelper().configureQueue(queue,
                                                       streamParameters,
                                                       upstreamSubQueueId,
                                                       callback);
}

void Cluster::configureQueue(
    mqbi::Queue*                                 queue,
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    unsigned int                                 upstreamSubQueueId,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(queue));

    d_clusterOrchestrator.queueHelper().configureQueue(queue,
                                                       handleParameters,
                                                       upstreamSubQueueId,
                                                       callback);
}

void Cluster::onQueueHandleCreated(mqbi::Queue*     queue,
                                   const bmqt::Uri& uri,
                                   bool             handleCreated)
{
    // executed by *ANY* thread

    d_clusterOrchestrator.queueHelper().onQueueHandleCreated(queue,
                                                             uri,
                                                             handleCreated);
}

void Cluster::onQueueHandleDestroyed(mqbi::Queue* queue, const bmqt::Uri& uri)
{
    // executed by *ANY* thread

    d_clusterOrchestrator.queueHelper().onQueueHandleDestroyed(queue, uri);
}

void Cluster::onDomainReconfigured(const mqbi::Domain&     domain,
                                   const mqbconfm::Domain& oldDefn,
                                   const mqbconfm::Domain& newDefn)
{
    if (!oldDefn.mode().isFanoutValue()) {
        return;  // RETURN
    }

    // Compute list of added and removed App IDs.
    bsl::unordered_set<bsl::string> oldCfgAppIds(
        oldDefn.mode().fanout().appIDs().cbegin(),
        oldDefn.mode().fanout().appIDs().cend(),
        d_allocator_p);
    bsl::unordered_set<bsl::string> newCfgAppIds(
        newDefn.mode().fanout().appIDs().cbegin(),
        newDefn.mode().fanout().appIDs().cend(),
        d_allocator_p);

    bsl::unordered_set<bsl::string> addedIds, removedIds;
    mqbc::StorageUtil::loadAddedAndRemovedEntries(&addedIds,
                                                  &removedIds,
                                                  oldCfgAppIds,
                                                  newCfgAppIds);

    // TODO: This should be one call - one QueueUpdateAdvisory for all Apps
    bsl::unordered_set<bsl::string>::const_iterator it = addedIds.cbegin();
    for (; it != addedIds.cend(); ++it) {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::registerAppId,
                                 &d_clusterOrchestrator,
                                 *it,
                                 bsl::ref(domain)),
            this);
    }
    for (it = removedIds.cbegin(); it != removedIds.cend(); ++it) {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::unregisterAppId,
                                 &d_clusterOrchestrator,
                                 *it,
                                 bsl::ref(domain)),
            this);
    }
}

int Cluster::processCommand(mqbcmd::ClusterResult*        result,
                            const mqbcmd::ClusterCommand& command)
{
    // executed by *ANY* thread

    // Note that we bind 'cmd' & 'os' by their addresses because bdlf::bind
    // does not support binding by references.  While passing streams by
    // addresses is ungainly, it works, and the stream objects are guaranteed
    // to outlive the callback because we synchronize the callback.

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::processCommandDispatched,
                             this,
                             result,
                             bsl::ref(command)),
        this);
    dispatcher()->synchronize(this);

    return 0;
}

void Cluster::processControlMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the *IO* thread
    typedef bmqp_ctrlmsg::ControlMessageChoice MsgChoice;  // shortcut

    switch (message.choice().selectionId()) {
    case MsgChoice::SELECTION_ID_STATUS:
    case MsgChoice::SELECTION_ID_OPEN_QUEUE_RESPONSE:
    case MsgChoice::SELECTION_ID_CONFIGURE_STREAM_RESPONSE:
    case MsgChoice::SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE:
    case MsgChoice::SELECTION_ID_CLOSE_QUEUE_RESPONSE: {
        BSLS_ASSERT_SAFE(!message.rId().isNull());

        dispatcher()->execute(
            bdlf::BindUtil::bind(&Cluster::processResponseDispatched,
                                 this,
                                 message,
                                 source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_OPEN_QUEUE: {
        mqbc::ClusterNodeSession* node =
            d_clusterData.membership().getClusterNodeSession(source);
        BSLS_ASSERT_SAFE(node);
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterQueueHelper::processPeerOpenQueueRequest,
                &d_clusterOrchestrator.queueHelper(),
                message,
                node),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_CONFIGURE_STREAM:
    case MsgChoice::SELECTION_ID_CONFIGURE_QUEUE_STREAM: {
        mqbc::ClusterNodeSession* node =
            d_clusterData.membership().getClusterNodeSession(source);
        BSLS_ASSERT_SAFE(node);
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterQueueHelper::processPeerConfigureStreamRequest,
                &d_clusterOrchestrator.queueHelper(),
                message,
                node),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_CLOSE_QUEUE: {
        mqbc::ClusterNodeSession* node =
            d_clusterData.membership().getClusterNodeSession(source);
        BSLS_ASSERT_SAFE(node);
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterQueueHelper::processPeerCloseQueueRequest,
                &d_clusterOrchestrator.queueHelper(),
                message,
                node),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_CLUSTER_MESSAGE: {
        processClusterControlMessage(message, source);
    } break;
    case MsgChoice::SELECTION_ID_DISCONNECT:
    case MsgChoice::SELECTION_ID_DISCONNECT_RESPONSE: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << description() << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_ADMIN_COMMAND: {
        // Assume this is a rerouted command, so just execute it on the
        // application
        const bmqp_ctrlmsg::AdminCommand& adminCommand =
            message.choice().adminCommand();
        const bsl::string& cmd = adminCommand.command();
        d_adminCb(source->hostName(),
                  cmd,
                  bdlf::BindUtil::bind(&Cluster::onProcessedAdminCommand,
                                       this,
                                       source,
                                       message,
                                       bdlf::PlaceHolders::_1,   // rc
                                       bdlf::PlaceHolders::_2),  // response
                  true);  // from reroute
    } break;
    case MsgChoice::SELECTION_ID_ADMIN_COMMAND_RESPONSE: {
        requestManager().processResponse(message);
    } break;
    case MsgChoice::SELECTION_ID_UNDEFINED:
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << description() << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
        BSLS_ASSERT_SAFE(message.choice().selectionId() !=
                         MsgChoice::SELECTION_ID_UNDEFINED);
        // We should never receive an 'UNDEFINED' message, but while
        // introducing new features, we may receive unknown messages.
    } break;  // BREAK
    }
}

void Cluster::processClusterSyncRequest(
    const bmqp_ctrlmsg::ControlMessage& request,
    mqbnet::ClusterNode*                requester)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(request.choice().isClusterMessageValue());
    BSLS_ASSERT_SAFE(request.choice()
                         .clusterMessage()
                         .choice()
                         .isClusterSyncRequestValue());

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          response(&localAllocator);
    response.rId() = request.rId();
    response.choice().makeClusterMessage().choice().makeClusterSyncResponse();

    // Send the response to the requester
    d_clusterData.messageTransmitter().sendMessage(response, requester);
}

void Cluster::processClusterControlMessage(
    const bmqp_ctrlmsg::ControlMessage& message,
    mqbnet::ClusterNode*                source)
{
    // executed by the *IO* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message.choice().isClusterMessageValue());

    const bmqp_ctrlmsg::ClusterMessage& clusterMessage =
        message.choice().clusterMessage();

    typedef bmqp_ctrlmsg::ClusterMessageChoice MsgChoice;  // shortcut

    switch (clusterMessage.choice().selectionId()) {
    case MsgChoice::SELECTION_ID_LEADER_SYNC_STATE_QUERY_RESPONSE:
    case MsgChoice::SELECTION_ID_LEADER_SYNC_DATA_QUERY_RESPONSE:
    case MsgChoice::SELECTION_ID_STORAGE_SYNC_RESPONSE:
    case MsgChoice::SELECTION_ID_PARTITION_SYNC_STATE_QUERY_RESPONSE:
    case MsgChoice::SELECTION_ID_PARTITION_SYNC_DATA_QUERY_RESPONSE:
    case MsgChoice::SELECTION_ID_CLUSTER_SYNC_RESPONSE: {
        // NOTE: that we cant simply just check if the msg has an id, because
        //       in cluster, it can receive requests which will have an id; so
        //       only messages that are response type should be sent to the
        //       requestManager for processing.

        // A response must have an associated request Id
        BSLS_ASSERT_SAFE(!message.rId().isNull());

        dispatcher()->execute(
            bdlf::BindUtil::bind(&Cluster::processResponseDispatched,
                                 this,
                                 message,
                                 source),
            this);
    } break;  // BREAK

    case MsgChoice::SELECTION_ID_STOP_RESPONSE: {
        BALL_LOG_INFO << description() << ": processStopResponse: " << message;
        d_stopRequestsManager_p->processResponse(message);
    } break;
    case MsgChoice::SELECTION_ID_PARTITION_PRIMARY_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processPartitionPrimaryAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::processLeaderAdvisory,
                                 &d_clusterOrchestrator,
                                 message,
                                 source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processQueueAssignmentAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_NODE_STATUS_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processNodeStatusAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_SYNC_STATE_QUERY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processLeaderSyncStateQuery,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_LEADER_SYNC_DATA_QUERY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processLeaderSyncDataQuery,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_ASSIGNMENT_REQUEST: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processQueueAssignmentRequest,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_STORAGE_SYNC_REQUEST: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processStorageSyncRequest,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PARTITION_SYNC_STATE_QUERY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processPartitionSyncStateRequest,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PARTITION_SYNC_DATA_QUERY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processPartitionSyncDataRequest,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PARTITION_SYNC_DATA_QUERY_STATUS: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processPartitionSyncDataRequestStatus,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PRIMARY_STATUS_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processPrimaryStatusAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_CLUSTER_SYNC_REQUEST: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&Cluster::processClusterSyncRequest,
                                 this,
                                 message,
                                 source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UN_ASSIGNMENT_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processQueueUnAssignmentAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_QUEUE_UNASSIGNED_ADVISORY: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processQueueUnassignedAdvisory,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_STATE_NOTIFICATION: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processStateNotification,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_STOP_REQUEST: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::processStopRequest,
                                 &d_clusterOrchestrator,
                                 message,
                                 source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_CLUSTER_STATE_F_S_M_MESSAGE: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                &ClusterOrchestrator::processClusterStateFSMMessage,
                &d_clusterOrchestrator,
                message,
                source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_PARTITION_MESSAGE: {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterOrchestrator::processPartitionMessage,
                                 &d_clusterOrchestrator,
                                 message,
                                 source),
            this);
    } break;  // BREAK
    case MsgChoice::SELECTION_ID_UNDEFINED:
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << description() << ": unexpected clusterMessage:" << message
            << BMQTSK_ALARMLOG_END;
    } break;  // BREAK
    }
}

void Cluster::processEvent(const bmqp::Event&   event,
                           mqbnet::ClusterNode* source)
{
    // executed by the *IO* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(source);

    // Helper macro to dispatch event of the specified type 'T' with isRelay
    // set to the value specified in 'R'.
#define DISPATCH_EVENT(T, R)                                                  \
    {                                                                         \
        mqbi::DispatcherEvent*       _evt = dispatcher()->getEvent(this);     \
        bsl::shared_ptr<bdlbb::Blob> _blobSp =                                \
            d_clusterData.blobSpPool().getObject();                           \
        *_blobSp = *(event.blob());                                           \
        (*_evt)                                                               \
            .setType(T)                                                       \
            .setIsRelay(R)                                                    \
            .setSource(this)                                                  \
            .setBlob(_blobSp)                                                 \
            .setClusterNode(source);                                          \
        dispatcher()->dispatchEvent(_evt, this);                              \
    }                                                                         \
    while (0)

    switch (event.type()) {
    case bmqp::EventType::e_CONTROL: {
        if (isLocal()) {
            // We shouldn't be getting any intra-cluster messages if its a
            // local cluster.
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << description() << ": received a 'CONTROL' event from node "
                << source->nodeDescription() << " in local cluster setup."
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          message(&localAllocator);

        int rc = event.loadControlEvent(&message);
        if (rc != 0) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << description() << ": failed to decode 'CLUSTER' message from"
                << " node " << source->nodeDescription() << ", rc: " << rc
                << "\n"
                << bmqu::BlobStartHexDumper(event.blob())
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        BALL_LOG_TRACE << description()
                       << ": Received a control message: " << message
                       << " from " << source->nodeDescription();

        rc = bmqp::ControlMessageUtil::validate(message);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            // Malformed control message should be rejected
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_ERROR << "#CORRUPTED_CONTROL_MESSAGE " << description()
                           << ": Received malformed control message from node "
                           << source->nodeDescription()
                           << " [reason: 'malformed fields', rc: " << rc
                           << ", message: " << message << "]";
            return;  // RETURN
        }

        processControlMessage(message, source);
    } break;  // BREAK
    case bmqp::EventType::e_ELECTOR: {
        d_clusterOrchestrator.processElectorEvent(event, source);
    } break;  // BREAK
    case bmqp::EventType::e_PUT: {
        // This event arrives from a replica to this node, which should be the
        // primary of the partition of the queue to which this PUT event
        // belongs.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_PUT, false);
    } break;  // BREAK
    case bmqp::EventType::e_CONFIRM: {
        // This event arrives from a replica to this node, which should be the
        // primary of the partition of the queue to which this CONFIRM event
        // belongs.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_CONFIRM, false);
    } break;  // BREAK
    case bmqp::EventType::e_REJECT: {
        // This event arrives from a replica to this node, which should be the
        // primary of the partition of the queue to which this REJECT event
        // belongs.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_REJECT, false);
    } break;  // BREAK
    case bmqp::EventType::e_PUSH: {
        // This event arrives from primary to replica, and hence is a relay
        // event.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_PUSH, true);
    } break;  // BREAK
    case bmqp::EventType::e_ACK: {
        // This event arrives from primary to replica, and hence is a relay
        // event.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_ACK, true);
    } break;  // BREAK
    case bmqp::EventType::e_CLUSTER_STATE: {
        if (isLocal()) {
            // We shouldn't be getting any intra-cluster messages if its a
            // local cluster.
            BMQTSK_ALARMLOG_ALARM("CLUSTER")
                << description()
                << ": received a 'CLUSTER_STATE' event from node "
                << source->nodeDescription() << " in local cluster setup."
                << BMQTSK_ALARMLOG_END;
            return;  // RETURN
        }

        DISPATCH_EVENT(mqbi::DispatcherEventType::e_CLUSTER_STATE, false);
    } break;
    case bmqp::EventType::e_STORAGE: {
        // Storage event arrives from primary to replica/replication nodes.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_STORAGE, false);
    } break;  // BREAK
    case bmqp::EventType::e_PARTITION_SYNC: {
        // PartitionSync event may arrive from a passive primary to replicas,
        // or from the chosen syncing peer to the passive primary.  We
        // currently don't have a dispatcher event type for
        // EventType::e_PARTITION_SYNC.  So we overload
        // DispatcherEventType::e_STORAGE.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_STORAGE, false);
    } break;  // BREAK
    case bmqp::EventType::e_RECOVERY: {
        // This event arrives from a peer cluster node.
        DISPATCH_EVENT(mqbi::DispatcherEventType::e_RECOVERY, false);
    } break;  // BREAK
    case bmqp::EventType::e_HEARTBEAT_REQ:
    case bmqp::EventType::e_HEARTBEAT_RSP: {
        BSLS_ASSERT_SAFE(false && "Heartbeat are handled at mqbnet layer");
    } break;
    case bmqp::EventType::e_REPLICATION_RECEIPT: {
        // Receipt event arrives from replication nodes to primary.
        d_storageManager_mp->processReceiptEvent(event, source);
    } break;  // BREAK
    case bmqp::EventType::e_UNDEFINED:
    default: {
        BMQTSK_ALARMLOG_ALARM("CLUSTER")
            << description() << ": Received bmqp event of type '"
            << event.type() << "' which is currently not handled."
            << BMQTSK_ALARMLOG_END;
        BSLS_ASSERT_SAFE(event.type() != bmqp::EventType::e_UNDEFINED);
        // We should never receive an 'UNDEFINED' event, but while
        // introducing new features, we may receive unknown event types.
    } break;  // BREAK
    }

#undef DISPATCH_EVENT
}

void Cluster::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BALL_LOG_TRACE << description() << ": processing dispatcher event '"
                   << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent& realEvent =
            *event.asCallbackEvent();
        BSLS_ASSERT_SAFE(realEvent.callback().hasCallback());
        realEvent.callback()();
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_PUT: {
        const mqbi::DispatcherPutEvent& realEvent = *event.asPutEvent();
        if (realEvent.isRelay()) {
            // We pass a parent object event here because the implementation
            // uses `source()` field from this parent object
            onRelayPutEvent(event);
        }
        else {
            onPutEvent(realEvent);
        }
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_ACK: {
        const mqbi::DispatcherAckEvent& realEvent = *event.asAckEvent();
        if (realEvent.isRelay()) {
            onRelayAckEvent(realEvent);
        }
        else {
            onAckEvent(realEvent);
        }
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_CONFIRM: {
        const mqbi::DispatcherConfirmEvent& realEvent =
            *event.asConfirmEvent();
        if (realEvent.isRelay()) {
            onRelayConfirmEvent(realEvent);
        }
        else {
            onConfirmEvent(realEvent);
        }
    } break;
    case mqbi::DispatcherEventType::e_REJECT: {
        const mqbi::DispatcherRejectEvent& realEvent = *event.asRejectEvent();
        if (realEvent.isRelay()) {
            onRelayRejectEvent(realEvent);
        }
        else {
            onRejectEvent(realEvent);
        }
    } break;
    case mqbi::DispatcherEventType::e_CLUSTER_STATE: {
        const mqbi::DispatcherClusterStateEvent& clusterStateEvt =
            *event.asClusterStateEvent();
        d_clusterOrchestrator.processClusterStateEvent(clusterStateEvt);
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_STORAGE: {
        const mqbi::DispatcherStorageEvent& storageEvt =
            *event.asStorageEvent();
        d_storageManager_mp->processStorageEvent(storageEvt);
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_RECOVERY: {
        const mqbi::DispatcherRecoveryEvent& recoveryEvt =
            *event.asRecoveryEvent();
        d_storageManager_mp->processRecoveryEvent(recoveryEvt);
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_PUSH: {
        const mqbi::DispatcherPushEvent& realEvent = *event.asPushEvent();
        if (realEvent.isRelay()) {
            onRelayPushEvent(realEvent);
        }
        else {
            onPushEvent(realEvent);
        }
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT:
    case mqbi::DispatcherEventType::e_CONTROL_MSG:
    case mqbi::DispatcherEventType::e_DISPATCHER:
    case mqbi::DispatcherEventType::e_UNDEFINED:
    default:
        BALL_LOG_ERROR << "Received dispatcher event of unexpected type";
        break;  // BREAK
    };
}

void Cluster::flush()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
}

void Cluster::onNodeStateChange(mqbnet::ClusterNode* node, bool isAvailable)
{
    // executed by the *IO* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterOrchestrator::processNodeStateChangeEvent,
                             &d_clusterOrchestrator,
                             node,
                             isAvailable),
        this);
}

void Cluster::onProxyConnectionUp(
    const bsl::shared_ptr<bmqio::Channel>& channel,
    const bmqp_ctrlmsg::ClientIdentity&    identity,
    const bsl::string&                     description)
{
    // executed by the *IO* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::onProxyConnectionUpDispatched,
                             this,
                             channel,
                             identity,
                             description),
        this);
}

void Cluster::onNodeHighWatermark(mqbnet::ClusterNode* node)
{
    // executed by the *IO* thread
    // When writting to the channel, the 'sendPacket()' returns limit if it
    // goes beyond the watermark provided at write; and this HighWatermark
    // notification is only emitted if it goes beyond the session's
    // highWatermark.  Ideally we should not hit this notification because we
    // are using "large enough" value for the high water mark, but if we hit
    // this, it means we have lost that packet and so, we alarm on it.  (Recall
    // that we don't use a 'channel buffer queue' in this component, unlike
    // mqba::ClientSession).

    BMQTSK_ALARMLOG_ALARM("CLUSTER")
        << description() << ": Channel HighWatermark hit for node "
        << node->nodeDescription() << BMQTSK_ALARMLOG_END;
}

void Cluster::onNodeLowWatermark(
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* node)
{
    // executed by the *IO* thread

    // TBD: Enqueue an event to be processed in the cluster's DISPATCHER thread
    //      to resume flushing data from the channelBufferQueue of the 'node'.

    // dispatcher()->execute(bdlf::BindUtil::bind(
    //                                       &Cluster::flushChannelBufferQueue,
    //                                       this,
    //                                       node),
    //                       this);
}

void Cluster::onClusterLeader(mqbnet::ClusterNode*                node,
                              mqbc::ElectorInfoLeaderStatus::Enum status)
{
    if (status == mqbc::ElectorInfoLeaderStatus::e_PASSIVE) {
        return;  // RETURN
    }

    d_clusterOrchestrator.updateDatumStats();
    d_clusterData.stats().setIsLeader(
        d_clusterData.membership().selfNode() == node
            ? mqbstat::ClusterStats::LeaderStatus::e_LEADER
            : mqbstat::ClusterStats::LeaderStatus::e_FOLLOWER);
}

void Cluster::onLeaderPassiveThreshold()
{
    if (d_clusterData.electorInfo().isSelfLeader()) {
        // Self is the passive leader, so there is nothing to be done here
        // (self should eventually transition to active upon completion of
        // synchronization).
        return;  // RETURN
    }

    if ((d_clusterData.electorInfo().leaderNode() == 0) ||
        (d_clusterData.electorInfo().leaderStatus() ==
         mqbc::ElectorInfoLeaderStatus::e_ACTIVE)) {
        // No leader or there's an active leader, so there is nothing for us to
        // do.
        return;  // RETURN
    }

    bmqp_ctrlmsg::ControlMessage ctlMsg;
    ctlMsg.choice()
        .makeClusterMessage()
        .choice()
        .makeStateNotification()
        .choice()
        .makeLeaderPassive();

    d_clusterData.messageTransmitter().sendMessage(
        ctlMsg,
        d_clusterData.electorInfo().leaderNode());
}

void Cluster::onFailoverThreshold()
{
    const mqbcfg::ClusterMonitorConfig& monitorConfig =
        d_clusterData.clusterConfig().clusterMonitorConfig();
    bmqu::MemOutStream os;

    os << "'" << d_clusterData.identity().name()
       << "' has not completed the failover process above the threshold "
       << "amount of "
       << bmqu::PrintUtil::prettyTimeInterval(
              monitorConfig.thresholdFailover() *
              bdlt::TimeUnitRatio::k_NS_PER_S)
       << ". There are "
       << d_clusterOrchestrator.queueHelper().numPendingReopenQueueRequests()
       << " pending reopen-queue requests.\n";
    // Log only a summary in the alarm
    printClusterStateSummary(os, 0, 4);
    BMQTSK_ALARMLOG_PANIC("CLUSTER") << os.str() << BMQTSK_ALARMLOG_END;
}

void Cluster::onProcessedAdminCommand(
    mqbnet::ClusterNode*                source,
    const bmqp_ctrlmsg::ControlMessage& adminCommandCtrlMsg,
    int                                 rc,
    const bsl::string&                  result)
{
    if (rc != 0) {
        BALL_LOG_ERROR << "Error processing routed command [rc: " << rc << "] "
                       << result;
    }

    // Regardless of rc, send the admin command response back to the source for
    // the client to read.
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          response(&localAllocator);

    response.rId() = adminCommandCtrlMsg.rId().value();
    response.choice().makeAdminCommandResponse();

    response.choice().adminCommandResponse().text() = result;

    d_clusterData.messageTransmitter().sendMessageSafe(response, source);
}

void Cluster::loadClusterStatus(mqbcmd::ClusterResult* result)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    mqbcmd::ClusterStatus& clusterStatus = result->makeClusterStatus();

    clusterStatus.name()        = d_clusterData.identity().name();
    clusterStatus.description() = description();
    clusterStatus.selfNodeDescription() =
        d_clusterData.membership().selfNode()->nodeDescription();
    clusterStatus.isHealthy() = d_clusterMonitor.isHealthy();

    loadNodesInfo(&clusterStatus.nodeStatuses());
    loadElectorInfo(&clusterStatus.electorInfo());
    loadPartitionsInfo(&clusterStatus.partitionsInfo());
    loadQueuesInfo(&clusterStatus.queuesInfo());

    mqbcmd::StorageCommand cmd;
    cmd.makeSummary();
    mqbcmd::StorageResult storageResult;
    d_storageManager_mp->processCommand(&storageResult, cmd);
    clusterStatus.clusterStorageSummary() =
        storageResult.clusterStorageSummary();
}

void Cluster::printClusterStateSummary(bsl::ostream& out,
                                       int           level,
                                       int           spacesPerLevel) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    mqbcmd::NodeStatuses nodeStatuses;
    loadNodesInfo(&nodeStatuses);
    out << "\n\n";
    mqbcmd::ElectorInfo electorInfo;
    loadElectorInfo(&electorInfo);
    out << "\n\n";
    mqbcmd::PartitionsInfo partitionsInfo;
    loadPartitionsInfo(&partitionsInfo);

    mqbcmd::Result result;
    result.makeNodeStatuses(nodeStatuses);
    mqbcmd::HumanPrinter::print(out, result, level, spacesPerLevel);
    result.makeElectorInfo(electorInfo);
    mqbcmd::HumanPrinter::print(out, result, level, spacesPerLevel);
    result.makePartitionsInfo(partitionsInfo);
    mqbcmd::HumanPrinter::print(out, result, level, spacesPerLevel);
}

bool Cluster::isCSLModeEnabled() const
{
    return d_clusterData.clusterConfig()
        .clusterAttributes()
        .isCSLModeEnabled();
}

bool Cluster::isFSMWorkflow() const
{
    return d_clusterData.clusterConfig().clusterAttributes().isFSMWorkflow();
}

bmqt::GenericResult::Enum
Cluster::sendRequest(const Cluster::RequestManagerType::RequestSp& request,
                     mqbnet::ClusterNode*                          target,
                     bsls::TimeInterval                            timeout)
{
    // executed by the cluster *DISPATCHER* thread or the *QUEUE DISPATCHER*
    // thread
    //
    // It is safe to invoke this function from non-cluster threads because
    // `d_clusterData.electorInfo().leaderNodeId()` is only modified upon
    // elector transition, while `d_clusterData.requestManager().sendRequest`
    // is guarded by a mutex on its own.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(target != 0 ||
                     d_clusterData.electorInfo().leaderNodeId() !=
                         mqbnet::Elector::k_INVALID_NODE_ID);
    // If target == 0, this means send to leader, make sure there is a
    // leader.

    if (target == 0) {
        target = d_clusterData.membership().netCluster()->lookupNode(
            d_clusterData.electorInfo().leaderNodeId());
    }

    BSLS_ASSERT_SAFE(target);
    request->setGroupId(target->nodeId());

    return d_clusterData.requestManager().sendRequest(
        request,
        bdlf::BindUtil::bind(&mqbnet::ClusterNode::write,
                             target,
                             bdlf::PlaceHolders::_1,
                             bmqp::EventType::e_CONTROL),
        target->nodeDescription(),
        timeout);
}

void Cluster::processResponse(const bmqp_ctrlmsg::ControlMessage& response)
{
    // executed by the *IO* thread

    // A response must have an associated request Id
    BSLS_ASSERT_SAFE(!response.rId().isNull());

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Cluster::processResponseDispatched,
                             this,
                             response,
                             static_cast<mqbnet::ClusterNode*>(0)),  // source
        this);
}

void Cluster::getPrimaryNodes(int*          rc,
                              bsl::ostream& errorDescription,
                              bsl::vector<mqbnet::ClusterNode*>* nodes,
                              bool* isSelfPrimary) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(rc);
    BSLS_ASSERT_SAFE(nodes);
    BSLS_ASSERT_SAFE(isSelfPrimary);
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    enum RcEnum {
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    const mqbc::ClusterState::PartitionsInfo& partitionsInfo =
        d_state.partitions();

    nodes->clear();
    *isSelfPrimary = false;

    for (mqbc::ClusterState::PartitionsInfo::const_iterator pit =
             partitionsInfo.begin();
         pit != partitionsInfo.end();
         pit++) {
        if (pit->primaryStatus() !=
            // TODO: Handle this case (will want to buffer)
            bmqp_ctrlmsg::PrimaryStatus::E_ACTIVE) {
            BALL_LOG_WARN << "While collecting primary nodes: "
                          << "primary for partition " << pit->partitionId()
                          << " is not active";
            errorDescription << "Primary is not active for partition id "
                             << pit->partitionId();
            *rc = rc_ERROR;
            return;  // RETURN
        }

        mqbnet::ClusterNode* primary = pit->primaryNode();

        if (primary) {
            // Don't add duplicate
            if (bsl::find(nodes->begin(), nodes->end(), primary) !=
                nodes->end()) {
                continue;  // CONTINUE
            }
            // Check for self
            if (d_state.isSelfActivePrimary(pit->partitionId())) {
                *isSelfPrimary = true;
                continue;  // CONTINUE
            }
            nodes->push_back(primary);
        }
        else {
            BALL_LOG_WARN << "Error while collecting primary nodes: No "
                             "primary found for partition id "
                          << pit->partitionId();
            errorDescription << "No primary found for partition id "
                             << pit->partitionId();
            *rc = rc_ERROR;
            return;  // RETURN
        }
    }

    *rc = rc_SUCCESS;
}

void Cluster::getPartitionPrimaryNode(int*                  rc,
                                      bsl::ostream&         errorDescription,
                                      mqbnet::ClusterNode** node,
                                      bool*                 isSelfPrimary,
                                      int                   partitionId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(rc);
    BSLS_ASSERT_SAFE(node);
    BSLS_ASSERT_SAFE(isSelfPrimary);
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    enum RcEnum {
        rc_SUCCESS = 0,
        rc_ERROR   = -1,
    };

    const mqbc::ClusterState::PartitionsInfo& partitions =
        d_state.partitions();

    // Check boundary conditions for partitionId
    if (partitionId < 0 ||
        static_cast<int>(partitions.size()) <= partitionId) {
        errorDescription << "Invalid partition id: " << partitionId;
        *rc = rc_ERROR;
        return;  // RETURN
    }

    // Self is active primary
    if (d_state.isSelfActivePrimary(partitionId)) {
        *isSelfPrimary = true;
        *rc            = rc_SUCCESS;
    }
    else if (d_state.hasActivePrimary(partitionId)) {
        // Partition has active primary, get it and return that
        mqbnet::ClusterNode* primary =
            partitions.at(partitionId).primaryNode();
        if (primary) {
            *node = primary;
            *rc   = rc_SUCCESS;
        }
        else {
            // No primary node
            errorDescription << "No primary node for partition id "
                             << partitionId;
            *rc = rc_ERROR;
        }
    }
    // No active primary
    else {
        errorDescription << "No active primary for partition id "
                         << partitionId;
        *rc = rc_ERROR;
    }
}

}  // close package namespace
}  // close enterprise namespace
