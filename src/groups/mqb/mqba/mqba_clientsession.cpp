// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_clientsession.cpp                                             -*-C++-*-
#include <mqba_clientsession.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
//
/// Threading
///---------
// In order to be lock free, *all* methods are or must be executed on the
// *client dispatcher thread*.  The only exceptions are:
//: o processEvent: main entry of processing of data from the channel, executes
//:   on the IO thread and does basic safe processing before enqueuing for the
//:   client dispatcher
//: o onHighWatermark / onLowWatermark: notification event on the IO thread
//: o tearDown: executes on the IO thread when the channel went down
//
/// Shutdown
///--------
// At a high level, a graceful shutdown has the following flow:
//   - The client sends a 'Disconnect' control message
//   - The broker replies with a 'DisconnectResponse' control message
//   - In response to that message, the client closes the channel
//   - This triggers execution of the 'teardown' method
// However, an ungraceful shutdown can happen at any time (including in the
// middle of the above described sequence).
//
// The following defines the contract between the client and the broker with
// regards to graceful shutdown:
//: o The 'Disconnect' control message is the last packet sent by the client
//:   (note that it may not even be sent if the client didn't go down cleanly).
//: o The 'DisconnectResponse' control message is the last one sent by the
//:   broker to the client.
//
// During disconnect processing / teardown, we leverage the dispatcher queues
// to synchronize and ensure no more messages or events are pending for the
// client, so we can safely destroy it.  However, this solution will not work
// for remotely activated asynchronous requests (such as 'openQueue' which
// could be sending an async request to the cluster, and deliver the response
// through the proxyBroker<->clusterBroker IO channel).  For those kind of
// requests, the 'bmqu::SharedResource' must be used.  All callbacks having the
// shared resource bound and which will be invoked through the dispatcher must
// do so by using the 'e_DISPATCHER' dispatcher event type, to ensure the
// client will not be added to the dispatcher's flush list: the client may
// potentially have been destroyed prior to invocation of the callback.
//
// As soon as a queue handle has been dropped, it may be deleted.  Client
// session holds raw pointers to the handle.  There could be a race situation
// where Queue has enqueued some PUSH (or ACK) events to the dispatcher queue
// of the client; while the client dropped the handle (due to ungraceful
// shutdown), and the drop and handle deletion will be processed before those
// PUSH/ACK events, leading to dangling pointer access.  That is why the handle
// pointer is nilled immediately after a call to drop, and checked before
// access.

//
// The following variables are used for keeping track of the shutdown state:
//: o d_self:
//:   This is used, as described above, for the situation of remotely activated
//:   asynchronous messages, which can't be synchronized by enqueuing a marker
//:   in the dispatcher's queue.
//: o d_operationState:
//:   This enum is only set and checked in the client thread.  By default it
//:   has e_RUNNING value corresponding to the state when the session is
//:   running normally.  It is changed when the session is shutting down by one
//:   of the reasons:
//:   o e_SHUTTING_DOWN - when 'initiateShutdown' is called and graceful
//:     shutdown is performed.  All the queue handles get deconfigured and the
//:     session is waiting for the unconfirmed messages if there are any;
//:   o e_DISCONNECTING - when disconnect request comes from the client.  All
//:     the queue handles get dropped and when done the disconnect response is
//:     sent back to the client;
//:   o e_DISCONNECTED - in case of the channel went down, or right after
//:     sending the 'DisconnectResponse' message to the client.  Once set to
//:     e_DISCONNECTED, no messages should ever be delivered to the client.
//:   Since the above events may happen concurrently (e.g. disconnect request
//:   comes when the graceful shutdown is in progress) the following state
//:   transitions are possible:
//:   e_RUNNING       -> e_SHUTTING_DOWN
//:   e_RUNNING       -> e_DISCONNECTING
//:   e_RUNNING       -> e_DISCONNECTED
//:   e_SHUTTING_DOWN -> e_DISCONNECTING
//:   e_SHUTTING_DOWN -> e_DISCONNECTED
//:   e_DISCONNECTING -> e_DISCONNECTED
//:   This means that the e_SHUTTING_DOWN has the lowest priority, i.e. the
//:   graceful shutdown sequence is started only if the session was running
//:   normally, and the sequence is interrupted once disconnect or channel down
//:   events come.
//: o d_isDisconnecting:
//:   This boolean is only set and checked in 'processEvent', executing on the
//:   IO thread, to validate and safe-guard against a client misbehaving and
//:   sending messages after sending a 'Disconnect' request message.
//
// NOTE: the last 'hop' in the openQueue/closeQueue methods (the one that will
//       enqueue back to execute in the client dispatcher thread) doesn't need
//       to use the 'd_self', because it already holds the shared resource, and
//       in 'teardown', we ensure to drain the client dispatcher thread before
//       returning.  However that last method must verify the
//       'd_operationState' because after enqueuing to dispatch, the client may
//       be dying.
//
///'d_operationState' in onConfirmEvent / onPutEvent:
///------------------------------------------------------
// We should NOT check for 'd_operationState' enum in those two methods:
// those are downstream to upstream events and if the client sent us such
// messages, we should honor them even if the client crashed right after.
//
// 'd_operationState' is set to 'e_DISCONNECTED' under two conditions:
// 1) at the end of the processing of a client Disconnect request, after which,
//    per contract, no messages (especially no 'Confirm' nor 'Put' events are
//    expected to be received).  The 'd_isDisconnecting' check in
//    'processEvent' guarantees that.
// 2) when the channel drops, which is synchronizing on the client dispatcher
//    thread where this value is set.
//    In this case, the only possible sequence of event is the following:
//      - ProcessEvent(IO thread)
//          |-> enqueue 'onConfirmEvent/onPutEvent' to be processed on the
//              client dispatcher thread
//      - 'teardown' and then enqueuing a 'teardownImpl' to be processed on the
//        client dispatcher thread where d_operationState is set.  This can
//        only happen after 'processEvent' finished because both executes on
//        the IO thread.
//      - The client dispatcher queue now looks like:
//          |...|tearDownImpl|onConfirm/PutEvent| ->>
//        We therefore have guarantees that the onConfirm/PutEvent will be
//        processed before the queue handles are dropped (which happens in
//        teardownImpl).
//
// NOTE:
//   - Not checking the 'd_operationState' everytime in those methods is also a
//     welcome micro optimization as those are on the critical message path.
//   - for the case of 'PutEvent', we will have to check, and not send NACKs if
//     the 'd_operationState' is set to 'e_DISCONNECTED'.

// MQB
#include <mqbblp_clustercatalog.h>
#include <mqbblp_queueengineutil.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbi_cluster.h>
#include <mqbi_queue.h>
#include <mqbnet_tcpsessionfactory.h>
#include <mqbstat_brokerstats.h>
#include <mqbu_messageguidutil.h>

// BMQ
#include <bmqp_compression.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_controlmessageutil.h>
#include <bmqp_event.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocolutil.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqp_rejectmessageiterator.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

#include <bmqio_status.h>
#include <bmqst_statcontext.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_printutil.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_log.h>
#include <bdlb_nullablevalue.h>
#include <bdlb_scopeexit.h>
#include <bdld_datum.h>
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlf_bind.h>
#include <bdlf_noop.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqba {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.CLIENTSESSION");

const int k_NAGLE_PACKET_SIZE = 1024 * 1024;  // 1MB

/// This method does nothing other than calling the 'initiateShutdown' callback
/// if it is present; it is just used so that we can control when the session
/// can be destroyed, during the shutdown flow, by binding the specified
/// `session` (which is a shared_ptr to the session itself) to events being
/// enqueued to the dispatcher and let it die `naturally` when all threads are
/// drained up to the event.
void sessionHolderDummy(
    const mqba::ClientSession::ShutdownCb& shutdownCallback,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<void>& session)
{
    if (shutdownCallback) {
        shutdownCallback();
    }
}

/// Create the queue stats datum associated with the specified `statContext`
/// and having the specified `domain`, `cluster`, and `queueFlags`.
void createQueueStatsDatum(bmqst::StatContext* statContext,
                           const bsl::string&  domain,
                           const bsl::string&  cluster,
                           bsls::Types::Uint64 queueFlags)
{
    typedef bslma::ManagedPtr<bdld::ManagedDatum> ManagedDatumMp;
    bslma::Allocator* alloc = statContext->datumAllocator();
    ManagedDatumMp    datum = statContext->datum();

    bdld::DatumMapBuilder builder(alloc);
    builder.pushBack("queueFlags",
                     bdld::Datum::createInteger64(queueFlags, alloc));
    builder.pushBack("domain", bdld::Datum::copyString(domain, alloc));
    builder.pushBack("cluster", bdld::Datum::copyString(cluster, alloc));

    datum->adopt(builder.commit());
}

bmqp_ctrlmsg::ClientIdentity* extractClientIdentity(
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage)
// Based on the client type, return the pointer to the correct identity field
// of the specified 'negotiationMesage'.
{
    switch (negotiationMessage.selectionId()) {
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_CLIENT_IDENTITY: {
        return const_cast<bmqp_ctrlmsg::ClientIdentity*>(
            &negotiationMessage.clientIdentity());  // RETURN
    }
    case bmqp_ctrlmsg::NegotiationMessage::SELECTION_INDEX_BROKER_RESPONSE: {
        return const_cast<bmqp_ctrlmsg::ClientIdentity*>(
            &negotiationMessage.brokerResponse().brokerIdentity());  // RETURN
    }
    default: {
        BSLS_ASSERT_OPT(false && "Invalid negotiation message");
        return 0;  // RETURN
    }
    };
}

bool isClientGeneratingGUIDs(
    const bmqp_ctrlmsg::ClientIdentity& clientIdentity)
// Return true when the client identity 'guidInfo' struct contains non-empty
// 'clientId' field
{
    return !clientIdentity.guidInfo().clientId().empty();  // RETURN
}

// Finalize the specified 'handle' associated with the specified 'description'
void finalizeClosedHandle(bsl::string description,
                          const bsl::shared_ptr<mqbi::QueueHandle>& handle)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    BSLS_ASSERT_SAFE(
        handle->queue()->dispatcher()->inDispatcherThread(handle->queue()));

    BALL_LOG_INFO << description << ": Closed queue handle is finalized: "
                  << handle->handleParameters();
}

}  // close unnamed namespace

// -------------------------
// struct ClientSessionState
// -------------------------

ClientSessionState::ClientSessionState(
    bslma::ManagedPtr<bmqst::StatContext>& clientStatContext,
    BlobSpPool*                            blobSpPool,
    bdlbb::BlobBufferFactory*              bufferFactory,
    bmqp::EncodingType::Enum               encodingType,
    bslma::Allocator*                      allocator)
: d_allocator_p(allocator)
, d_channelBufferQueue(allocator)
, d_unackedMessageInfos(d_allocator_p)
, d_dispatcherClientData()
, d_statContext_mp(clientStatContext)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool)
, d_schemaEventBuilder(bufferFactory, allocator, encodingType)
, d_pushBuilder(bufferFactory, allocator)
, d_ackBuilder(bufferFactory, allocator)
, d_throttledFailedAckMessages()
, d_throttledFailedPutMessages()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(encodingType != bmqp::EncodingType::e_UNKNOWN);

    d_throttledFailedAckMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 5 seconds
    d_throttledFailedPutMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 5 seconds
}

// -------------------
// class ClientSession
// -------------------

// PRIVATE MANIPULATORS
void ClientSession::sendErrorResponse(
    bmqp_ctrlmsg::StatusCategory::Value failureCategory,
    const bslstl::StringRef&            errorDescription,
    const int                           code,
    const bmqp_ctrlmsg::ControlMessage& request)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    response.rId() = request.rId().value();
    response.choice().makeStatus();
    response.choice().status().category() = failureCategory;
    response.choice().status().message()  = errorDescription;
    response.choice().status().code()     = code;

    d_state.d_schemaEventBuilder.reset();
    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_ERROR << "#CLIENT_SEND_FAILURE " << description()
                       << ": Unable to send "
                       << request.choice().selectionName()
                       << " failure response [reason: ENCODING_FAILED, rc: "
                       << rc << "]: " << response;
        return;  // RETURN
    }

    BALL_LOG_INFO << description() << ": Sending "
                  << request.choice().selectionName()
                  << " failure response: " << response;

    // Send the response
    sendPacketDispatched(d_state.d_schemaEventBuilder.blob(), true);
}

void ClientSession::sendPacket(const bdlbb::Blob& blob, bool flushBuilders)
{
    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::sendPacketDispatched,
                             this,
                             blob,
                             flushBuilders),
        this);
}

