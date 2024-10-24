// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbblp_clusterproxy.cpp                                            -*-C++-*-
#include <mqbblp_clusterproxy.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clustermembership.h>
#include <mqbc_clusterutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>
#include <mqbi_queue.h>

// BMQ
#include <bmqp_ackmessageiterator.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqt_messageguid.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_severity.h>
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdld_datum.h>
#include <bdld_datummapbuilder.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlmt_eventscheduler.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslmt_lockguard.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const double k_ACTIVE_NODE_INITIAL_WAIT = 10.0;
// Maximum time to wait, during initialization, for a node within
// the same data center to come up - refer to the 'Active node
// selection' documentation note in the header for more details.

typedef bsl::function<void()> CompletionCallback;

/// Utility function used in `bmqu::OperationChain` as the operation
/// callback which just calls the completion callback.
void completeShutDown(const CompletionCallback& callback)
{
    callback();
}

}  // close unnamed namespace

// ------------------
// class ClusterProxy
// ------------------

// PRIVATE MANIPULATORS
void ClusterProxy::generateNack(bmqt::AckResult::Enum               status,
                                const bmqp::PutHeader&              putHeader,
                                DispatcherClient*                   source,
                                const bsl::shared_ptr<bdlbb::Blob>& appData,
                                const bsl::shared_ptr<bdlbb::Blob>& options,
                                bmqt::GenericResult::Enum           rc)
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

    BMQU_THROTTLEDACTION_THROTTLE(
        d_throttledSkippedPutMessages,
        BALL_LOG_ERROR << description() << ": skipping relay-PUT message ["
                       << "queueId: " << putHeader.queueId() << ", GUID: "
                       << putHeader.messageGUID() << "], rc: " << rc << ".";);
}

void ClusterProxy::startDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "start() can only be called once on this object");

    d_queueHelper.initialize();

    d_clusterData.membership().netCluster()->registerObserver(this);

    // Start the monitor
    d_clusterMonitor.start();
    d_clusterMonitor.registerObserver(this);

    d_isStarted = true;

    // Because the 'cluster' was already created, it may already had some
    // sessions up before we registered ourself as observer.  So scan all nodes
    // and all session to build initial state and then schedule a refresh to
    // find active node if there is none after processing all pending events.

    d_activeNodeManager.initialize(&d_clusterData.transportManager());

    // Ready to read.
    d_clusterData.membership().netCluster()->enableRead();

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterProxy::refreshActiveNodeManagerDispatched,
                             this),
        this);

    // Schedule an event, to ensure we will eventually use a connection - refer
    // to the ClusterActiveNodeManager documentation for more details.
    bsls::TimeInterval interval = bmqsys::Time::nowMonotonicClock() +
                                  bsls::TimeInterval(
                                      k_ACTIVE_NODE_INITIAL_WAIT);
    d_clusterData.scheduler().scheduleEvent(
        &d_activeNodeLookupEventHandle,
        interval,
        bdlf::BindUtil::bind(&ClusterProxy::onActiveNodeLookupTimerExpired,
                             this));
}

void ClusterProxy::initiateShutdownDispatched(const VoidFunctor& callback,
                                              bool supportShutdownV2)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(!d_isStopping && "Shutdown already in progress");

    BALL_LOG_INFO << "Shutting down ClusterProxy: [name: '" << name() << "']";

    // Mark self as stopping.
    d_isStopping = true;

    if (supportShutdownV2) {
        d_queueHelper.requestToStopPushing();

        bsls::TimeInterval whenToStop(
            bsls::SystemTime::now(bsls::SystemClockType::e_MONOTONIC));
        whenToStop.addMilliseconds(d_clusterData.clusterConfig()
                                       .queueOperations()
                                       .shutdownTimeoutMs());

        d_shutdownChain.appendInplace(
            bdlf::BindUtil::bind(&ClusterQueueHelper::checkUnconfirmedV2,
                                 &d_queueHelper,
                                 whenToStop,
                                 bdlf::PlaceHolders::_1));  // completionCb
    }
    else {
        // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest
        // V2.

        // Fill the first link with client session shutdown operations
        bmqu::OperationChainLink link(d_shutdownChain.allocator());
        SessionSpVec             sessions;
        bsls::TimeInterval       shutdownTimeout;
        shutdownTimeout.addMilliseconds(
            clusterProxyConfig()->queueOperations().shutdownTimeoutMs());

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
            if (mqbnet::ClusterUtil::isClientOrProxy(negoMsg)) {
                if (mqbnet::ClusterUtil::isClient(negoMsg)) {
                    link.insert(bdlf::BindUtil::bind(
                        &mqbnet::Session::initiateShutdown,
                        sessionSp,
                        bdlf::PlaceHolders::_1,
                        shutdownTimeout,
                        false));
                }
                else {
                    sessions.push_back(sessionSp);
                }
            }
        }

        link.insert(bdlf::BindUtil::bind(
            &ClusterProxy::sendStopRequest,
            this,
            sessions,
            bdlf::PlaceHolders::_1));  // completion callback

        d_shutdownChain.append(&link);
    }

    // Add callback to be invoked once V1 shuts down all client sessions or
    // V2 finishes waiting for unconfirmed
    d_shutdownChain.appendInplace(bdlf::BindUtil::bind(&completeShutDown,
                                                       bdlf::PlaceHolders::_1),
                                  callback);

    d_shutdownChain.start();
}