void ClientSession::sendPacketDispatched(const bdlbb::Blob& blob,
                                         bool               flushBuilders)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // This method is the centralized *single* place where we should try to
    // send data to the client over the channel.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isDisconnected())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    // NOTE: By any means, blobs in the channelBufferQueue should be considered
    //       by other parts of the broker (queueHandle for example) as if they
    //       have been sent out to the client.  The sole purpose of this logic
    //       is to smooth out the send of huge burst of data, with respect to
    //       the Channel watermarks limits and the rate at which data can be
    //       written to the channel.  Messages should never be staying more
    //       than a couple milliseconds (or eventually seconds if there are
    //       gigabytes of data) in this buffer queue.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_state.d_channelBufferQueue.empty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // If the channelBuffer is not empty, we can't send, we have to enqueue
        // to guarantee ordering of messages.
        d_state.d_channelBufferQueue.push_back(blob);
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(flushBuilders)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // 'flushBuilders' should only be used when sending control messages,
        // so not on the likely path.

        flush();
        // If 'flush' wasn't able to send all the data, some might now be
        // buffered in the 'channelBufferQueue', so check for it again.
        if (!d_state.d_channelBufferQueue.empty()) {
            d_state.d_channelBufferQueue.push_back(blob);
            return;  // RETURN
        }
    }

    // Try to send the data, if we fail due to high watermark limit, enqueue
    // the message to the channelBufferQueue, and we'll send it later, once we
    // get the lowWatermark notification.
    bmqio::Status status;
    d_channel_sp->write(&status, blob);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            status.category() != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (status.category() == bmqio::StatusCategory::e_CONNECTION) {
            // This code relies on the fact that `e_CONNECTION` error cannot be
            // returned without calling `tearDown`.
            return;  // RETURN
        }
        if (status.category() == bmqio::StatusCategory::e_LIMIT) {
            BALL_LOG_WARN << "#CLIENT_SEND_FAILURE " << description()
                          << ": Failed to send data [size: "
                          << bmqu::PrintUtil::prettyNumber(blob.length())
                          << " bytes] to client due to channel watermark limit"
                          << "; enqueuing to the ChannelBufferQueue.";
            d_state.d_channelBufferQueue.push_back(blob);
        }
        else {
            BALL_LOG_INFO << "#CLIENT_SEND_FAILURE " << description()
                          << ": Failed to send data [size: "
                          << bmqu::PrintUtil::prettyNumber(blob.length())
                          << " bytes] to client with status: " << status;
        }
    }
}

void ClientSession::flushChannelBufferQueue()
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isDisconnected())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    BALL_LOG_INFO << description() << ": Flushing ChannelBufferQueue ("
                  << d_state.d_channelBufferQueue.size() << " items)";

    // Try to send as many data as possible
    while (!d_state.d_channelBufferQueue.empty()) {
        const bdlbb::Blob& blob = d_state.d_channelBufferQueue.front();

        bmqio::Status status;
        d_channel_sp->write(&status, blob);
        if (status.category() == bmqio::StatusCategory::e_LIMIT) {
            // We are hitting the limit again, can't continue.. stop sending
            // and we'll resume with the next lowWatermark notification.
            BALL_LOG_WARN
                << "#CLIENT_SEND_FAILURE " << description()
                << ": Failed to send data to client due to channel "
                << "watermark limit while flushing ChannelBufferQueue"
                   "; will continue later";
            break;  // BREAK
        }
        d_state.d_channelBufferQueue.pop_front();
    }
}

void ClientSession::sendAck(bmqt::AckResult::Enum    status,
                            int                      correlationId,
                            const bmqt::MessageGUID& messageGUID,
                            const QueueState*        queueState,
                            int                      queueId,
                            bool                     isSelfGenerated,
                            const bslstl::StringRef& source)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // NOTE: if this is the first hop, 'messageGUID' will be unset.  If this is
    //       not the first hop, correlationId will be NULL.  But this method
    //       does not care about them.

    bmqt::Uri uri;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(status !=
                                              bmqt::AckResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // When we receive a PUT event for an unknown queue (likely the queue
        // was already closed), we NACK it with e_INVALID_ARGUMENT; in this
        // case, the queue is expected not to be found in the d_queues map, and
        // therefore we expect to be passed a null pointer for the
        // 'queueState'; otherwise we expect a valid queue state, with a valid
        // non-closed queue handle.

        if (queueState) {
            BSLS_ASSERT_SAFE(queueState->d_handle_p);

            uri = queueState->d_handle_p->queue()->uri();

            // If queue is found, report locally generated NACK
            if (isSelfGenerated) {
                queueState->d_handle_p->queue()->stats()->onEvent(
                    mqbstat::QueueStatsDomain::EventType::e_NACK,
                    1);
            }
        }

        // Throttle error log if this is a 'failed Ack': note that we log at
        // INFO level in order not to overwhelm the dashboard, if a queue is
        // full, every post will nack, which could be a lot.
        if (d_state.d_throttledFailedAckMessages.requestPermission()) {
            BALL_LOG_INFO << description() << ": failed Ack "
                          << "[status: " << status << ", source: '" << source
                          << "'"
                          << ", correlationId: " << correlationId
                          << ", GUID: " << messageGUID << ", queue: '" << uri
                          << "' "
                          << "(id: " << queueId << ")]";
        }
    }

    // Always print at trace level
    BALL_LOG_TRACE << description() << ": sending Ack "
                   << "[status: " << status << ", source: '" << source << "'"
                   << ", correlationId: " << correlationId
                   << ", GUID: " << messageGUID << ", queue: '" << uri
                   << "' (id: " << queueId << ")]";

    // Append the ACK to the ackBuilder
    bmqt::EventBuilderResult::Enum rc = bmqp::ProtocolUtil::buildEvent(
        bdlf::BindUtil::bind(&bmqp::AckEventBuilder::appendMessage,
                             &d_state.d_ackBuilder,
                             bmqp::ProtocolUtil::ackResultToCode(status),
                             correlationId,
                             messageGUID,
                             queueId),
        bdlf::BindUtil::bind(&ClientSession::flush, this));

    if (rc != bmqt::EventBuilderResult::e_SUCCESS) {
        BALL_LOG_ERROR << "Failed to append ACK [rc: " << rc << ", source: '"
                       << source << "'"
                       << ", correlationId: " << correlationId
                       << ", GUID: " << messageGUID << ", queue: '" << uri
                       << "' (id: " << queueId << ")]";
    }

    if (d_state.d_ackBuilder.eventSize() >= k_NAGLE_PACKET_SIZE) {
        flush();
    }

    mqbstat::QueueStatsClient* queueStats = 0;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queueState == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Invalid/unknown queue
        queueStats = invalidQueueStats();
    }
    else {
        // Known queue (or subStream of the queue)
        StreamsMap::const_iterator subQueueCiter =
            queueState->d_subQueueInfosMap.findBySubIdSafe(
                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                subQueueCiter == queueState->d_subQueueInfosMap.end())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // Invalid/unknown subStream
            // Producer has closed the queue before receiving ACKs

            queueStats = invalidQueueStats();
        }
        else {
            queueStats = subQueueCiter->value().d_stats.get();
        }
    }

    queueStats->onEvent(mqbstat::QueueStatsClient::EventType::e_ACK, 1);
}

void ClientSession::tearDownImpl(bslmt::Semaphore*            semaphore,
                                 const bsl::shared_ptr<void>& session,
                                 bool                         isBrokerShutdown)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    BALL_LOG_INFO << description() << ": tearDownImpl";

    bdlb::ScopeExitAny guard(bdlf::BindUtil::bind(
        static_cast<void (bslmt::Semaphore::*)()>(&bslmt::Semaphore::post),
        semaphore));

    if (d_operationState == e_DEAD) {
        // Cannot do any work anymore
        return;  // RETURN
    }

    // If stop request handling is in progress cancel checking for the
    // unconfirmed messages.
    if (d_periodicUnconfirmedCheckHandler) {
        d_scheduler_p->cancelEventAndWait(d_periodicUnconfirmedCheckHandler);
    }

    d_self.invalidate();
    // Invalidating this CS in CS thread for the sake of synchronization
    // with `finishCheckUnconfirmed / finishCheckUnconfirmedDispatched` and
    // `checkUnconfirmed` / `checkUnconfirmedDispatched`.  Otherwise, they
    // need `weakMemFn`.

    // Stop the graceful shutdown chain (no-op if not started)
    d_shutdownChain.stop();
    d_shutdownChain.removeAll();

    // Drop all *applicable* queue handles, ie, all those handles for which
    // either a final close-queue request has not been received, or those
    // handles which haven't been dropped yet (eg, via
    // 'processDisconnectAllQueues').  Use 'drop' instead of 'release' because
    // client is going down, we don't need the QueueEngine to call us back to
    // notify the handle can be deleted because once this 'tearDownImpl' method
    // returns, this entire object is destroyed.  We also don't need to send a
    // close queue response since this release is not initiated by the client,
    // but by the broker upon client's disconnect.  Finally, we need to use
    // 'drop' to prevent a double release: if the client sent a closeQueue
    // request and immediately after the channel went down (i.e., the
    // closeQueue request is in process in the queue thread, so the handle is
    // still active at the time tearDown is processed).

    const bool hasLostTheClient = (!isBrokerShutdown && !isProxy());

    //    if (d_operationState == e_SHUTTING_DOWN_V2) {
    //        // Leave over queues and handles
    //    }
    //    else {
    // Set up the 'd_operationState' to indicate that the channel is dying and
    // we should not use it anymore trying to send any messages and should also
    // stop enqueuing 'callbacks' to the client dispatcher thread ...
    const bool doDeconfigure = d_operationState == e_RUNNING;

    int numHandlesDropped = dropAllQueueHandles(doDeconfigure,
                                                hasLostTheClient);
    BALL_LOG_INFO << description() << ": Dropped " << numHandlesDropped
                  << " queue handles.";
    //    }
    // Set up the 'd_operationState' to indicate that the channel is dying and
    // we should not use it anymore trying to send any messages and should also
    // stop enqueuing 'callbacks' to the client dispatcher thread ...
    d_operationState = e_DEAD;

    // Invalidating 'd_queueSessionManager' _after_ calling 'clearClient',
    // otherwise handle can get destructed because of
    // 'QueueSessionManager::onHandleReleased' early exit.
    d_queueSessionManager.tearDown();

    // Now that we enqueued a 'drop' for all applicable queue handles, we need
    // to synchronize on all queues, to make sure this drop event has been
    // processed.  We do so by enqueuing an event to all queues dispatchers
    // with the 'tearDownAllQueuesDone' finalize callback having the 'handle'
    // bound to it (so that the session is not yet destroyed).
    dispatcher()->execute(
        mqbi::Dispatcher::ProcessorFunctor(),  // empty
        mqbi::DispatcherClientType::e_QUEUE,
        bdlf::BindUtil::bind(&ClientSession::tearDownAllQueuesDone,
                             this,
                             session));
}

void ClientSession::tearDownAllQueuesDone(const bsl::shared_ptr<void>& session)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    // At this point, all queues dispatcher have been drained, we now need to
    // synchronize with the client's dispatcher thread: an event emanating from
    // a queue may have enqueued something to this client's events queue, so we
    // need to keep the session alive until all such events have been
    // processed.  Enqueue an event to the dispatcher associated to this
    // client, binding to it the 'handle' in the finalize callback.  Only after
    // this event has been processed will we have the guarantee that nothing
    // else references the session and we can therefore let it being destroyed,
    // by having the 'handle' go out of scope.  This dispatcher event must be
    // of type 'e_DISPATCHER' to make sure this client will not be added to the
    // dispatcher's flush list, since it is being destroyed.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&sessionHolderDummy, d_shutdownCallback, session),
        this,
        mqbi::DispatcherEventType::e_DISPATCHER);
}

void ClientSession::onHandleConfigured(
    const bmqp_ctrlmsg::Status&           status,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    const bmqp_ctrlmsg::ControlMessage&   streamParamsCtrlMsg)
{
    // executed by the *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::onHandleConfiguredDispatched,
                             this,
                             status,
                             streamParameters,
                             streamParamsCtrlMsg),
        this);
}