void ClusterProxy::stopDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // Cancel all requests.
    bmqp_ctrlmsg::ControlMessage response;
    bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
    failure.message()  = "ClusterProxy is being stopped";

    d_clusterData.requestManager().cancelAllRequests(response);

    d_clusterMonitor.unregisterObserver(this);
    d_clusterMonitor.stop();
    d_clusterData.membership().netCluster()->unregisterObserver(this);
    d_queueHelper.teardown();

    // Cancel scheduler event
    if (d_activeNodeLookupEventHandle) {
        d_clusterData.scheduler().cancelEventAndWait(
            &d_activeNodeLookupEventHandle);
    }

    // Close all channels of the associated cluster.  Once we return from this
    // 'stopDispatched' method, this cluster proxy object gets destroyed, but
    // not the underlying channels, which will be destroyed later; so if a
    // message is received on the channel, it will try to invoke the
    // 'processEvent' method on that now destroyed cluster proxy instance.
    // While we should revisit this, closing the channel at this time here is a
    // working *probably temporary* fix.  Note that similar workaround exists
    // in 'Cluster::stopDispatched'.
    //
    // NOTE: channel.close() will not flush the channel's buffer, so there
    //       is a risk that any last message(s) may not be delivered.
    // TBD:  In the future, we should review the entire shutdown logic, and
    //       eventually implement a request/response based mechanism in
    //       order to provide true graceful shutdown.
    d_clusterData.membership().netCluster()->closeChannels();
}

void ClusterProxy::processCommandDispatched(
    mqbcmd::ClusterResult*        result,
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
        d_queueHelper.loadState(&result->makeClusterQueueHelper());
        return;  // RETURN
    }

    // Note that ClusterProxy only implements a subset of the commands that can
    // be sent to a cluster.

    bdlma::LocalSequentialAllocator<256> localAllocator(d_allocator_p);
    bmqu::MemOutStream                   os(&localAllocator);
    os << "Unknown command '" << command << "'";
    result->makeError().message() = os.str();
}

// PRIVATE MANIPULATORS
void ClusterProxy::onActiveNodeLookupTimerExpired()
{
    // executed by the *SCHEDULER* thread
    dispatcher()->execute(
        bdlf::BindUtil::bind(
            &ClusterProxy::onActiveNodeLookupTimerExpiredDispatched,
            this),
        this);
}

void ClusterProxy::onActiveNodeLookupTimerExpiredDispatched()
{
    d_activeNodeLookupEventHandle.release();
    d_activeNodeManager.enableExtendedSelection();

    refreshActiveNodeManagerDispatched();
}

void ClusterProxy::refreshActiveNodeManagerDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    int result = d_activeNodeManager.refresh();

    processActiveNodeManagerResult(result, 0);
}

void ClusterProxy::processActiveNodeManagerResult(
    int                        result,
    const mqbnet::ClusterNode* node)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (result & mqbnet::ClusterActiveNodeManager::e_LOST_ACTIVE) {
        onActiveNodeDown(node);
    }
    if (result & mqbnet::ClusterActiveNodeManager::e_NEW_ACTIVE) {
        // Cancel the scheduler event, if any.
        if (d_activeNodeLookupEventHandle) {
            d_clusterData.scheduler().cancelEvent(
                &d_activeNodeLookupEventHandle);
            d_activeNodeManager.enableExtendedSelection();
        }

        onActiveNodeUp(d_activeNodeManager.activeNode());
    }

    updateDatumStats();
}

// PRIVATE MANIPULATORS
//   (Active node handling)
void ClusterProxy::onActiveNodeUp(mqbnet::ClusterNode* activeNode)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_isStopping) {
        BALL_LOG_INFO << description() << ": Ignoring 'ActiveNodeUp' "
                      << "notification because self is stopping.";
        return;  // RETURN
    }

    // Indicate we have a new leader (in proxy, the active is equal to the
    // leader); this will trigger a reopen of the queues that will eventually
    // invoke the queueHelper's 'stateRestoredFn' (i.e., 'onQueuesReopened').
    // Also manually bump up the elector term.
    d_clusterData.electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_LEADER,
        d_clusterData.electorInfo().electorTerm() + 1,
        activeNode,
        mqbc::ElectorInfoLeaderStatus::e_ACTIVE);
}

void ClusterProxy::onActiveNodeDown(const mqbnet::ClusterNode* node)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(node);

    if (d_isStopping) {
        BALL_LOG_INFO << description() << ": Ignoring 'ActiveNodeDown' "
                      << "notification because self is stopping.";

        bmqp_ctrlmsg::ControlMessage response;
        bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
        failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
        failure.code()     = mqbi::ClusterErrorCode::e_STOPPING;
        failure.message()  = "Lost connection with active node while stopping";

        d_clusterData.requestManager().cancelAllRequests(response,
                                                         node->nodeId());

        return;  // RETURN
    }

    // Cancel all requests with the 'CANCELED' category and the 'e_ACTIVE_LOST'
    // code; and the callback will simply re-issue those requests; which then
    // will be added to the pending request list.

    bmqp_ctrlmsg::ControlMessage response;
    bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();
    failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
    failure.code()     = mqbi::ClusterErrorCode::e_ACTIVE_LOST;
    failure.message()  = "Lost connection with active node";

    d_clusterData.requestManager().cancelAllRequests(response, node->nodeId());

    // Update the cluster state with the info.  Active node lock must *not* be
    // held.  Reuse the existing term since this is not a 'new active node'
    // event.

    d_clusterData.electorInfo().setElectorInfo(
        mqbnet::ElectorState::e_DORMANT,
        d_clusterData.electorInfo().electorTerm(),
        0,  // Leader (or 'active node') pointer
        mqbc::ElectorInfoLeaderStatus::e_UNDEFINED);
}

// PRIVATE MANIPULATORS
//   (Event processing)
void ClusterProxy::onPushEvent(const mqbi::DispatcherPushEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // This PUSH event is sent by cluster (replica/primary) when new messages
    // are available for various RemoteQueues maintained by this proxy.
    // Iterate over each message and route to appropriate queue.

    bmqp::Event rawEvent(event.blob().get(), d_allocator_p);
    bdlma::LocalSequentialAllocator<1024> lsa(d_allocator_p);
    bmqp::PushMessageIterator iter(&d_clusterData.bufferFactory(), &lsa);
    rawEvent.loadPushMessageIterator(&iter, false);

    BSLS_ASSERT_SAFE(iter.isValid());
    int rc = 0;
    while ((rc = iter.next()) == 1) {
        mqbi::Queue* queue = d_queueHelper.lookupQueue(
            iter.header().queueId());
        if (!queue) {
            BALL_LOG_ERROR << "#CLUSTER_INVALID_PUSH " << description()
                           << "Invalid queueId " << iter.header().queueId();
            continue;  // CONTINUE
        }

        bsl::shared_ptr<bdlbb::Blob> appDataSp =
            d_clusterData.blobSpPool().getObject();
        rc = iter.loadApplicationData(appDataSp.get());
        BSLS_ASSERT_SAFE(rc == 0);

        bsl::shared_ptr<bdlbb::Blob> optionsSp;
        if (iter.hasOptions()) {
            optionsSp = d_clusterData.blobSpPool().getObject();
            rc        = iter.loadOptions(optionsSp.get());
            BSLS_ASSERT_SAFE(0 == rc);
        }

        bmqp::MessagePropertiesInfo logic(iter.header());
        queue->onPushMessage(iter.header().messageGUID(),
                             appDataSp,
                             optionsSp,
                             logic,
                             iter.header().compressionAlgorithmType(),
                             bmqp::PushHeaderFlagUtil::isSet(
                                 iter.header().flags(),
                                 bmqp::PushHeaderFlags::e_OUT_OF_ORDER));
    }
}

void ClusterProxy::onAckEvent(const mqbi::DispatcherAckEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay() == false);

    // This ACK event is sent by cluster (replica/primary) for various
    // RemoteQueues maintained by this proxy.  Iterate over each ack message
    // and route to appropriate queue.

    bmqp::Event              rawEvent(event.blob().get(), d_allocator_p);
    bmqp::AckMessageIterator iter;
    rawEvent.loadAckMessageIterator(&iter);

    BSLS_ASSERT_SAFE(iter.isValid());
    int rc = 0;
    while ((rc = iter.next()) == 1) {
        const bmqp::AckMessage& ackMessage = iter.message();

        mqbi::Queue* queue = d_queueHelper.lookupQueue(ackMessage.queueId());
        if (!queue) {
            // Should not be able to receive an ack for an unknown queue
            //
            // NOTE: this could happen if the downstream client sent a message,
            //       then closes the queue/disconnect, before the cluster sent
            //       back the ACK.
            // TBD: Until closeQueue sequence is revisited to prevent this
            //      situation, log at INFO level.
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedAckMessages,
                BALL_LOG_INFO << "Received an ACK for unknown queue "
                              << "[queueId: " << ackMessage.queueId()
                              << ", guid: " << ackMessage.messageGUID()
                              << ", status: " << ackMessage.status() << "]";);
            continue;  // RETURN
        }

        queue->onAckMessage(ackMessage);
    }
}

void ClusterProxy::onRelayPutEvent(const mqbi::DispatcherPutEvent& event,
                                   mqbi::DispatcherClient*         source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay() == true);

    // This event is invoked as a result of RemoteQueue asking cluster proxy to
    // relay PUT message to cluster on it's behalf.  Note that we don't check
    // if we have an active node connection, and simply add the PUT message to
    // the builder.  If there is no active connection right now, these PUT
    // messages will be relayed whenever a connection with new active is
    // established and state is restored (ie, queues are re-opened).

    const bmqp::PutHeader& ph       = event.putHeader();
    bsls::Types::Uint64    genCount = event.genCount();
    bsls::Types::Uint64    term = d_clusterData.electorInfo().electorTerm();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(genCount != term)) {
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledSkippedPutMessages,
            BALL_LOG_WARN << description() << ": skipping relay-PUT message ["
                          << "queueId: " << ph.queueId() << ", GUID: "
                          << ph.messageGUID() << "], genCount: " << genCount
                          << " vs " << term << ".";);
        return;  // RETURN
    }

    mqbnet::ClusterNode*      activeNode = d_activeNodeManager.activeNode();
    bmqt::GenericResult::Enum rc;

    if (activeNode) {
        rc = activeNode->channel().writePut(event.putHeader(),
                                            event.blob(),
                                            event.state());
    }
    else {
        rc = bmqt::GenericResult::e_NOT_CONNECTED;
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        generateNack(bmqt::AckResult::e_NOT_READY,
                     ph,
                     source,
                     event.blob(),
                     event.options(),
                     rc);
    }
}

void ClusterProxy::onRelayConfirmEvent(
    const mqbi::DispatcherConfirmEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay() == true);

    // This event is invoked as a result of RemoteQueue asking cluster proxy to
    // relay CONFIRM message to cluster on it's behalf.

    mqbnet::ClusterNode*        activeNode = d_activeNodeManager.activeNode();
    const bmqp::ConfirmMessage& confirmMsg = event.confirmMessage();
    bmqt::GenericResult::Enum   rc;

    if (activeNode) {
        rc = activeNode->channel().writeConfirm(confirmMsg.queueId(),
                                                confirmMsg.subQueueId(),
                                                confirmMsg.messageGUID());
    }
    else {
        rc = bmqt::GenericResult::e_NOT_CONNECTED;
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Failed to relay Confirm message [rc: " << rc
                       << ", GUID: " << confirmMsg.messageGUID()
                       << ", queue Id: " << confirmMsg.queueId() << ""
                       << ", subqueue Id: " << confirmMsg.subQueueId() << "";
    }
}