void ClientSession::onHandleConfiguredDispatched(
    const bmqp_ctrlmsg::Status&           status,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    const bmqp_ctrlmsg::ControlMessage&   streamParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    const unsigned int qId =
        streamParamsCtrlMsg.choice().isConfigureQueueStreamValue()
            ? streamParamsCtrlMsg.choice().configureQueueStream().qId()
            : streamParamsCtrlMsg.choice().configureStream().qId();

    ClientSessionState::QueueStateMap::iterator queueStateIter =
        d_queueSessionManager.queues().find(qId);
    if (queueStateIter != d_queueSessionManager.queues().end()) {
        d_currentOpDescription
            << "Configure queue '"
            << queueStateIter->second.d_handle_p->queue()->uri() << "'";
    }
    else {
        d_currentOpDescription << "Configure queue [qId=" << qId << "]";
    }

    if (isDisconnected()) {
        // The client is disconnected or the channel is down
        logOperationTime(d_currentOpDescription);
        return;  // RETURN
    }

    // Send success/error response to client
    bdlma::LocalSequentialAllocator<2048> localAlloc(d_state.d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          response(&localAlloc);

    response.rId() = streamParamsCtrlMsg.rId().value();

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failure ...
        response.choice().makeStatus(status);

        BALL_LOG_ERROR << "#CLIENT_CONFIGUREQUEUE_FAILURE " << description()
                       << ": Error while configuring queue: [reason: '"
                       << status << "', request: " << streamParamsCtrlMsg
                       << "]";
    }
    else {
        if (streamParamsCtrlMsg.choice().isConfigureQueueStreamValue()) {
            bmqp_ctrlmsg::ConfigureQueueStream& configureQueueStream =
                response.choice().makeConfigureQueueStreamResponse().request();

            configureQueueStream.qId() = qId;
            bmqp::ProtocolUtil::convert(
                &configureQueueStream.streamParameters(),
                streamParameters,
                streamParamsCtrlMsg.choice()
                    .configureQueueStream()
                    .streamParameters()
                    .subIdInfo());
        }
        else {
            bmqp_ctrlmsg::ConfigureStream& configureStream =
                response.choice().makeConfigureStreamResponse().request();

            configureStream.qId()              = qId;
            configureStream.streamParameters() = streamParameters;
        }

        if (queueStateIter == d_queueSessionManager.queues().end()) {
            // Failure to find queue
            BALL_LOG_WARN
                << "#CLIENT_UNKNOWN_QUEUE " << description()
                << ": Response to configure a queue with unknown Id (" << qId
                << ")";
        }
        else {
            queueStateIter->second.d_subQueueInfosMap.addSubscriptions(
                streamParameters);
        }
    }

    d_state.d_schemaEventBuilder.reset();
    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_WARN << "#CLIENT_SEND_FAILURE " << description()
                      << ": Unable to send configureQueue response [reason: "
                      << "ENCODING_FAILED, rc: " << rc
                      << ", request: " << streamParamsCtrlMsg
                      << "]: " << response;

        logOperationTime(d_currentOpDescription);
        return;  // RETURN
    }

    BALL_LOG_INFO << description()
                  << ": Sending configureQueue response: " << response
                  << " for request: " << streamParamsCtrlMsg;

    // Send the response
    sendPacketDispatched(d_state.d_schemaEventBuilder.blob(), true);

    logOperationTime(d_currentOpDescription);
}

void ClientSession::initiateShutdownDispatched(
    const ShutdownCb&         callback,
    const bsls::TimeInterval& timeout,
    bool                      supportShutdownV2)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(callback);

    BALL_LOG_INFO << description()
                  << ": initiateShutdownDispatched. Timeout: " << timeout;

    if (d_operationState == e_DEAD) {
        // The client is disconnected.  No need to wait for tearDown.
        callback();
        return;  // RETURN
    }

    if (d_operationState == e_SHUTTING_DOWN) {
        // More than one cluster calls 'initiateShutdown'?
        callback();
        return;  // RETURN
    }

    if (d_operationState == e_SHUTTING_DOWN_V2) {
        // More than one cluster calls 'initiateShutdown'?
        callback();
        return;  // RETURN
    }

    flush();  // Flush any pending messages

    // 'tearDown' should invoke the 'callback'
    d_shutdownCallback = callback;

    if (d_operationState == e_DISCONNECTING) {
        // Not teared down yet. No need to wait for unconfirmed messages.
        // Wait for tearDown.

        closeChannel();
        return;  // RETURN
    }

    if (supportShutdownV2) {
        d_operationState = e_SHUTTING_DOWN_V2;
        d_queueSessionManager.shutDown();

        callback();
    }
    else {
        // After de-configuring (below), wait for unconfirmed messages.
        // Once the wait for unconfirmed is over, close the channel

        ShutdownContextSp context;
        context.createInplace(
            d_state.d_allocator_p,
            bdlf::BindUtil::bind(&ClientSession::closeChannel, this),
            timeout);

        deconfigureAndWait(context);
    }
}

void ClientSession::deconfigureAndWait(ShutdownContextSp& context)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(context);

    // Use the same 'e_SHUTTING_DOWN' state for both shutting down and
    // StopRequest processing.

    d_operationState = e_SHUTTING_DOWN;

    // Fill the first link with handle deconfigure operations
    bmqu::OperationChainLink link(d_shutdownChain.allocator());

    for (QueueStateMapIter it = d_queueSessionManager.queues().begin();
         it != d_queueSessionManager.queues().end();
         ++it) {
        if (!it->second.d_hasReceivedFinalCloseQueue) {
            link.insert(
                bdlf::BindUtil::bind(&mqbi::QueueHandle::deconfigureAll,
                                     it->second.d_handle_p,
                                     bdlf::PlaceHolders::_1));
        }
    }
    // No-op if the link is empty
    d_shutdownChain.append(&link);

    d_shutdownChain.appendInplace(
        bdlf::BindUtil::bind(&ClientSession::checkUnconfirmed,
                             this,
                             context,
                             bdlf::PlaceHolders::_1));
    d_shutdownChain.start();
}

void ClientSession::invalidateDispatched()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_operationState == e_DEAD) {
        return;  // RETURN
    }

    d_self.invalidate();
    d_operationState = e_DEAD;
}

void ClientSession::checkUnconfirmed(const ShutdownContextSp& shutdownCtx,
                                     const VoidFunctor&       completionCb)
{
    // executed by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::checkUnconfirmedDispatched,
                             this,
                             shutdownCtx,
                             completionCb),
        this);
}

void ClientSession::checkUnconfirmedDispatched(
    const ShutdownContextSp& shutdownCtx,
    const VoidFunctor&       completionCb)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_operationState != e_RUNNING);
    BSLS_ASSERT_SAFE(completionCb);

    // Completion callback must be called to finish operation in the chain
    bdlb::ScopeExitAny guard(completionCb);

    // All the session substreams are deconfigured.  If the shutdown timeout is
    // expired or the client is disconnected skip checking of the unconfirmed
    // messages and break the shutdown sequence.
    const bool isDisconnected = d_operationState != e_SHUTTING_DOWN;
    if (bmqsys::Time::nowMonotonicClock() >= shutdownCtx->d_stopTime ||
        isDisconnected) {
        BALL_LOG_INFO << description()
                      << (isDisconnected ? ": the client is disconnected."
                                         : ": shutdown timeout has expired.")
                      << " Skip checking unconfirmed messages";

        d_periodicUnconfirmedCheckHandler.release();
        return;  // RETURN
    }

    // Reset unconfirmed messages counter
    shutdownCtx->d_numUnconfirmedTotal = 0;

    // Add countUnconfirmed operations
    bmqu::OperationChainLink link(d_shutdownChain.allocator());

    for (QueueStateMapIter it = d_queueSessionManager.queues().begin();
         it != d_queueSessionManager.queues().end();
         ++it) {
        if (!it->second.d_hasReceivedFinalCloseQueue) {
            link.insert(bdlf::BindUtil::bind(&ClientSession::countUnconfirmed,
                                             this,
                                             it->second.d_handle_p,
                                             shutdownCtx,
                                             bdlf::PlaceHolders::_1));
        }
    }
    // No-op if the link is empty
    d_shutdownChain.append(&link);

    d_shutdownChain.appendInplace(
        bdlf::BindUtil::bind(&ClientSession::finishCheckUnconfirmed,
                             this,
                             shutdownCtx,
                             bdlf::PlaceHolders::_1));
}

void ClientSession::countUnconfirmed(mqbi::QueueHandle*       handle,
                                     const ShutdownContextSp& shutdownCtx,
                                     const VoidFunctor&       completionCb)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    handle->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::countUnconfirmedDispatched,
                             this,
                             handle,
                             shutdownCtx,
                             completionCb),
        handle->queue());
}

void ClientSession::countUnconfirmedDispatched(
    mqbi::QueueHandle*       handle,
    const ShutdownContextSp& shutdownCtx,
    const VoidFunctor&       completionCb)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(shutdownCtx);
    BSLS_ASSERT_SAFE(completionCb);

    // It is safe to use the handle ptr here from the queue dispatcher thread
    // context because the execution of this method is scheduled over the valid
    // handle.  If the handle gets closed it will be released from the same
    // context (see 'finalizeClosedHandle'), i.e. after this method is invoked.

    shutdownCtx->d_numUnconfirmedTotal.add(handle->countUnconfirmed());

    // Completion callback must be called to finish operation in the chain.
    completionCb();
}

void ClientSession::finishCheckUnconfirmed(
    const ShutdownContextSp& shutdownCtx,
    const VoidFunctor&       completionCb)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::finishCheckUnconfirmedDispatched,
                             this,
                             shutdownCtx,
                             completionCb),
        this);
}

void ClientSession::finishCheckUnconfirmedDispatched(
    const ShutdownContextSp& shutdownCtx,
    const VoidFunctor&       completionCb)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_operationState != e_RUNNING);
    BSLS_ASSERT_SAFE(completionCb);

    // Completion callback must be called to finish operation in the chain
    bdlb::ScopeExitAny guard(completionCb);

    const bsls::TimeInterval nextCheckTime =
        bmqsys::Time::nowMonotonicClock().addSeconds(1);

    // If there are no unconfirmed messages, or if there is no more time to
    // wait for the confirms, or the client is disconnected  then finish the
    // shutdown sequence.
    if (shutdownCtx->d_stopTime < nextCheckTime ||
        shutdownCtx->d_numUnconfirmedTotal == 0 ||
        d_operationState != e_SHUTTING_DOWN) {
        BALL_LOG_INFO << description() << ": finish shutdown sequence having "
                      << shutdownCtx->d_numUnconfirmedTotal
                      << " unconfirmed messages";

        d_periodicUnconfirmedCheckHandler.release();
        return;  // RETURN
    }

    BALL_LOG_INFO << description() << ": Waiting for "
                  << shutdownCtx->d_numUnconfirmedTotal
                  << " unconfirmed messages. Next check at: [" << nextCheckTime
                  << "]. Timeout at: [" << shutdownCtx->d_stopTime << "]";

    // Schedule one more check for unconfirmed messages.
    d_scheduler_p->scheduleEvent(
        &d_periodicUnconfirmedCheckHandler,
        nextCheckTime,
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(&ClientSession::checkUnconfirmed,
                                           d_self.acquireWeak()),
            shutdownCtx,
            bdlf::noOp));
}

void ClientSession::closeChannel()
{
    // executed by *ANY* thread

    // Assume thread-safe read-only access to 'description()' and 'channel()'

    BALL_LOG_INFO << description() << ": Closing channel "
                  << channel()->peerUri();

    bmqio::Status status(
        bmqio::StatusCategory::e_SUCCESS,
        mqbnet::TCPSessionFactory::k_CHANNEL_STATUS_CLOSE_REASON,
        true,
        d_state.d_allocator_p);

    // Assume safe to call 'bmqio::Channel::close' on already closed channel.

    channel()->close(status);
}

void ClientSession::logOperationTime(bmqu::MemOutStream& opDescription)
{
    if (d_beginTimestamp) {
        BALL_LOG_INFO_BLOCK
        {
            const bsls::Types::Int64 elapsed =
                bmqsys::Time::highResolutionTimer() - d_beginTimestamp;
            BALL_LOG_OUTPUT_STREAM
                << description() << ": " << opDescription.str()
                << " took: " << bmqu::PrintUtil::prettyTimeInterval(elapsed)
                << " (" << elapsed << " nanoseconds)";
        }
        d_beginTimestamp = 0;
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "d_beginTimestamp was not initialized for operation: "
                      << opDescription.str();
    }

    opDescription.reset();
}

void ClientSession::processDisconnectAllQueues(
    const bmqp_ctrlmsg::ControlMessage& controlMessage)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (isDisconnected()) {
        // This means the client disconnected (ungraceful shutdown via channel
        // close) while in the process of a graceful shutdown (through
        // Disconnect request).
        return;  // RETURN
    }
    const bool doDeconfigure = d_operationState == e_RUNNING;
    d_operationState         = e_DISCONNECTING;

    // If stop request handling is in progress cancel checking for the
    // unconfirmed messages.
    if (d_periodicUnconfirmedCheckHandler) {
        d_scheduler_p->cancelEventAndWait(d_periodicUnconfirmedCheckHandler);
    }

    // Step 1/3 of disconnect request processing: executed following an enqueue
    // to the client dispatcher from the IO thread.  Drops all applicable
    // queues, and provides synchronisation with all the queues thread.

    // Drop all *applicable* queue handles ie, all those handles for which
    // either a final close-queue request has not been received, or those
    // handles which haven't been dropped yet.  Use 'drop' instead of 'release'
    // because the client is going down, we don't need the QueueEngine to call
    // us back to notify the handle can be deleted and we also don't need to
    // send a close queue response since this release is not initiated by the
    // client, but by the broker upon client's disconnect request.

    int numHandlesDropped = dropAllQueueHandles(doDeconfigure, false);

    BALL_LOG_INFO << description() << ": processing disconnect, dropped "
                  << numHandlesDropped << " queue handles.";

    // Schedule execution of step 2, by requesting the dispatcher to execute
    // 'processDisconnectAllQueuesDone' once all queues threads have been
    // drained.  Note, this must be using an 'e_DISPATCHER' dispatcher event
    // type, refer to top level documention for explanation (paragraph about
    // the bmqu::SharedResource).
    dispatcher()->execute(
        mqbi::Dispatcher::ProcessorFunctor(),  // empty
        mqbi::DispatcherClientType::e_QUEUE,
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(
                &ClientSession::processDisconnectAllQueuesDone,
                d_self.acquireWeak()),
            controlMessage));
}