void ClusterProxy::onRelayRejectEvent(const mqbi::DispatcherRejectEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(event.isRelay() == true);

    // executed by the *DISPATCHER* thread

    // This event is invoked as a result of RemoteQueue asking cluster proxy to
    // relay REJECT message to cluster on it's behalf.  Note that we don't
    // check if we have an active node connection, and simply add the REJECT
    // message to the builder.  If there is no active connection right now,
    // these REJECT messages will be relayed whenever a connection with new
    // active is established and state is restored (ie, queues are re-opened).

    const bmqp::RejectMessage& rejectMsg  = event.rejectMessage();
    mqbnet::ClusterNode*       activeNode = d_activeNodeManager.activeNode();
    bmqt::GenericResult::Enum  rc;

    if (activeNode) {
        rc = activeNode->channel().writeReject(rejectMsg.queueId(),
                                               rejectMsg.subQueueId(),
                                               rejectMsg.messageGUID());
    }
    else {
        rc = bmqt::GenericResult::e_NOT_CONNECTED;
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS)) {
        BALL_LOG_ERROR << "Failed to relay Reject message [rc: " << rc
                       << ", GUID: " << rejectMsg.messageGUID()
                       << ", queue Id: " << rejectMsg.queueId() << ""
                       << ", subqueue Id: " << rejectMsg.subQueueId() << "]";
    }
}

// PRIVATE MANIPULATORS
//   (virtual: mqbnet::SessionEventProcessor)
void ClusterProxy::processEvent(const bmqp::Event&   event,
                                mqbnet::ClusterNode* source)
{
    // executed by any of the *IO* threads

    switch (event.type()) {
    case bmqp::EventType::e_CONTROL: {
        bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
        bmqp_ctrlmsg::ControlMessage          controlMessage(&localAllocator);

        int rc = event.loadControlEvent(&controlMessage);
        if (rc != 0) {
            BALL_LOG_ERROR
                << "#CORRUPTED_EVENT " << description()
                << ": received invalid control message [reason: 'failed to "
                << "decode', rc: " << rc << "]\n"
                << bmqu::BlobStartHexDumper(event.blob());
            return;  // RETURN
        }

        BALL_LOG_INFO << description() << ": received " << controlMessage;

        if (controlMessage.choice().isClusterMessageValue()) {
            const bmqp_ctrlmsg::ClusterMessage& clusterMessage =
                controlMessage.choice().clusterMessage();

            if (clusterMessage.choice().isStopRequestValue()) {
                dispatcher()->execute(
                    bdlf::BindUtil::bind(&ClusterProxy::processPeerStopRequest,
                                         this,
                                         source,
                                         controlMessage),
                    this);
                return;  // RETURN
            }
            if (clusterMessage.choice().isStopResponseValue()) {
                dispatcher()->execute(
                    bdlf::BindUtil::bind(
                        &ClusterProxy::processPeerStopResponse,
                        this,
                        controlMessage),
                    this);
                return;  // RETURN
            }
            if (clusterMessage.choice().isNodeStatusAdvisoryValue()) {
                dispatcher()->execute(
                    bdlf::BindUtil::bind(
                        &ClusterProxy::processNodeStatusAdvisory,
                        this,
                        clusterMessage.choice().nodeStatusAdvisory(),
                        source),
                    this);
                return;  // RETURN
            }
        }

        if (!controlMessage.rId().isNull()) {
            // Control message has an id, this is a response to a request.
            // Forward it to the cluster-dispatcher thread.

            dispatcher()->execute(
                bdlf::BindUtil::bind(&ClusterProxy::processResponseDispatched,
                                     this,
                                     controlMessage),
                this);
            return;  // RETURN
        }
    } break;
    case bmqp::EventType::e_PUT: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description() << "PUT";
    } break;
    case bmqp::EventType::e_CONFIRM: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description() << "CONFIRM";
    } break;
    case bmqp::EventType::e_REJECT: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description() << "REJECT";
    } break;
    case bmqp::EventType::e_PUSH: {
        mqbi::DispatcherEvent*       dispEvent = dispatcher()->getEvent(this);
        bsl::shared_ptr<bdlbb::Blob> blobSp =
            d_clusterData.blobSpPool().getObject();
        *blobSp = *(event.blob());
        (*dispEvent).setSource(this).makePushEvent(blobSp);
        dispatcher()->dispatchEvent(dispEvent, this);
    } break;
    case bmqp::EventType::e_ACK: {
        mqbi::DispatcherEvent*       dispEvent = dispatcher()->getEvent(this);
        bsl::shared_ptr<bdlbb::Blob> blobSp =
            d_clusterData.blobSpPool().getObject();
        *blobSp = *(event.blob());
        (*dispEvent).setSource(this).makeAckEvent().setBlob(blobSp);
        dispatcher()->dispatchEvent(dispEvent, this);
    } break;
    case bmqp::EventType::e_UNDEFINED:
    case bmqp::EventType::e_CLUSTER_STATE:
    case bmqp::EventType::e_ELECTOR:
    case bmqp::EventType::e_STORAGE:
    case bmqp::EventType::e_RECOVERY:
    case bmqp::EventType::e_PARTITION_SYNC:
    case bmqp::EventType::e_HEARTBEAT_REQ:
    case bmqp::EventType::e_HEARTBEAT_RSP:
    case bmqp::EventType::e_REPLICATION_RECEIPT: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                       << "Received unexpected event: " << event;
        BSLS_ASSERT_SAFE(false && "Unexpected event received");
        return;  // RETURN
    }  // break;
    default: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                       << "Received unknown event: " << event;
        BSLS_ASSERT_SAFE(false && "Unknown event received");
        return;  // RETURN
    };
    }
}