void ClientSession::processDisconnectAllQueuesDone(
    const bmqp_ctrlmsg::ControlMessage& controlMessage)
{
    // executed by ONE of the *QUEUE* dispatcher threads

    // Step 2/3 of disconnect request processing: at this time, all queue
    // handles have been dropped, and all queues threads have been drained;
    // enqueue execution of the 'processDisconnect' method on the client
    // thread.

    dispatcher()->execute(
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(&ClientSession::processDisconnect,
                                           d_self.acquireWeak()),
            controlMessage),
        this);
}

void ClientSession::processDisconnect(
    const bmqp_ctrlmsg::ControlMessage& controlMessage)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_operationState != e_RUNNING);

    if (isDisconnected()) {
        // This means the client disconnected (ungraceful shutdown via channel
        // close) while in the process of a graceful shutdown (through
        // Disconnect request).
        return;  // RETURN
    }

    // Step 3/3 of disconnect request processing: send the disconnect response
    // to the client.

    // A graceful close message.. simply send back a 'disconnect' response as
    // an acknowledgement.
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    response.rId() = controlMessage.rId().value();
    response.choice().makeDisconnectResponse();

    d_state.d_schemaEventBuilder.reset();
    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_ERROR
            << "#CLIENT_SEND_FAILURE " << description()
            << ": Unable to send disconnect response [reason: ENCODING_FAILED,"
            << " rc: " << rc << "]: " << response;
        return;  // RETURN
    }

    BALL_LOG_INFO << description()
                  << ": Sending disconnect response: " << response;

    // Send the response
    sendPacketDispatched(d_state.d_schemaEventBuilder.blob(), true);

    // Setting the 'd_operationState' to 'e_DISCONNECTED' implies that no
    // messages will be pushed to the client after this one: we set it now
    // only, because up until the DisconnectResponse is being delivered we
    // should be sending messages to the client (such as closeQueue response,
    // ...).  And we set it after the above 'sendPacket' because that way we
    // can check 'd_operationState != e_DISCONNECTED' in sendPacket.
    d_operationState = e_DISCONNECTED;
    d_queueSessionManager.shutDown();
}

void ClientSession::processOpenQueue(
    const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_queueSessionManager.processOpenQueue(
        handleParamsCtrlMsg,
        bdlf::BindUtil::bind(&ClientSession::openQueueCb,
                             this,
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // handle
                             bdlf::PlaceHolders::_3,  // response
                             handleParamsCtrlMsg),
        bdlf::BindUtil::bind(&ClientSession::sendErrorResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // failureCategory
                             bdlf::PlaceHolders::_2,  // errorDescription
                             bdlf::PlaceHolders::_3,  // code
                             handleParamsCtrlMsg));
}

void ClientSession::openQueueCb(
    const bmqp_ctrlmsg::Status& status,
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    const bmqp_ctrlmsg::OpenQueueResponse&    openQueueResponse,
    const bmqp_ctrlmsg::ControlMessage&       handleParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(handleParamsCtrlMsg.choice().isOpenQueueValue());

    // Send success/error response to client
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    response.rId() = handleParamsCtrlMsg.rId().value();

    if (status.category() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        // Failure ...
        response.choice().makeStatus(status);

        BALL_LOG_WARN << "#CLIENT_OPENQUEUE_FAILURE " << description()
                      << ": Error while opening queue: [reason: " << status
                      << ", request: " << handleParamsCtrlMsg << "]";
    }
    else {
        bmqp_ctrlmsg::OpenQueueResponse& res =
            response.choice().makeOpenQueueResponse(openQueueResponse);
        res.originalRequest() = handleParamsCtrlMsg.choice().openQueue();
    }

    d_state.d_schemaEventBuilder.reset();
    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_WARN
            << "#CLIENT_SEND_FAILURE " << description()
            << ": Unable to send openQueue response [reason: ENCODING_FAILED, "
            << "rc: " << rc << ", request: " << handleParamsCtrlMsg
            << "]: " << response;
        return;  // RETURN
    }

    BALL_LOG_INFO << description()
                  << ": Sending openQueue response: " << response
                  << " for request: " << handleParamsCtrlMsg;

    // Send the response
    sendPacketDispatched(d_state.d_schemaEventBuilder.blob(), true);

    const bsl::string& queueUri =
        handleParamsCtrlMsg.choice().openQueue().handleParameters().uri();
    d_currentOpDescription << "Open queue '" << queueUri << "'";
    logOperationTime(d_currentOpDescription);
}

void ClientSession::processCloseQueue(
    const bmqp_ctrlmsg::ControlMessage& handleParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_queueSessionManager.processCloseQueue(
        handleParamsCtrlMsg,
        bdlf::BindUtil::bind(&ClientSession::closeQueueCb,
                             this,
                             bdlf::PlaceHolders::_1,  // handle
                             handleParamsCtrlMsg),
        bdlf::BindUtil::bind(&ClientSession::sendErrorResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // failureCategory
                             bdlf::PlaceHolders::_2,  // errorDescription
                             bdlf::PlaceHolders::_3,  // code
                             handleParamsCtrlMsg));
}

void ClientSession::closeQueueCb(
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const bmqp_ctrlmsg::ControlMessage&       handleParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(handleParamsCtrlMsg.choice().isCloseQueueValue());
    bdlma::LocalSequentialAllocator<2048> localAllocator(
        d_state.d_allocator_p);

    bmqp_ctrlmsg::ControlMessage response(&localAllocator);
    response.rId() = handleParamsCtrlMsg.rId().value();
    response.choice().makeCloseQueueResponse();

    d_state.d_schemaEventBuilder.reset();
    int rc = d_state.d_schemaEventBuilder.setMessage(
        response,
        bmqp::EventType::e_CONTROL);
    if (rc != 0) {
        BALL_LOG_ERROR
            << "#CLIENT_SEND_FAILURE " << description()
            << ": Unable to send close queue failure response [reason: "
            << "ENCODING_FAILED, rc: " << rc << "]: " << response;
        return;  // RETURN
    }

    BALL_LOG_INFO << description()
                  << ": Sending closeQueue response: " << response;

    // Send the response.
    sendPacketDispatched(d_state.d_schemaEventBuilder.blob(), true);

    // Release the handle's ptr in the queue's context to guarantee that the
    // handle will be destroyed after all ongoing queue events are handled.
    // E.g. in case of graceful shutdown each handle may be checked for the
    // unconfirmed messages (see 'checkUnconfirmedDispatched').  This check is
    // done in the queue's dispatcher thread, but the handle may be dropped
    // right after this check is scheduled.  Releasing the handle in the
    // queue's thread allows to keep the handle alive until the check is
    // complete.
    //
    // NOTE: We copy and pass 'description()' string to the callback because
    //       this 'ClientSession' object might be already destroyed when event
    //       is executed.  See internal-ticket D173020032.
    //
    // NOTE: We use the e_DISPATCHER type because the handle gets destroyed,
    //       and the process of this event by the dispatcher should not add it
    //       to the flush list.
    handle->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&finalizeClosedHandle, description(), handle),
        handle->queue(),
        mqbi::DispatcherEventType::e_DISPATCHER);

    const bsl::string& queueUri =
        handleParamsCtrlMsg.choice().closeQueue().handleParameters().uri();
    d_currentOpDescription << "Close queue '" << queueUri << "'";
    logOperationTime(d_currentOpDescription);
}

void ClientSession::processConfigureStream(
    const bmqp_ctrlmsg::ControlMessage& streamParamsCtrlMsg)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (isDisconnected()) {
        return;  // RETURN
    }

    // TODO: temporarily
    bmqp_ctrlmsg::ConfigureStream  adaptor;
    bmqp_ctrlmsg::ConfigureStream& req = adaptor;

    if (streamParamsCtrlMsg.choice().isConfigureQueueStreamValue()) {
        bmqp::ProtocolUtil::convert(
            &adaptor,
            streamParamsCtrlMsg.choice().configureQueueStream());
    }
    else {
        req = streamParamsCtrlMsg.choice().configureStream();
    }

    ClientSessionState::QueueStateMap::iterator queueStateIter =
        d_queueSessionManager.queues().find(req.qId());
    if (queueStateIter == d_queueSessionManager.queues().end()) {
        // Failure to find queue
        BALL_LOG_WARN << "#CLIENT_UNKNOWN_QUEUE " << description()
                      << ": Requested to configure a queue with unknown Id ("
                      << req.qId() << ")";

        sendErrorResponse(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                          "No queue with specified Id",
                          0,
                          streamParamsCtrlMsg);
        return;  // RETURN
    }

    ClientSessionState::QueueState& queueState = queueStateIter->second;
    if (queueState.d_hasReceivedFinalCloseQueue) {
        // Unexpected configureQueue request.  A closeQueue request with
        // 'isFinal' flag set to true was already received previously for this
        // queue from this client.  One scenario where this can occur is when
        // the client is a proxy, and proxy fails over to this node and sends
        // openQueue request as part of state-restore step, while a client
        // connected to that proxy drops the same queue (handle) such that
        // proxy sends a closeQueue request (with isFinal=true) flag, followed
        // by the subsequent configureQueue (this) request as part of
        // state-restore step.  Such configureQueue request should be
        // rejected.

        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                      << ": Requested to configure a queue with Id ("
                      << req.qId()
                      << "), for which a final 'closeQueue' request"
                      << " was already received.";
        sendErrorResponse(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                          "Unexpected configureQueue request",
                          0,
                          streamParamsCtrlMsg);
        return;  // RETURN
    }

    mqbi::QueueHandle* handle = queueState.d_handle_p;
    BSLS_ASSERT_SAFE(handle);

    StreamsMap::iterator subQInfoIter =
        queueStateIter->second.d_subQueueInfosMap.findByAppIdSafe(
            req.streamParameters().appId());
    if (subQInfoIter == queueStateIter->second.d_subQueueInfosMap.end()) {
        // Failure to find subStream of the queue.
        BALL_LOG_WARN << "#CLIENT_UNKNOWN_QUEUE " << description()
                      << ": Requested to configure a queue " << req.qId()
                      << " with unknown appId "
                      << req.streamParameters().appId();

        sendErrorResponse(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                          "No queue with specified appId",
                          0,
                          streamParamsCtrlMsg);
        return;  // RETURN
    }

    // Configure the queueHandle.
    BALL_LOG_INFO << description() << ": Requested to configure queue "
                  << handle->queue()->description() << ": "
                  << streamParamsCtrlMsg;

    // QE should validate consumerPriority and consumerPriorityCount
    handle->configure(
        req.streamParameters(),
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(&ClientSession::onHandleConfigured,
                                           d_self.acquireWeak()),
            bdlf::PlaceHolders::_1,  // status
            bdlf::PlaceHolders::_2,  // streamParams
            streamParamsCtrlMsg));
}