// PRIVATE MANIPULATORS
//   (virtual: mqbi::Cluster)
bmqt::GenericResult::Enum
ClusterProxy::sendRequest(const RequestManagerType::RequestSp& request,
                          mqbnet::ClusterNode*                 target,
                          bsls::TimeInterval                   timeout)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    mqbnet::ClusterNode* activeNode = d_activeNodeManager.activeNode();

    if (!activeNode) {
        // Not connected to any node, can't deliver the request.
        ball::Severity::Level severity = ball::Severity::ERROR;
        if (d_activeNodeLookupEventHandle) {
            // If the cluster is being established, since it's lazily created,
            // it is expected that the first request will fail to send, just
            // silence the error.
            severity = ball::Severity::INFO;
        }
        BALL_LOG_STREAM(severity)
            << "#CLUSTER_SEND_FAILURE " << description()
            << ": Unable to send request [reason: 'NO_ACTIVE']: "
            << request->request();
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    if (target && target != activeNode) {
        if (!d_isStopping) {
            // Don't warn/alarm if self is stopping.
            BALL_LOG_WARN << description() << ": Unable to send request "
                          << request->request()
                          << ", because specified target "
                          << target->nodeDescription() << " is different from "
                          << "the upstream node as perceived by the proxy "
                          << activeNode->nodeDescription() << ".";
        }
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    request->setGroupId(activeNode->nodeId());

    return d_clusterData.requestManager().sendRequest(
        request,
        bdlf::BindUtil::bind(&mqbnet::ClusterNode::write,
                             activeNode,
                             bdlf::PlaceHolders::_1,
                             bmqp::EventType::e_CONTROL),
        activeNode->nodeDescription(),
        timeout);
}

void ClusterProxy::processResponse(
    const bmqp_ctrlmsg::ControlMessage& response)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    // Control message has an id
    BSLS_ASSERT_SAFE((!response.rId().isNull()));

    // This is a response to a request.  Forward it to the cluster-dispatcher
    // thread.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterProxy::processResponseDispatched,
                             this,
                             response),
        this);
}

void ClusterProxy::processPeerStopResponse(
    const bmqp_ctrlmsg::ControlMessage& response)
{
    BALL_LOG_INFO << description() << ": processStopResponse: " << response;

    d_stopRequestsManager_p->processResponse(response);
}

void ClusterProxy::processPeerStopRequest(
    mqbnet::ClusterNode*                clusterNode,
    const bmqp_ctrlmsg::ControlMessage& request)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_queueHelper.processNodeStoppingNotification(
        clusterNode,
        &request,
        0,
        bdlf::BindUtil::bind(&ClusterProxy::finishStopSequence,
                             this,
                             clusterNode));
}

void ClusterProxy::finishStopSequence(
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* clusterNode)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // REVISIT
    // Internal-ticket D169562052
    // TODO: handle/eliminate the possibility of Multiple StopRequests.
    // Currently, cannot switch the active node because another StopRequest can
    // be processing (note that the processing is asynchronous due to switching
    // to queue thread(s)).  The processing can change the state of subStreams
    // conflict with new active node changing the same state.

    /*
    int result = d_activeNodeManager.onNodeDown(clusterNode);

    processActiveNodeManagerResult(result);
    */
}

void ClusterProxy::updateDatumStats()
{
    typedef NodeStatsMap::iterator NodeStatsMapIter;
    NodeStatsMapIter               iter    = d_nodeStatsMap.begin();
    NodeStatsMapIter               endIter = d_nodeStatsMap.end();
    for (; iter != endIter; ++iter) {
        mqbnet::ClusterNode* node   = iter->first;
        int                  status = node->isAvailable();
        bsl::string          active = node == d_activeNodeManager.activeNode()
                                          ? "Active"
                                          : "";

        bmqst::StatContext* statContext = (iter->second).get();
        bslma::Allocator*   alloc       = statContext->datumAllocator();
        ManagedDatumMp      datum       = statContext->datum();

        bdld::DatumMapBuilder builder(alloc);
        builder.pushBack("leader", bdld::Datum::copyString(active, alloc));
        builder.pushBack("status", bdld::Datum::createInteger(status));
        builder.pushBack("isProxy", bdld::Datum::createInteger(true));

        datum->adopt(builder.commit());
    }

    d_clusterData.stats().setUpstream(
        d_activeNodeManager.activeNode()
            ? d_activeNodeManager.activeNode()->hostName()
            : "");
}

void ClusterProxy::processNodeStatusAdvisory(
    const bmqp_ctrlmsg::NodeStatusAdvisory& advisory,
    mqbnet::ClusterNode*                    source)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // This advisory is when a node in the cluster identified by the specified
    // 'source', changes its status (eg STARTING -> AVAILABLE, AVAILABLE ->
    // STOPPING, etc).
    BALL_LOG_INFO << description() << ": received node status advisory [ "
                  << advisory << " ] from " << source->nodeDescription();

    int result = d_activeNodeManager.onNodeStatusChange(source,
                                                        advisory.status());
    processActiveNodeManagerResult(result, source);
}

void ClusterProxy::processResponseDispatched(
    const bmqp_ctrlmsg::ControlMessage& response)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    int rc = d_clusterData.requestManager().processResponse(response);

    if (rc != 0 && response.choice().isOpenQueueResponseValue() &&
        !d_isStopping) {
        // We received an openQueue *SUCCESS* response but we already timed the
        // request out; in order to keep consistent state across all brokers,
        // we must send a closeQueue (as far as upstream brokers are concerned,
        // this client has the queue opened, which is not true, so roll it
        // back).  This is done only if self is not stopping.

        // Prepare the request.  We don't need to bind any response since we
        // don't need to do any processing, this request is really just to
        // inform upstream brokers; the internal state in the client was
        // already updated when the request time out was processed.

        RequestManagerType::RequestSp request =
            d_clusterData.requestManager().createRequest();
        bmqp_ctrlmsg::CloseQueue& closeQueue =
            request->request().choice().makeCloseQueue();

        closeQueue.handleParameters() = response.choice()
                                            .openQueueResponse()
                                            .originalRequest()
                                            .handleParameters();
        sendRequest(request,
                    0,  // target
                    bsls::TimeInterval(60));
    }
}

void ClusterProxy::sendStopRequest(const SessionSpVec& sessions,
                                   const StopRequestCompletionCallback& stopCb)
{
    // Send a StopRequest to available proxies connected to the virtual cluster
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
                  << " proxies; timeout is " << timeoutMs;

    d_stopRequestsManager_p->sendRequest(contextSp, timeoutMs);

    // continue after receipt of all StopResponses or the timeout
}

// PRIVATE ACCESSORS
void ClusterProxy::loadQueuesInfo(mqbcmd::StorageContent* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_queueHelper.loadQueuesInfo(out);
}

// CREATORS
ClusterProxy::ClusterProxy(
    const bslstl::StringRef&              name,
    const mqbcfg::ClusterProxyDefinition& clusterProxyConfig,
    bslma::ManagedPtr<mqbnet::Cluster>    netCluster,
    const StatContextsMap&                statContexts,
    mqbi::Dispatcher*                     dispatcher,
    mqbnet::TransportManager*             transportManager,
    StopRequestManagerType*               stopRequestsManager,
    const mqbi::ClusterResources&         resources,
    bslma::Allocator*                     allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_isStopping(false)
, d_clusterData(name,
                resources,
                mqbcfg::ClusterDefinition(allocator),
                clusterProxyConfig,
                netCluster,
                this,
                0,
                transportManager,
                statContexts.find("clusters")->second,
                statContexts,
                allocator)
, d_state(this,
          0,  // Partition count.  Proxy has no notion of partition.
          allocator)
, d_activeNodeManager(d_clusterData.membership().netCluster()->nodes(),
                      description(),
                      mqbcfg::BrokerConfig::get().hostDataCenter())
, d_queueHelper(&d_clusterData, &d_state, 0, allocator)
, d_nodeStatsMap(allocator)
, d_throttledFailedAckMessages(5000, 1)   // 1 log per 5s interval
, d_throttledSkippedPutMessages(5000, 1)  // 1 log per 5s interval
, d_clusterMonitor(&d_clusterData, &d_state, d_allocator_p)
, d_activeNodeLookupEventHandle()
, d_shutdownChain(d_allocator_p)
, d_stopRequestsManager_p(stopRequestsManager)
{
    // PRECONDITIONS
    mqbnet::Cluster* netCluster_p = d_clusterData.membership().netCluster();
    BSLS_ASSERT_SAFE(netCluster_p && "NetCluster not set !");
    BSLS_ASSERT_SAFE(resources.scheduler()->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);

    d_clusterData.clusterConfig().queueOperations() =
        clusterProxyConfig.queueOperations();

    d_clusterData.stats().setIsMember(false);

    // Create and populate stats data for each cluster node
    NodeStatsMap& nodeStatsMap = d_nodeStatsMap;
    NodeListIter  nodeIter     = netCluster_p->nodes().begin();
    NodeListIter  endIter      = netCluster_p->nodes().end();
    for (; nodeIter != endIter; ++nodeIter) {
        bmqst::StatContextConfiguration config((*nodeIter)->hostName());

        StatContextMp statContextMp =
            d_clusterData.clusterNodesStatContext()->addSubcontext(config);
        StatContextSp statContextSp(statContextMp, d_allocator_p);

        nodeStatsMap.insert(bsl::make_pair(*nodeIter, statContextSp));
    }

    // Register to the dispatcher
    mqbi::Dispatcher::ProcessorHandle processor = dispatcher->registerClient(
        this,
        mqbi::DispatcherClientType::e_CLUSTER);

    d_clusterData.requestManager().setExecutor(dispatcher->executor(this));

    BALL_LOG_INFO << "Created ClusterProxy: "
                  << "[name: '" << name << "'"
                  << ", dispatcherProcessor: " << processor << "]";

    updateDatumStats();
}

ClusterProxy::~ClusterProxy()
{
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "stop() must be called before destruction");

    BALL_LOG_INFO << "ClusterProxy '" << description() << "': destructor.";

    // Unregister from the dispatcher
    dispatcher()->unregisterClient(this);

    d_shutdownChain.stop();
    d_shutdownChain.join();
}

// MANIPULATORS
//   (virtual: mqbi::Cluster)
int ClusterProxy::start(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    dispatcher()->execute(bdlf::BindUtil::bind(&ClusterProxy::startDispatched,
                                               this),
                          this);
    dispatcher()->synchronize(this);

    return 0;
}