void ClientSession::onAckEvent(const mqbi::DispatcherAckEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // NOTE: we do not log anything here, all logging is done in 'sendAck'.

    const bmqp::AckMessage& ackMessage    = event.ackMessage();
    int                     correlationId = ackMessage.correlationId();

    QueueStateMapCIter queueStateCiter = d_queueSessionManager.queues().find(
        ackMessage.queueId());
    BSLS_ASSERT_SAFE(queueStateCiter != d_queueSessionManager.queues().end());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !queueStateCiter->second.d_handle_p)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO
            << description()
            << ": Dropping received ACK due to client ungracefully went down "
            << "for queue, [queueId: " << ackMessage.queueId()
            << ", GUID: " << ackMessage.messageGUID()
            << ", status: " << ackMessage.status() << "]";
        return;  // RETURN
    }

    mqbi::Queue* queue = queueStateCiter->second.d_handle_p->queue();

    if (handleRequesterContext()->isFirstHop()) {
        ClientSessionState::UnackedMessageInfoMap::const_iterator cit =
            d_state.d_unackedMessageInfos.find(ackMessage.messageGUID());

        if (cit != d_state.d_unackedMessageInfos.end()) {
            // Calculate time delta between PUT and ACK
            const bsls::Types::Int64 timeDelta =
                bmqsys::Time::highResolutionTimer() - cit->second.d_timeStamp;
            queue->stats()->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_ACK_TIME,
                timeDelta);

            if (!d_isClientGeneratingGUIDs) {
                // Legacy client.
                // Retrieve correlationId from GUID to pass on to the client.
                correlationId = cit->second.d_correlationId;
            }
            else {
                // Set the correlationId to null explicitly because the client
                // generated the GUID itself and did not specify a
                // correlationId for the corresponding PUT message.
                correlationId = bmqp::AckMessage::k_NULL_CORRELATION_ID;
            }

            d_state.d_unackedMessageInfos.erase(cit);
        }
        else {
            if (ackMessage.status() == 0) {
                // Received a positive acknowledgement that was not requested
                // by the client, drop it.  This happens because the broker to
                // broker communication always request ack (see document about
                // e_ACK_REQUESTED in onPutEvent).
                return;  // RETURN
            }

            // No correlationId for this GUID.  This means that client did not
            // specify a correlationId for the corresponding PUT message and
            // therefore did not manifest interest in acknowledgement.
            // However, for failure we always send an ACK back to the client,
            // so set the correlationId to null explicitly.
            correlationId = bmqp::AckMessage::k_NULL_CORRELATION_ID;
        }
    }
    // If we have at-most-once delivery we shouldn't be receiving any acks.
    if (queue->isAtMostOnce()) {
        BALL_LOG_WARN
            << "#CLIENT_UNEXPECTED_ACK " << description()
            << ": Received unexpected ack for at-most-once queue, [queueId: "
            << ackMessage.queueId() << ", GUID: " << ackMessage.messageGUID()
            << ", status: " << ackMessage.status() << "]";
        return;  // RETURN
    }

    sendAck(bmqp::ProtocolUtil::ackResultFromCode(ackMessage.status()),
            correlationId,
            ackMessage.messageGUID(),
            &(queueStateCiter->second),
            ackMessage.queueId(),
            false,  // isSelfGenerated
            "onAckEvent");
}

void ClientSession::onConfirmEvent(const mqbi::DispatcherConfirmEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // NOTE: Refer to implementation notes at the top of this file, section
    //       'onConfirmEvent/onPutEvent' for why we do not check for
    //       'd_operationState' here.

    bmqp::Event rawEvent(event.blob().get(), d_state.d_allocator_p);

    bmqp::ConfirmMessageIterator confIt;
    rawEvent.loadConfirmMessageIterator(&confIt);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!confIt.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "#CORRUPTED_EVENT " << description()
                                   << ": received an invalid CONFIRM event\n";
            confIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
        return;  // RETURN
    }

    int msgNum = 0;
    int rc     = 0;

    bdlma::LocalSequentialAllocator<256> localAllocator(d_state.d_allocator_p);
    bmqu::MemOutStream                   errorStream(&localAllocator);

    while ((rc = confIt.next()) == 1) {
        const int          id    = confIt.message().queueId();
        const unsigned int subId = static_cast<unsigned int>(
            confIt.message().subQueueId());
        const bmqp::QueueId queueId(id, subId);
        mqbi::QueueHandle*  queueHandle = 0;

        bool isValid = validateMessage(&queueHandle,
                                       &errorStream,
                                       queueId,
                                       bmqp::EventType::e_CONFIRM);

        if (isValid) {
            BSLS_ASSERT_SAFE(queueHandle);

            BALL_LOG_TRACE << description() << ": Confirm message #"
                           << ++msgNum << " [queue: '"
                           << queueHandle->queue()->uri()
                           << "' GUID: " << confIt.message().messageGUID()
                           << "]";

            queueHandle->confirmMessage(confIt.message().messageGUID(), subId);
        }
        else {
            BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                          << ": Received CONFIRM message " << errorStream.str()
                          << " [queue: '"
                          << (queueHandle ? queueHandle->queue()->uri()
                                          : "<UNKNOWN>")
                          << "', queueId: " << queueId
                          << ", GUID: " << confIt.message().messageGUID()
                          << "].";
            errorStream.reset();
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "#CORRUPTED_EVENT " << description()
                << ": Invalid ConfirmMessage event [rc: " << rc << "]\n";
            confIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
    }
}

void ClientSession::onRejectEvent(const mqbi::DispatcherRejectEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (handleRequesterContext()->isFirstHop()) {
        BALL_LOG_ERROR << "#CLIENT_UNEXPECTED_EVENT " << description()
                       << ": Unexpected 'REJECT' received from SDK.";
        return;  // RETURN
    }

    bmqp::Event rawEvent(event.blob().get(), d_state.d_allocator_p);

    bmqp::RejectMessageIterator rejectIt;
    rawEvent.loadRejectMessageIterator(&rejectIt);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!rejectIt.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "#CORRUPTED_EVENT " << description()
                                   << ": received an invalid REJECT event\n";
            rejectIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
        return;  // RETURN
    }

    int msgNum = 0;
    int rc     = 0;

    bdlma::LocalSequentialAllocator<256> localAllocator(d_state.d_allocator_p);
    bmqu::MemOutStream                   errorStream(&localAllocator);

    while ((rc = rejectIt.next()) == 1) {
        const int          id    = rejectIt.message().queueId();
        const unsigned int subId = static_cast<unsigned int>(
            rejectIt.message().subQueueId());
        const bmqp::QueueId queueId(id, subId);
        mqbi::QueueHandle*  queueHandle = 0;

        bool isValid = validateMessage(&queueHandle,
                                       &errorStream,
                                       queueId,
                                       bmqp::EventType::e_REJECT);

        if (isValid) {
            BSLS_ASSERT_SAFE(queueHandle);

            BALL_LOG_TRACE << description() << ": Reject message #" << ++msgNum
                           << " [queue: '" << queueHandle->queue()->uri()
                           << "', GUID: " << rejectIt.message().messageGUID()
                           << "]";

            queueHandle->rejectMessage(rejectIt.message().messageGUID(),
                                       queueId.subId());
        }
        else {
            BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                          << ": Received REJECT message " << errorStream.str()
                          << " [queue: '"
                          << (queueHandle ? queueHandle->queue()->uri()
                                          : "<UNKNOWN>")
                          << "', queueId: " << queueId
                          << ", GUID: " << rejectIt.message().messageGUID()
                          << "].";
            errorStream.reset();
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "#CORRUPTED_EVENT " << description()
                << ": Invalid RejectMessage event [rc: " << rc << "]\n";
            rejectIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
    }
}

bool ClientSession::validateMessage(mqbi::QueueHandle**   queueHandle,
                                    bsl::ostream*         errorStream,
                                    const bmqp::QueueId&  queueId,
                                    bmqp::EventType::Enum eventType)
{
    BSLS_ASSERT_SAFE((eventType == bmqp::EventType::e_CONFIRM ||
                      eventType == bmqp::EventType::e_REJECT) &&
                     "Unsupported eventType");

    ClientSessionState::QueueStateMap::iterator queueIt =
        d_queueSessionManager.queues().find(queueId.id());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            queueIt == d_queueSessionManager.queues().end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (eventType == bmqp::EventType::e_CONFIRM) {
            // Update invalid queue stats
            invalidQueueStats()->onEvent(
                mqbstat::QueueStatsClient::EventType::e_CONFIRM,
                1);
        }

        *errorStream << "for an unknown queue";

        return false;  // RETURN
    }

    StreamsMap::iterator subQueueIt =
        queueIt->second.d_subQueueInfosMap.findBySubIdSafe(queueId.subId());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            subQueueIt == queueIt->second.d_subQueueInfosMap.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (eventType == bmqp::EventType::e_CONFIRM) {
            // Update invalid queue stats
            invalidQueueStats()->onEvent(
                mqbstat::QueueStatsClient::EventType::e_CONFIRM,
                1);
        }

        *errorStream << "for an unknown subQueueId";

        return false;  // RETURN
    }

    *queueHandle = queueIt->second.d_handle_p;
    BSLS_ASSERT_SAFE(queueHandle);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            queueIt->second.d_hasReceivedFinalCloseQueue)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        *errorStream << "for queue for which final closeQueue request was "
                        "already received";

        return false;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            (*queueHandle)->queue()->isAtMostOnce())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        *errorStream << "for queue in broadcast mode";

        return false;  // RETURN
    }

    // Can be optimized out
    if ((*queueHandle)->queue()->hasMultipleSubStreams()) {
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !bmqp::QueueUtil::isValidFanoutConsumerSubQueueId(
                    queueId.subId()))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            *errorStream << "for queue in fanout mode with invalid subId";

            return false;  // RETURN
        }
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                 bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID != queueId.subId())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // SubId must be the default one in non-fanout mode.
        *errorStream << "for queue in non-fanout mode with invalid subId";

        return false;  // RETURN
    }

    if (eventType == bmqp::EventType::e_CONFIRM) {
        // Update stats for the queue (or subStream of the queue)
        subQueueIt->value().d_stats->onEvent(
            mqbstat::QueueStatsClient::EventType::e_CONFIRM,
            1);
    }

    return true;
}

void ClientSession::onPushEvent(const mqbi::DispatcherPushEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    static const int k_PAYLOAD_DUMP = 48;  // How much first bytes of the
                                           // messages payload to dump in TRACE

    BSLS_ASSERT_SAFE(event.blob());
    ClientSessionState::QueueStateMap::const_iterator citer =
        d_queueSessionManager.queues().find(event.queueId());
    BSLS_ASSERT_SAFE(citer != d_queueSessionManager.queues().end());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!citer->second.d_handle_p)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // This is likely due to the client ungracefully went down while there
        // were in flight push event being delivered; no need to log anything,
        // messages will be redelivered by upstream to another client.
        return;  // RETURN
    }

    bdlbb::Blob* blob = event.blob().get();

    if (BALL_LOG_IS_ENABLED(ball::Severity::TRACE)) {
        ClientSessionState::QueueStateMap::const_iterator it =
            d_queueSessionManager.queues().find(event.queueId());

        // If there are multiple subStreams, we add the corresponding
        // subQueueInfos.
        for (size_t i = 0; i < event.subQueueInfos().size(); ++i) {
            BALL_LOG_TRACE << description() << ": PUSH'ing message "
                           << "[queue: '"
                           << it->second.d_handle_p->queue()->uri() << "'"
                           << ", subQueueInfo: " << event.subQueueInfos()[i]
                           << ", GUID: " << event.guid() << "]:\n"
                           << bmqu::BlobStartHexDumper(blob, k_PAYLOAD_DUMP);
        }
    }

    bmqp::MessagePropertiesInfo pushProperties(event.messagePropertiesInfo());

    if (!event.msgGroupId().empty()) {
        d_state.d_pushBuilder.addMsgGroupIdOption(event.msgGroupId());
    }

    // Append subQueueInfos

    // Add SubQueueIds option and also indicate to the builder to attach
    // RDA counters in the option, unless this client session represents an
    // SDK client.  We will continue to send old style SubQueueIds option
    // for a while.
    d_state.d_pushBuilder.addSubQueueInfosOption(
        event.subQueueInfos(),
        !handleRequesterContext()->isFirstHop());

    bdlbb::Blob buffer(d_state.d_bufferFactory_p, d_state.d_allocator_p);
    bmqt::CompressionAlgorithmType::Enum cat =
        event.compressionAlgorithmType();
    int convertingRc = 0;

    // Append the message to the builder
    if (pushProperties.isExtended()) {
        if (!bmqp::ProtocolUtil::hasFeature(
                bmqp::MessagePropertiesFeatures::k_FIELD_NAME,
                bmqp::MessagePropertiesFeatures::k_MESSAGE_PROPERTIES_EX,
                d_clientIdentity_p->features())) {
            // Re-encode 'payload'
            // Convert MessageProperties into the old style
            // 1. Copy MPHs (if needed)
            // 2. Overwrite MPHs
            // 3. Decompress (if needed)
            convertingRc = bmqp::ProtocolUtil::convertToOld(
                &buffer,
                blob,
                cat,
                d_state.d_bufferFactory_p,
                d_state.d_allocator_p);

            pushProperties = bmqp::MessagePropertiesInfo::makeNoSchema();

            cat = bmqt::CompressionAlgorithmType::e_NONE;
            if (buffer.length()) {
                blob = &buffer;
            }
        }
    }

    if (convertingRc == 0) {
        int flags = 0;

        if (event.isOutOfOrderPush()) {
            bmqp::PushHeaderFlagUtil::setFlag(
                &flags,
                bmqp::PushHeaderFlags::e_OUT_OF_ORDER);
        }

        d_state.d_pushBuilder.packMessage(*blob,
                                          event.queueId(),
                                          event.guid(),
                                          flags,
                                          cat,
                                          pushProperties);

        // Flush if the builder is 'full'
        if (d_state.d_pushBuilder.eventSize() >= k_NAGLE_PACKET_SIZE) {
            flush();
        }

        // Finally, Update stats
        // TODO: Extract this and the version from 'mqbblp::Cluster' to a
        // function
        for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
             i < event.subQueueInfos().size();
             ++i) {
            StreamsMap::const_iterator subQueueCiter =
                citer->second.d_subQueueInfosMap.findBySubscriptionIdSafe(
                    event.subQueueInfos()[i].id());

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    subQueueCiter == citer->second.d_subQueueInfosMap.end())) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

                // subStream of the queue not found
                BALL_LOG_ERROR
                    << "#CLIENT_INVALID_PUSH " << description()
                    << ": PUSH for an unknown subStream of the queue [queue: '"
                    << citer->second.d_handle_p->queue()->uri()
                    << "', subQueueInfo: " << event.subQueueInfos()[i]
                    << ", GUID: " << event.guid() << "]:\n"
                    << bmqu::BlobStartHexDumper(blob, k_PAYLOAD_DUMP);

                invalidQueueStats()->onEvent(
                    mqbstat::QueueStatsClient::EventType::e_PUSH,
                    blob->length());
            }
            else {
                subQueueCiter->value().d_stats->onEvent(
                    mqbstat::QueueStatsClient::EventType::e_PUSH,
                    blob->length());
            }
        }
    }
    else {
        // REVISIT: This code does not update the client stats on invalid PUSH

        bsl::string filepath;
        int         dumpRc = mqbblp::QueueEngineUtil::dumpMessageInTempfile(
            &filepath,
            *event.blob().get(),
            0,
            d_state.d_bufferFactory_p);

        // REVISIT: An alternative is to send Reject upstream

        if (dumpRc == 0) {
            BMQTSK_ALARMLOG_ALARM("CLIENT_INVALID_PUSH")
                << description() << ": error '" << convertingRc
                << "' converting to old format [queue: '"
                << citer->second.d_handle_p->queue()->uri()
                << "', GUID: " << event.guid()
                << ", compressionAlgorithmType: "
                << event.compressionAlgorithmType()
                << "] Message was dumped in file at location [" << filepath
                << "] on this machine." << BMQTSK_ALARMLOG_END;
        }
        else {
            BMQTSK_ALARMLOG_ALARM("CLIENT_INVALID_PUSH")
                << description() << ": error '" << convertingRc
                << "' converting to old format [queue: '"
                << citer->second.d_handle_p->queue()->uri()
                << "', GUID: " << event.guid()
                << ", compressionAlgorithmType: "
                << event.compressionAlgorithmType()
                << "] Attempt to dump message in a file failed with error "
                << dumpRc << BMQTSK_ALARMLOG_END;
        }
    }
}