void ClusterProxy::initiateShutdown(const VoidFunctor& callback,
                                    bool               supportShutdownV2)
{
    // executed by *ANY* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!dispatcher()->inDispatcherThread(this));
    // Deadlock detection (because of the 'synchronize' call below,
    // we can't execute from any of the cluster's dispatcher thread).

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterProxy::initiateShutdownDispatched,
                             this,
                             callback,
                             supportShutdownV2),
        this);

    dispatcher()->synchronize(this);
}

void ClusterProxy::stop()
{
    // executed by *ANY* thread

    if (!d_isStarted) {
        return;  // RETURN
    }

    BALL_LOG_INFO << "Stopping ClusterProxy: [name: '" << name() << "']";

    d_isStarted = false;

    dispatcher()->execute(bdlf::BindUtil::bind(&ClusterProxy::stopDispatched,
                                               this),
                          this);
    dispatcher()->synchronize(this);

    // We synchronize twice here.  1st synchronize() is needed to ensure that
    // 'stopDispatched' has been executed.  The 2nd synchronize() is needed to
    // ensure that any event that 'stopDispatched' may have enqueued back to
    // cluster's dispatcher thread has also been executed.  An example is
    // 'ClusterStateMonitor::verifyAllStates' which schedules
    // 'ClusterStateMonitor::verifyAllStatesDispatched'.
    // 'ClusterStateMonitor::stop' waits for concurrent 'verifyAllStates' but
    // we also need to wait for 'verifyAllStatesDispatched'.
    // Another way to think of this is that 'stopDispatched' is async, which
    // itself performs some async operations, so we need to synchronize twice.

    dispatcher()->synchronize(this);
}

void ClusterProxy::registerStateObserver(
    BSLS_ANNOTATION_UNUSED mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    // NOTHING
    //
    // TODO_CSL Register the observer to 'dummy state' after it becomes the
    //          true state.
}

void ClusterProxy::unregisterStateObserver(
    BSLS_ANNOTATION_UNUSED mqbc::ClusterStateObserver* observer)
{
    // executed by *ANY* thread

    // NOTHING
    //
    // TODO_CSL Register the observer to 'dummy state' after it becomes the
    //          true state.
}

void ClusterProxy::openQueue(
    const bmqt::Uri&                                          uri,
    mqbi::Domain*                                             domain,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const mqbi::Cluster::OpenQueueCallback&                   callback)
{
    // executed by *ANY* thread

    // Enqueue an event to process in the dispatcher thread
    dispatcher()->execute(bdlf::BindUtil::bind(&ClusterQueueHelper::openQueue,
                                               &d_queueHelper,
                                               uri,
                                               domain,
                                               handleParameters,
                                               clientContext,
                                               callback),
                          this);
}

void ClusterProxy::configureQueue(
    mqbi::Queue*                                       queue,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    unsigned int                                       upstreamSubQueueId,
    const mqbi::QueueHandle::HandleConfiguredCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(queue));

    d_queueHelper.configureQueue(queue,
                                 streamParameters,
                                 upstreamSubQueueId,
                                 callback);
}

void ClusterProxy::configureQueue(
    mqbi::Queue*                                 queue,
    const bmqp_ctrlmsg::QueueHandleParameters&   handleParameters,
    unsigned int                                 upstreamSubQueueId,
    const mqbi::Cluster::HandleReleasedCallback& callback)
{
    // executed by the associated *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(queue));

    d_queueHelper.configureQueue(queue,
                                 handleParameters,
                                 upstreamSubQueueId,
                                 callback);
}

void ClusterProxy::onQueueHandleCreated(mqbi::Queue*     queue,
                                        const bmqt::Uri& uri,
                                        bool             handleCreated)
{
    // executed by *ANY* thread

    d_queueHelper.onQueueHandleCreated(queue, uri, handleCreated);
}

void ClusterProxy::onQueueHandleDestroyed(mqbi::Queue*     queue,
                                          const bmqt::Uri& uri)
{
    // executed by *ANY* thread

    d_queueHelper.onQueueHandleDestroyed(queue, uri);
}

void ClusterProxy::onDomainReconfigured(
    BSLS_ANNOTATION_UNUSED const mqbi::Domain& domain,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Domain& oldDefn,
    BSLS_ANNOTATION_UNUSED const mqbconfm::Domain& newDefn)
{
    // NOTHING
    //
    // No action required from 'ClusterProxy' when a domain is updated.
}

int ClusterProxy::processCommand(mqbcmd::ClusterResult*        result,
                                 const mqbcmd::ClusterCommand& command)
{
    // executed by *ANY* thread
    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterProxy::processCommandDispatched,
                             this,
                             result,
                             bsl::ref(command)),
        this);
    dispatcher()->synchronize(this);

    return 0;
}