void ClientSession::onPutEvent(const mqbi::DispatcherPutEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // IMPLEMENTATION NOTES:
    //
    /// ACKs
    ///----
    // Message GUIDs will be used as identifiers for PUT and ACK messages
    // from this point forward.  If this broker is first hop after the legacy
    // client, message GUID will be created here.  The GUID->correlationId
    // mapping is maintained in the first hop.

    //
    ///'d_operationState' NOT checked
    ///----------------------------------
    // Refer to implementation notes at the top of this file, section
    // 'onConfirmEvent/onPutEvent' for why we do not check for
    // 'd_operationState' here; but only before sending NACKs..

    // There are 3 possible scenarios:
    // 1. Old style PUT with compressed MPs;
    // 2. Old style PUT with not compressed MPs;
    // 3. New style PUT;
    //
    // 1. Forcibly decompress MPs.
    //    Note that the MPs still have the old format of length/offset.
    // 2. No effect.
    //    Note that the MPs still have the old format of length/offset.
    // 3. No effect.
    bmqp::Event rawEvent(event.blob().get(), d_state.d_allocator_p);

    // Third argument in PutMsgIterator below conveys if the iterator should
    // decomperss old style (v1) msg properties.  The value of this flag is
    // derived from broker config as well as broker version, with this logic:
    //
    // - If brokerVersion is 999999 (developer workflow, CI, Jenkins, etc),
    //   use true, irrespective of flag's value specified in the configuration.
    //
    // - If brokerVersion is not 999999 (dev and non-dev deployments), use the
    //   value from configuration.

    bool isDecompressingOldMPs = false;

    if (mqbcfg::BrokerConfig::get().brokerVersion() == 999999) {
        isDecompressingOldMPs = true;
    }
    else if (mqbcfg::BrokerConfig::get()
                 .messagePropertiesV2()
                 .advertiseV2Support()) {
        isDecompressingOldMPs = true;
    }

    bmqp::PutMessageIterator putIt(d_state.d_bufferFactory_p,
                                   d_state.d_allocator_p,
                                   isDecompressingOldMPs);

    BSLS_ASSERT_SAFE(rawEvent.isPutEvent());
    rawEvent.loadPutMessageIterator(&putIt);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!putIt.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "#CORRUPTED_EVENT " << description()
                                   << ": received an invalid PUT event\n";
            putIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
        }
        return;  // RETURN
    }

    int        rc         = 0;
    int        msgNum     = 0;
    const bool isFirstHop = handleRequesterContext()->isFirstHop();
    while ((rc = putIt.next()) == 1) {
        bmqp::PutHeader& putHeader = const_cast<bmqp::PutHeader&>(
            putIt.header());
        const bmqp::QueueId queueId(putHeader.queueId(),
                                    bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        QueueState*   queueStatePtr   = 0;
        SubQueueInfo* subQueueInfoPtr = 0;

        bsl::shared_ptr<bdlbb::Blob> appDataSp =
            d_state.d_blobSpPool_p->getObject();
        bsl::shared_ptr<bdlbb::Blob> optionsSp;

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !validatePutMessage(&queueStatePtr,
                                    &subQueueInfoPtr,
                                    &appDataSp,
                                    &optionsSp,
                                    putIt))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // Update invalid queue stats
            invalidQueueStats()->onEvent(
                mqbstat::QueueStatsClient::EventType::e_PUT,
                appDataSp->length());
            continue;  // CONTINUE
        }

        // Update stats for the queue (or subStream of the queue)
        BSLS_ASSERT_SAFE(queueStatePtr && subQueueInfoPtr);
        BSLS_ASSERT_SAFE(queueStatePtr->d_handle_p);

        subQueueInfoPtr->d_stats->onEvent(
            mqbstat::QueueStatsClient::EventType::e_PUT,
            appDataSp->length());

        const bool isAtMostOnce =
            queueStatePtr->d_handle_p->queue()->isAtMostOnce();
        int  flags          = putHeader.flags();
        bool isAckRequested = bmqp::PutHeaderFlagUtil::isSet(
            flags,
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        int correlationId = bmqp::AckMessage::k_NULL_CORRELATION_ID;

        // All good.  Post message to the queue.

        // Generate a GUID at the first hop if message's source is a
        // legacy SDK client.
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                isFirstHop && !d_isClientGeneratingGUIDs)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // A message will only go once through a first hop, so on
            // average, this is the unlikely case (except for localQueues).

            correlationId = putIt.header().correlationId();
            // Must read the correlationId before over-writting it with a
            // GUID since they share the bytes in the putHeader.

            mqbu::MessageGUIDUtil::generateGUID(
                const_cast<bmqt::MessageGUID*>(&putHeader.messageGUID()));

            // Checking the consistency of e_ACK_REQUESTED and correlationId.
            const bool correlationIdSet =
                bmqp::AckMessage::k_NULL_CORRELATION_ID != correlationId;

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isAckRequested !=
                                                      correlationIdSet)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

                BALL_LOG_WARN
                    << "#CLIENT_INVALID_PUT " << description()
                    << ": PUT message with incompatible correlationId and "
                    << "ACK_REQUESTED flag. correlationId: " << correlationId
                    << ", ackRequested: " << bsl::boolalpha << isAckRequested;

                // Align 'isAckRequested' flag with correlationId.
                isAckRequested = correlationIdSet;
            }
        }

        // If at-most-once queue and ack is requested, send an ack
        // immediately regardless of whether this is first hop or not.
        if (isAtMostOnce) {
            if (isAckRequested) {
                sendAck(bmqt::AckResult::e_SUCCESS,
                        correlationId,
                        putHeader.messageGUID(),
                        queueStatePtr,
                        putHeader.queueId(),
                        true,
                        "putEvent::atMostOnce");
            }

            // Align flags and 'correlationId' with current mode.  Reset
            // 'correlationId' and 'isAckRequested' so that we don't get any
            // further responses from upstream.  Also force clear the
            // e_ACK_REQUESTED flag because it shouldn't be propagated
            // upstream (if set).
            correlationId  = bmqp::AckMessage::k_NULL_CORRELATION_ID;
            isAckRequested = false;
            bmqp::PutHeaderFlagUtil::unsetFlag(
                &flags,
                bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        }
        else {
            // Force setting the e_ACK_REQUESTED flag.  Broker to Broker
            // communication has to keep an internal mapping from GUID to
            // source in order to route back any potential ACK that would
            // come; and there would be no way to clear that map if not
            // requesting an ACK (positive ACKs would then be silent).
            bmqp::PutHeaderFlagUtil::setFlag(
                &flags,
                bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        }
        putHeader.setFlags(flags);

        // Keep track of message arrival time as well as correlationId
        // specified by the legacy client.
        // If the GUID is already generated for the PUT message by the
        // client, then correlationId should be null.
        if (isFirstHop && isAckRequested) {
            ClientSessionState::UnackedMessageInfoMapInsertRc insertRc =
                d_state.d_unackedMessageInfos.insert(
                    bsl::make_pair(putHeader.messageGUID(),
                                   ClientSessionState::UnackedMessageInfo(
                                       correlationId,
                                       bmqsys::Time::highResolutionTimer())));
            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(insertRc.second ==
                                                      false)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

                // GUID collision :(
                // There are several scenarios when this can occur:
                // - client session generated non-unique GUIDs for
                //   two unacked PUTs
                // - SDK generated non-unique GUIDs for
                //   two unacked PUTs
                // - SDK sent the same message second time while the first
                //   one is still unacked
                //
                // In order to not lose the data, we do send the message
                // upstream and here we just log the error.
                if (d_state.d_throttledFailedPutMessages.requestPermission()) {
                    BALL_LOG_WARN
                        << "#UNACKED_PUT_GUID_COLLISION " << description()
                        << ": GUID collision detected "
                        << "for a PUT message at first hop for queue ["
                        << queueStatePtr->d_handle_p->queue()->uri()
                        << "], correlationId: " << correlationId
                        << ", message size: " << putIt.applicationDataSize()
                        << ", GUID: " << insertRc.first->first << ". "
                        << "This could be due to retransmitted PUT "
                        << "message.";
                }
            }
        }

        BALL_LOG_TRACE << description() << ": PUT message #" << ++msgNum
                       << " [queue: "
                       << queueStatePtr->d_handle_p->queue()->uri()
                       << ", GUID: " << putIt.header().messageGUID()
                       << ", flags: " << putIt.header().flags()
                       << ", message size: " << putIt.applicationDataSize()
                       << "]:\n"
                       << bmqu::BlobStartHexDumper(appDataSp.get(), 64);

        queueStatePtr->d_handle_p->postMessage(putIt.header(),
                                               appDataSp,
                                               optionsSp);
    }

    // Check if the PUT event was valid
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_state.d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM << "#CORRUPTED_EVENT " << description()
                                       << ": Invalid Put event [rc: " << rc
                                       << "]\n";
                putIt.dumpBlob(BALL_LOG_OUTPUT_STREAM);
            }
        }
    }
}

mqbstat::QueueStatsClient* ClientSession::invalidQueueStats()
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_state.d_invalidQueueStats.isNull())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        d_state.d_invalidQueueStats.makeValue();
        d_state.d_invalidQueueStats.value().initialize(
            "bmq://invalid/queue",
            d_state.d_statContext_mp.get(),
            d_state.d_allocator_p);
        // TBD: The queue uri should be '** INVALID QUEUE **', but that can
        //      only be done once the stats UI panel has been updated to
        //      support that.

        bmqst::StatContext* statContext =
            d_state.d_invalidQueueStats.value().statContext();
        createQueueStatsDatum(statContext, "invalid", "none", 0);
    }

    return &d_state.d_invalidQueueStats.value();
}