void ClusterProxy::loadClusterStatus(mqbcmd::ClusterResult* out)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    mqbcmd::ClusterProxyStatus& clusterProxyStatus =
        out->makeClusterProxyStatus();

    clusterProxyStatus.description() = description();
    if (d_activeNodeManager.activeNode()) {
        clusterProxyStatus.activeNodeDescription().makeValue(
            d_activeNodeManager.activeNode()->nodeDescription());
    }

    clusterProxyStatus.isHealthy() = d_clusterMonitor.isHealthy();
    // Print nodes status
    d_activeNodeManager.loadNodesInfo(&clusterProxyStatus.nodeStatuses());

    // Print queue status
    loadQueuesInfo(&clusterProxyStatus.queuesInfo());
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
void ClusterProxy::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BALL_LOG_TRACE << description() << ": processing dispatcher event '"
                   << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_PUT: {
        const mqbi::DispatcherPutEvent* realEvent =
            &event.getAs<mqbi::DispatcherPutEvent>();
        if (realEvent->isRelay()) {
            onRelayPutEvent(*realEvent, event.source());
        }
        else {
            BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                           << "PUT event not yet implemented";
        }
    } break;
    case mqbi::DispatcherEventType::e_CONFIRM: {
        const mqbi::DispatcherConfirmEvent* realEvent =
            &event.getAs<mqbi::DispatcherConfirmEvent>();
        if (realEvent->isRelay()) {
            onRelayConfirmEvent(*realEvent);
        }
        else {
            BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                           << "CONFIRM event not yet implemented";
        }
    } break;
    case mqbi::DispatcherEventType::e_REJECT: {
        const mqbi::DispatcherRejectEvent* realEvent =
            &event.getAs<mqbi::DispatcherRejectEvent>();
        if (realEvent->isRelay()) {
            onRelayRejectEvent(*realEvent);
        }
        else {
            BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                           << "REJECT event not yet implemented";
        }
    } break;
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            &event.getAs<mqbi::DispatcherCallbackEvent>();
        BSLS_ASSERT_SAFE(realEvent->callback());
        realEvent->callback()(dispatcherClientData().processorHandle());
    } break;
    case mqbi::DispatcherEventType::e_PUSH: {
        onPushEvent(event.getAs<mqbi::DispatcherPushEvent>());
    } break;
    case mqbi::DispatcherEventType::e_ACK: {
        onAckEvent(event.getAs<mqbi::DispatcherAckEvent>());
    } break;
    case mqbi::DispatcherEventType::e_CONTROL_MSG: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                       << "ADMIN event not yet implemented";
    } break;
    case mqbi::DispatcherEventType::e_DISPATCHER: {
        BSLS_ASSERT_OPT(false &&
                        "'DISPATCHER' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_CLUSTER_STATE: {
        BSLS_ASSERT_OPT(false &&
                        "'CLUSTER_STATE' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_STORAGE: {
        BSLS_ASSERT_OPT(false && "'STORAGE' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_RECOVERY: {
        BSLS_ASSERT_OPT(false &&
                        "'RECOVERY' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_UNDEFINED: {
        BSLS_ASSERT_OPT(false &&
                        "'UNDEFINED' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT: {
        BSLS_ASSERT_OPT(
            false && "'REPLICATION_RECEIPT' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    default: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << description()
                       << ": received unexpected dispatcher event: " << event;
    }
    }
}

void ClusterProxy::flush()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
}

// MANIPULATORS
//   (virtual: mqbc::ClusterStateObserver)
void ClusterProxy::onFailoverThreshold()
{
    const mqbcfg::ClusterMonitorConfig& monitorConfig =
        d_clusterData.clusterConfig().clusterMonitorConfig();
    bmqu::MemOutStream os;

    os << "'" << d_clusterData.identity().description() << "'"
       << " has not completed the failover process above the threshold "
       << "amount of "
       << bmqu::PrintUtil::prettyTimeInterval(
              monitorConfig.thresholdFailover() *
              bdlt::TimeUnitRatio::k_NS_PER_S)
       << ". There are " << d_queueHelper.numPendingReopenQueueRequests()
       << " pending reopen-queue requests.\n";
    // Log only a summary in the alarm
    printClusterStateSummary(os, 0, 4);
    BMQTSK_ALARMLOG_PANIC("CLUSTERPROXY") << os.str() << BMQTSK_ALARMLOG_END;
}

// MANIPULATORS
//   (virtual: mqbnet::ClusterObserver)
void ClusterProxy::onNodeStateChange(mqbnet::ClusterNode* node,
                                     bool                 isAvailable)
{
    if (isAvailable) {
        // Assuming this is IO thread and it is safe to access
        // 'mqbnet::ClusterNode::channel' and 'negotiationMessage'

        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterProxy::onNodeUpDispatched,
                                 this,
                                 node,
                                 node->identity()),
            this);
    }
    else {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&ClusterProxy::onNodeDownDispatched,
                                 this,
                                 node),
            this);
    }
}

void ClusterProxy::onNodeUpDispatched(
    mqbnet::ClusterNode*                node,
    const bmqp_ctrlmsg::ClientIdentity& identity)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    int result = d_activeNodeManager.onNodeUp(node, identity);

    processActiveNodeManagerResult(result, node);
}

void ClusterProxy::onNodeDownDispatched(mqbnet::ClusterNode* node)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(node);

    int result = d_activeNodeManager.onNodeDown(node);

    if (result == 0) {
        // This is not the active node
        bmqp_ctrlmsg::ControlMessage response;
        bmqp_ctrlmsg::Status&        failure = response.choice().makeStatus();

        failure.category() = bmqp_ctrlmsg::StatusCategory::E_CANCELED;
        failure.code()     = mqbi::ClusterErrorCode::e_NODE_DOWN;
        failure.message()  = "Lost connection with node";

        d_clusterData.requestManager().cancelAllRequests(response,
                                                         node->nodeId());
    }

    processActiveNodeManagerResult(result, node);
}

// ACCESSORS
//   (virtual: mqbi::Cluster)
void ClusterProxy::printClusterStateSummary(bsl::ostream& out,
                                            int           level,
                                            int           spacesPerLevel) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    mqbcmd::NodeStatuses nodeStatuses;
    d_activeNodeManager.loadNodesInfo(&nodeStatuses);

    mqbcmd::Result result;
    result.makeNodeStatuses(nodeStatuses);
    mqbcmd::HumanPrinter::print(out, result, level, spacesPerLevel);
}

}  // close package namespace
}  // close enterprise namespace