bool ClientSession::validatePutMessage(QueueState**   queueState,
                                       SubQueueInfo** subQueueInfo,
                                       bsl::shared_ptr<bdlbb::Blob>* appDataSp,
                                       bsl::shared_ptr<bdlbb::Blob>* optionsSp,
                                       const bmqp::PutMessageIterator& putIt)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(queueState);
    BSLS_ASSERT_SAFE(subQueueInfo);
    BSLS_ASSERT_SAFE(appDataSp);
    BSLS_ASSERT_SAFE(optionsSp);

    const bmqp::QueueId queueId(putIt.header().queueId(),
                                bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

    *queueState   = 0;
    *subQueueInfo = 0;

    QueueStateMapIter queueStateIter = d_queueSessionManager.queues().find(
        queueId.id());
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            queueStateIter != d_queueSessionManager.queues().end())) {
        // Queue is found
        *queueState = &(queueStateIter->second);

        // Populate subQueueInfo output ptr
        StreamsMap::iterator subQueueIter =
            (*queueState)->d_subQueueInfosMap.findBySubIdSafe(queueId.subId());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                subQueueIter == (*queueState)->d_subQueueInfosMap.end())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // default subStream not found
            *subQueueInfo = 0;
        }
        else {
            *subQueueInfo = &(subQueueIter->value());
        }
    }

    const bool isFirstHop = handleRequesterContext()->isFirstHop();

    bmqt::AckResult::Enum ackStatus      = bmqt::AckResult::e_SUCCESS;
    const char*           ackDescription = 0;

    // Retrieve the payload of that message
    int rc = putIt.loadApplicationData((*appDataSp).get());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_state.d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM
                    << "#CORRUPTED_EVENT " << description()
                    << ": Failed to load PUT message payload [queueId: "
                    << queueId;

                if (isFirstHop && !d_isClientGeneratingGUIDs) {
                    BALL_LOG_OUTPUT_STREAM << ", correlationId: "
                                           << putIt.header().correlationId();
                }
                else {
                    BALL_LOG_OUTPUT_STREAM << ", GUID: "
                                           << putIt.header().messageGUID();
                }
                BALL_LOG_OUTPUT_STREAM << ", rc: " << rc << "]";
            }
        }

        ackStatus      = bmqt::AckResult::e_INVALID_ARGUMENT;
        ackDescription = "putEvent::failedLoadApplicationData";
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(*queueState == 0 ||
                                                   *subQueueInfo == 0)) {
        // Check if queueId represents a valid queue (or subStream of the
        // queue)
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_state.d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_WARN << "#CLIENT_INVALID_PUT " << description()
                          << ": PUT message for queue with unknown Id ("
                          << queueId << ")";
        }

        ackStatus      = bmqt::AckResult::e_INVALID_ARGUMENT;
        ackDescription = "putEvent::unknownQueue";
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                 (*queueState)->d_hasReceivedFinalCloseQueue)) {
        // Has this client already sent the 'final' closeQueue request?
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_state.d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                          << ": PUT message for queue Id (" << queueId
                          << ") after final"
                          << " closeQueue request.";
        }

        ackStatus      = bmqt::AckResult::e_REFUSED;
        ackDescription = "putEvent::afterFinalCloseQueue";
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(putIt.hasOptions())) {
        // Retrieve the message options.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        *optionsSp = d_state.d_blobSpPool_p->getObject();
        // We operate on a PUT-event - so a copy-all options by using
        // 'loadOptions()' operation is ok right now.  Revisit this if
        // there are any options you don't want to propagate.
        rc = putIt.loadOptions((*optionsSp).get());
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            if (d_state.d_throttledFailedPutMessages.requestPermission()) {
                BALL_LOG_ERROR_BLOCK
                {
                    BALL_LOG_OUTPUT_STREAM << "#CORRUPTED_EVENT "
                                           << description()
                                           << ": Failed to load PUT message "
                                           << "options [queueId: " << queueId;
                    if (isFirstHop && !d_isClientGeneratingGUIDs) {
                        BALL_LOG_OUTPUT_STREAM
                            << ", correlationId: "
                            << putIt.header().correlationId();
                    }
                    else {
                        BALL_LOG_OUTPUT_STREAM << ", GUID: "
                                               << putIt.header().messageGUID();
                    }
                    BALL_LOG_OUTPUT_STREAM << ", rc: " << rc << "]";
                }
            }

            ackStatus      = bmqt::AckResult::e_INVALID_ARGUMENT;
            ackDescription = "putEvent::failedLoadOptions";
        }
    }
    else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                 isFirstHop && d_isClientGeneratingGUIDs &&
                 putIt.header().messageGUID().isUnset())) {
        // Check if SDK sent PUT with non-empty GUID
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (d_state.d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_ERROR << "#CLIENT_INVALID_PUT " << description()
                           << ": PUT message with unset GUID [queueId: "
                           << queueId << "]";
        }

        ackStatus      = bmqt::AckResult::e_INVALID_ARGUMENT;
        ackDescription = "putEvent::unsetGUID";
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(ackStatus !=
                                              bmqt::AckResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (isFirstHop && !d_isClientGeneratingGUIDs) {
            sendAck(ackStatus,
                    putIt.header().correlationId(),
                    bmqt::MessageGUID(),
                    *queueState,
                    queueId.id(),
                    true,  // isSelfGenerated
                    ackDescription);
        }
        else {
            sendAck(ackStatus,
                    bmqp::AckMessage::k_NULL_CORRELATION_ID,
                    putIt.header().messageGUID(),
                    *queueState,
                    queueId.id(),
                    true,  // isSelfGenerated
                    ackDescription);
        }

        return false;  // RETURN
    }

    return true;
}

// CREATORS
ClientSession::ClientSession(
    const bsl::shared_ptr<bmqio::Channel>&  channel,
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage,
    const bsl::string&                      sessionDescription,
    mqbi::Dispatcher*                       dispatcher,
    mqbblp::ClusterCatalog*                 clusterCatalog,
    mqbi::DomainFactory*                    domainFactory,
    bslma::ManagedPtr<bmqst::StatContext>&  clientStatContext,
    ClientSessionState::BlobSpPool*         blobSpPool,
    bdlbb::BlobBufferFactory*               bufferFactory,
    bdlmt::EventScheduler*                  scheduler,
    bslma::Allocator*                       allocator)
: d_self(this)  // use default allocator
, d_operationState(e_RUNNING)
, d_isDisconnecting(false)
, d_negotiationMessage(negotiationMessage, allocator)
, d_clientIdentity_p(extractClientIdentity(d_negotiationMessage))
, d_isClientGeneratingGUIDs(isClientGeneratingGUIDs(*d_clientIdentity_p))
, d_description(sessionDescription, allocator)
, d_channel_sp(channel)
, d_state(clientStatContext,
          blobSpPool,
          bufferFactory,
          bmqp::SchemaEventBuilderUtil::bestEncodingSupported(
              d_clientIdentity_p->features()),
          allocator)
, d_queueSessionManager(this,
                        *d_clientIdentity_p,
                        d_state.d_statContext_mp.get(),
                        domainFactory,
                        allocator)
, d_clusterCatalog_p(clusterCatalog)
, d_scheduler_p(scheduler)
, d_periodicUnconfirmedCheckHandler()
, d_shutdownChain(allocator)
, d_shutdownCallback()
, d_beginTimestamp(0)
, d_currentOpDescription(256, allocator)
{
    // Register this client to the dispatcher
    mqbi::Dispatcher::ProcessorHandle processor = dispatcher->registerClient(
        this,
        mqbi::DispatcherClientType::e_SESSION);

    // Observe the Channel for High/Low watermark notification.  We don't need
    // to unregister from the channel, because this Session has a longer
    // lifecycle than the Channel itself.
    channel->onWatermark(
        bdlf::BindUtil::bind(&ClientSession::onWatermark,
                             this,
                             bdlf::PlaceHolders::_1));  // type

    mqbstat::BrokerStats::instance().onEvent(
        mqbstat::BrokerStats::EventType::e_CLIENT_CREATED);

    BALL_LOG_INFO << description() << ": created "
                  << "[dispatcherProcessor: " << processor
                  << ", identity: " << *(d_clientIdentity_p)
                  << ", ptr: " << this << ", queueHandleRequesterId: "
                  << d_queueSessionManager.requesterContext()->requesterId()
                  << "].";
}

ClientSession::~ClientSession()
{
    // executed by ONE of the *QUEUE* dispatcher threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_self.isValid());
    BSLS_ASSERT_SAFE(d_shutdownChain.numOperations() == 0);

    BALL_LOG_INFO << description() << ": destructor";

    mqbstat::BrokerStats::instance().onEvent(
        mqbstat::BrokerStats::EventType::e_CLIENT_DESTROYED);

    // Unregister from the dispatcher
    dispatcher()->unregisterClient(this);

    d_currentOpDescription << "Close session";
    logOperationTime(d_currentOpDescription);
}

// MANIPULATORS
//   (virtual: mqbnet::Session)
void ClientSession::processEvent(
    const bmqp::Event&     event,
    BSLS_ANNOTATION_UNUSED mqbnet::ClusterNode* source)
{
    // executed by the *IO* thread

    if (event.isControlEvent()) {
        bdlma::LocalSequentialAllocator<2048> localAllocator(
            d_state.d_allocator_p);
        bmqp_ctrlmsg::ControlMessage controlMessage(&localAllocator);

        int rc = event.loadControlEvent(&controlMessage);
        if (rc != 0) {
            BALL_LOG_ERROR << "#CORRUPTED_EVENT " << description()
                           << ": Received invalid control message from client "
                           << "[reason: 'failed to decode', rc: " << rc
                           << "]:\n"
                           << bmqu::BlobStartHexDumper(event.blob());
            return;  // RETURN
        }

        BALL_LOG_INFO << description()
                      << ": Received control message: " << controlMessage;

        rc = bmqp::ControlMessageUtil::validate(controlMessage);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            // Malformed control message should be rejected
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_ERROR << "#CORRUPTED_CONTROL_MESSAGE " << description()
                           << ": Received malformed control message from "
                           << "client [reason: 'malformed fields', rc: " << rc
                           << "]";
            return;  // RETURN
        }

        // Control messages are enqueued to be processed in dispatcher thread.
        mqbi::Dispatcher::VoidFunctor eventCallback;

        typedef bmqp_ctrlmsg::ControlMessageChoice MsgChoice;  // shortcut

        const MsgChoice& choice = controlMessage.choice();

        switch (choice.selectionId()) {
        case MsgChoice::SELECTION_ID_DISCONNECT: {
            if (d_isDisconnecting) {
                BALL_LOG_ERROR
                    << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                    << ": received multiple Disconnect controlMessage";
                return;  // RETURN
            }

            d_beginTimestamp = bmqsys::Time::highResolutionTimer();

            d_isDisconnecting = true;
            eventCallback     = bdlf::BindUtil::bind(
                &ClientSession::processDisconnectAllQueues,
                this,
                controlMessage);
        } break;
        case MsgChoice::SELECTION_ID_OPEN_QUEUE: {
            d_beginTimestamp = bmqsys::Time::highResolutionTimer();

            eventCallback = bdlf::BindUtil::bind(
                &ClientSession::processOpenQueue,
                this,
                controlMessage);
        } break;
        case MsgChoice::SELECTION_ID_CLOSE_QUEUE: {
            d_beginTimestamp = bmqsys::Time::highResolutionTimer();

            eventCallback = bdlf::BindUtil::bind(
                &ClientSession::processCloseQueue,
                this,
                controlMessage);
        } break;
        case MsgChoice::SELECTION_ID_CONFIGURE_QUEUE_STREAM:
        case MsgChoice::SELECTION_ID_CONFIGURE_STREAM: {
            d_beginTimestamp = bmqsys::Time::highResolutionTimer();

            eventCallback = bdlf::BindUtil::bind(
                &ClientSession::processConfigureStream,
                this,
                controlMessage);
        } break;
        case MsgChoice::SELECTION_ID_DISCONNECT_RESPONSE:
        case MsgChoice::SELECTION_ID_OPEN_QUEUE_RESPONSE:
        case MsgChoice::SELECTION_ID_CLOSE_QUEUE_RESPONSE:
        case MsgChoice::SELECTION_ID_CONFIGURE_STREAM_RESPONSE:
        case MsgChoice::SELECTION_ID_CONFIGURE_QUEUE_STREAM_RESPONSE: {
            // We should not be receiving ANY response-like control message
            BALL_LOG_ERROR
                << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                << ": received unexpected 'response' controlMessage: "
                << controlMessage;
            return;  // RETURN
        }
        case MsgChoice::SELECTION_ID_CLUSTER_MESSAGE: {
            eventCallback = bdlf::BindUtil::bind(
                &ClientSession::processClusterMessage,
                this,
                controlMessage);
        } break;
        case MsgChoice::SELECTION_ID_UNDEFINED:
        case MsgChoice::SELECTION_ID_STATUS:
        default: {
            BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                           << ": unexpected controlMessage: "
                           << controlMessage;
            return;  // RETURN
        }
        }

        if (d_isDisconnecting &&
            choice.selectionId() != MsgChoice::SELECTION_ID_DISCONNECT) {
            BALL_LOG_ERROR
                << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                << ": Dropping control message received after a Disconnect "
                << "notification: " << controlMessage;
            return;  // RETURN
        }

        // Dispatch the event
        dispatcher()->execute(eventCallback, this);
        return;  // RETURN
    }
    else {
        if (d_isDisconnecting) {
            BALL_LOG_ERROR
                << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                << ": Dropping event received after a Disconnect notification:"
                << " " << event;
            return;  // RETURN
        }

        // Dispatch the event
        switch (event.type()) {
        case bmqp::EventType::e_PUT:
        case bmqp::EventType::e_CONFIRM:
        case bmqp::EventType::e_REJECT: {
            break;  // BREAK
        }
        default: {
            BALL_LOG_ERROR << "#CLIENT_UNEXPECTED_EVENT " << description()
                           << ": Unexpected event type: " << event;
            return;  // RETURN
        }
        }

        mqbi::DispatcherEvent*       dispEvent = dispatcher()->getEvent(this);
        bsl::shared_ptr<bdlbb::Blob> blobSp =
            d_state.d_blobSpPool_p->getObject();
        *blobSp = *(event.blob());

        if (event.isPutEvent()) {
            (*dispEvent).setSource(this).makePutEvent().setBlob(blobSp);
        }
        else if (event.isConfirmEvent()) {
            (*dispEvent).setSource(this).makeConfirmEvent().setBlob(blobSp);
        }
        else if (event.isRejectEvent()) {
            (*dispEvent).setSource(this).makeRejectEvent().setBlob(blobSp);
        }

        dispatcher()->dispatchEvent(dispEvent, this);
    }
}

void ClientSession::tearDown(const bsl::shared_ptr<void>& session,
                             bool                         isBrokerShutdown)
{
    // executed by the *IO* thread (except for UT)

    BALL_LOG_INFO << description() << ": tearDown";

    // Cancel the reads on the channel
    d_channel_sp->cancelRead();

    // Enqueue an event to the client dispatcher thread and wait for it to
    // finish; only after this will we have the guarantee that no method will
    // try to use the channel.
    bslmt::Semaphore semaphore;

    // NOTE: We use the e_DISPATCHER type because this Client is dying, and the
    //       process of this event by the dispatcher should not add it to the
    //       flush list.
    dispatcher()->execute(bdlf::BindUtil::bind(&ClientSession::tearDownImpl,
                                               this,
                                               &semaphore,
                                               session,
                                               isBrokerShutdown),
                          this,
                          mqbi::DispatcherEventType::e_DISPATCHER);
    semaphore.wait();

    // At this point, we are sure the client dispatcher thread has no pending
    // processing that could be using the channel, so we can return, which will
    // destroy the channel.  The session will die when all references to the
    // 'session' go out of scope.
}

void ClientSession::initiateShutdown(const ShutdownCb&         callback,
                                     const bsls::TimeInterval& timeout,
                                     bool supportShutdownV2)
{
    // executed by the *ANY* thread

    d_beginTimestamp = bmqsys::Time::highResolutionTimer();

    BALL_LOG_INFO << description() << ": initiateShutdown";

    // The 'd_self.acquire()' return 'shared_ptr<ClientSession>' but that does
    // not relate to the 'shared_ptr<mqbnet::Session>' acquired by
    // 'mqbnet::TransportManagerIterator'.  The latter is bound to the
    // 'initiateShutdown'.  The former can be null after 'd_self.invalidate()'
    // call. ('invalidate()' waits for all _acquired_ 'shared_ptr' references
    // to drop).
    //
    // We have a choice, either 1) bind the latter to 'initiateShutdown' to
    // make sure 'd_self.acquire()' returns not null, or 2) invoke the
    // 'callback' earlier if fail to 'acquire()' because of 'invalidate()', or
    // 3) bind _acquired_ 'shared_ptr' to 'initiateShutdown'.
    //
    // Choosing 2), assuming that calling the (completion) callback from a
    // thread other than the *CLIENT* dispatcher thread is ok.  The
    // 'bmqu::OperationChainLink' expects the completion callback from multiple
    // sessions anyway.

    bsl::shared_ptr<ClientSession> ptr = d_self.acquire();

    if (!ptr) {
        callback();
    }
    else {
        dispatcher()->execute(
            bdlf::BindUtil::bind(
                bdlf::MemFnUtil::memFn(
                    &ClientSession::initiateShutdownDispatched,
                    d_self.acquire()),
                callback,
                timeout,
                supportShutdownV2),
            this,
            mqbi::DispatcherEventType::e_DISPATCHER);
        // Use 'mqbi::DispatcherEventType::e_DISPATCHER' to avoid (re)enabling
        // 'd_flushList'
    }
}

void ClientSession::invalidate()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!dispatcher()->inDispatcherThread(this));

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::invalidateDispatched, this),
        this);
    dispatcher()->synchronize(this);
}

// MANIPULATORS
void ClientSession::onWatermark(bmqio::ChannelWatermarkType::Enum type)
{
    switch (type) {
    case bmqio::ChannelWatermarkType::e_LOW_WATERMARK: {
        onLowWatermark();
    } break;  // BREAK
    case bmqio::ChannelWatermarkType::e_HIGH_WATERMARK: {
        onHighWatermark();
    } break;  // BREAK
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown watermark type");
    }
    }
}

void ClientSession::onHighWatermark()
{
    // executed by the *IO* thread
    // When writting to the channel, the 'sendPacket()' returns limit if it
    // goes beyond the watermark provided at write; and this HighWatermark
    // notification is only emitted if it goes beyond the session's
    // highWatermark.  So we should never hit this notification because we are
    // using the channelBufferQueue and stop as soon as we get limit error on
    // write.
    BALL_LOG_INFO << description() << ": Channel HighWatermark hit.";
}

void ClientSession::onLowWatermark()
{
    // executed by the *IO* thread

    if (!d_self.isValid()) {
        // In the case of graceful shutdown, the disconnection is initiated by
        // the client, upon reception of the DisconnectResponse message from
        // the broker; which implies the channelBufferQueue as well as the IO
        // buffer were fully drained (since this is the last message delivered
        // by the broker).  Therefore, in the graceful shutdown situation, it's
        // fine to return now because there is nothing to flush anyway.  And in
        // the ungraceful shutdown, this flag set implies the channel is dead
        // and unusable, and that we should no longer try to use it nor should
        // we enqueue anything for processing to our own dispatcher thread.
        return;  // RETURN
    }

    // Enqueue an event to be processed in the client's DISPATCHER thread to
    // resume flushing data from the channelBufferQueue.
    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClientSession::flushChannelBufferQueue, this),
        this);
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
void ClientSession::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // NOTE: We don't perform 'd_operationState' check in this method because
    //       it might be desirable to dispatch certain callbacks to the client
    //       session object while it is being cleaned up.  Performing above
    //       check in each callback/method provides more flexibility.

    BALL_LOG_TRACE << description() << ": processing dispatcher event '"
                   << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_CONFIRM: {
        onConfirmEvent(event.getAs<mqbi::DispatcherConfirmEvent>());
    } break;
    case mqbi::DispatcherEventType::e_REJECT: {
        onRejectEvent(event.getAs<mqbi::DispatcherRejectEvent>());
    } break;
    case mqbi::DispatcherEventType::e_PUSH: {
        onPushEvent(event.getAs<mqbi::DispatcherPushEvent>());
    } break;
    case mqbi::DispatcherEventType::e_PUT: {
        onPutEvent(event.getAs<mqbi::DispatcherPutEvent>());
    } break;
    case mqbi::DispatcherEventType::e_ACK: {
        onAckEvent(event.getAs<mqbi::DispatcherAckEvent>());
    } break;
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            &event.getAs<mqbi::DispatcherCallbackEvent>();

        BSLS_ASSERT_SAFE(realEvent->callback());
        flush();  // Flush any pending messages to guarantee ordering of events
        realEvent->callback()(dispatcherClientData().processorHandle());
    } break;
    case mqbi::DispatcherEventType::e_CONTROL_MSG: {
        BSLS_ASSERT_OPT(false &&
                        "'CONTROL_MSG' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_DISPATCHER: {
        BSLS_ASSERT_OPT(false &&
                        "'DISPATCHER' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_UNDEFINED: {
        BSLS_ASSERT_OPT(false && "'NONE' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_CLUSTER_STATE: {
        BSLS_ASSERT_OPT(false &&
                        "'CLUSTER_STATE' dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_STORAGE: {
        BSLS_ASSERT_OPT(false && "'STORAGE' dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_RECOVERY: {
        BSLS_ASSERT_OPT(false &&
                        "'RECOVERY' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT: {
        BSLS_ASSERT_OPT(
            false && "'REPLICATION_RECEIPT' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    default: {
        BALL_LOG_ERROR
            << "#CLIENT_UNEXPECTED_EVENT " << description()
            << ": Session received unexpected dispatcher event [type: "
            << event.type() << "]";
    }
    };
}

void ClientSession::flush()
{
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // Start by flushing the data ('PUSH') messages.
    if (d_state.d_pushBuilder.messageCount() != 0) {
        BALL_LOG_TRACE << description() << ": Flushing "
                       << d_state.d_pushBuilder.messageCount()
                       << " PUSH messages";
        sendPacketDispatched(d_state.d_pushBuilder.blob(), false);
        d_state.d_pushBuilder.reset();
    }

    // Then flush the 'ACK' messages.
    if (d_state.d_ackBuilder.messageCount() != 0) {
        BALL_LOG_TRACE << description() << ": Flushing "
                       << d_state.d_ackBuilder.messageCount()
                       << " ACK messages";
        sendPacketDispatched(d_state.d_ackBuilder.blob(), false);
        d_state.d_ackBuilder.reset();
    }
}

void ClientSession::processClusterMessage(
    const bmqp_ctrlmsg::ControlMessage& message)
{
    if (message.choice().clusterMessage().choice().isStopResponseValue()) {
        BALL_LOG_INFO << description() << ": processStopResponse: " << message;
        d_clusterCatalog_p->stopRequestManger().processResponse(message);
    }
    else if (message.choice().clusterMessage().choice().isStopRequestValue()) {
        // This is StopRequest from Proxy
        // Assume StopRequest V2

        const bmqp_ctrlmsg::StopRequest& request =
            message.choice().clusterMessage().choice().stopRequest();

        BSLS_ASSERT_SAFE(request.version() == 2);

        // Deconfigure all queues.  Do NOT wait for unconfirmed

        BALL_LOG_INFO << description() << ": processing StopRequest.";

        bmqp_ctrlmsg::ControlMessage response;

        response.choice().makeClusterMessage().choice().makeStopResponse();
        response.rId() = message.rId();

        d_state.d_schemaEventBuilder.reset();
        int rc = d_state.d_schemaEventBuilder.setMessage(
            response,
            bmqp::EventType::e_CONTROL);

        if (rc != 0) {
            BALL_LOG_ERROR
                << "#CLIENT_SEND_FAILURE " << description()
                << ": Encoding StopRequest response has failed, [rc: " << rc
                << "]: " << response;

            return;  // RETURN
        }

        ShutdownContextSp context;
        context.createInplace(
            d_state.d_allocator_p,
            bdlf::BindUtil::bind(&ClientSession::sendPacket,
                                 this,
                                 d_state.d_schemaEventBuilder.blob(),
                                 false));

        processStopRequest(context);
    }
    else {
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR " << description()
                       << ": unknown Cluster in StopResponse: " << message;
    }
}

void ClientSession::onDeconfiguredHandle(const ShutdownContextSp& contextSp)
{
    (void)contextSp;
}

void ClientSession::processStopRequest(ShutdownContextSp& contextSp)
{
    // This StopRequest arrives from a downstream (otherwise, ClusterProxy
    // would receive it).  As an upstream, this node needs to deconfigure all
    // queues and then respond with StopResponse.

    // Use the same logic as in the 'initiateShutdown' except that the final
    // step is sending StopResponse instead of 'closeChannel'
    // executed by the *CLIENT* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_operationState == e_DEAD) {
        // The client is disconnected.  No-op
        return;  // RETURN
    }
    if (d_operationState == e_SHUTTING_DOWN) {
        // The broker is already shutting down or processing a StopRequest
        // The de-configuring is done.
        // Even if the waiting is in progress, still reply with StopResponse
        return;  // RETURN
    }
    if (d_operationState == e_SHUTTING_DOWN_V2) {
        // The broker is already shutting down or processing a StopRequest
        // The de-configuring is done.
        // Even if the waiting is in progress, still reply with StopResponse
        return;  // RETURN
    }

    if (d_operationState == e_DISCONNECTING) {
        // The client is disconnecting.  No-op
        return;  // RETURN
    }
    for (QueueStateMapCIter cit = d_queueSessionManager.queues().begin();
         cit != d_queueSessionManager.queues().end();
         ++cit) {
        if (!cit->second.d_hasReceivedFinalCloseQueue) {
            cit->second.d_handle_p->deconfigureAll(
                bdlf::BindUtil::bind(&ClientSession::onDeconfiguredHandle,
                                     this,
                                     contextSp));
        }
    }
}

int ClientSession::dropAllQueueHandles(bool doDeconfigure, bool hasLostClient)
{
    int numHandlesDropped = 0;
    for (QueueStateMapIter it = d_queueSessionManager.queues().begin();
         it != d_queueSessionManager.queues().end();
         ++it) {
        if (!it->second.d_hasReceivedFinalCloseQueue) {
            it->second.d_handle_p->clearClient(hasLostClient);
            it->second.d_handle_p->drop(doDeconfigure);
            it->second.d_handle_p                   = 0;
            it->second.d_hasReceivedFinalCloseQueue = true;
            ++numHandlesDropped;
        }
    }

    return numHandlesDropped;
}

}  // close package namespace
}  // close enterprise namespace
