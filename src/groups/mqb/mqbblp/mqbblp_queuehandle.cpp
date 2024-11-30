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

// mqbblp_queuehandle.cpp                                             -*-C++-*-
#include <mqbblp_queuehandle.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcmd_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqc_orderedhashmap.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_logthrottle.h>
#include <bdlb_print.h>
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bsl_cmath.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_numeric.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const double k_WATERMARK_RATIO = 0.8;
// Percentage of the 'capacity' to use for the 'lowWatermark'

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND;
// Time interval between messages logged with throttling.

#define BMQ_LOGTHROTTLE_INFO()                                                \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

typedef bsl::function<void()> CompletionCallback;

/// Utility function used in `bmqu::OperationChain` as the operation
/// callback which just calls the completion callback.
void allSubstreamsDeconfigured(const CompletionCallback& callback)
{
    callback();
}

}  // close unnamed namespace

// ------------------------------------
// struct QueueHandle::SubStreamContext
// ------------------------------------

// CREATORS
QueueHandle::Subscription::Subscription(
    unsigned int                       subId,
    const bsl::shared_ptr<Downstream>& downstream,
    unsigned int                       upstreamId)

: d_unconfirmedMonitor(0, 0, 0, 0, 0, 0)  // Set later
, d_downstream(downstream)
, d_downstreamSubQueueId(subId)
, d_upstreamId(upstreamId)
{
    // NOTHING
}

const bsl::string& QueueHandle::Subscription::appId() const
{
    return d_downstream->d_appId;
}

// -----------------
// class QueueHandle
// -----------------

void QueueHandle::confirmMessageDispatched(const bmqt::MessageGUID& msgGUID,
                                           unsigned int downstreamSubQueueId)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !bmqt::QueueFlagsUtil::isReader(handleParameters().flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // NOTE: While we should never receive a CONFIRM for a queue which was
        //       not opened with READ flag, we can't assert that here because
        //       the SDK can not guarantee it at this time, and so it's
        //       possible that we receive a close queue (which will unset the
        //       READ parameters) before a CONFIRM event.  The SDK has some
        //       basic protection against this, however, if a client confirms a
        //       message and closes the queue from different thread, it could
        //       fool the check; and guarding against this would require making
        //       the entire SDK fully thread safe with mutex, which would come
        //       with a huge performance cost just to protect against this
        //       particular mis-usage.  Instead, we can just detect it here and
        //       emit a warning for the misbehaving client doing
        //       out-of-contract behavior.
        BALL_LOG_WARN
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Received a CONFIRM message from client '"
            << d_clientContext_sp->client()->description() << "' for queue '"
            << d_queue_sp->description()
            << "' which doesn't have the READ "
               "flag; this usually indicates that the client is confirming "
               "messages and closing the queue from different threads, and "
               "sending a confirm after having closed the queue.";
        // NOTE: This is fine to return here and not decrement the unconfirmed
        //       counters, because those have been reset and cleared when the
        //       read capacity was lost; we also don't need to inform the queue
        //       about that confirm and simply 'ignore' it.
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !validateDownstreamId(downstreamSubQueueId))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR "
                      << "Received a CONFIRM message from client '"
                      << d_clientContext_sp->client()->description()
                      << "' for queue '" << d_queue_sp->description()
                      << "' which doesn't have the subQueue: "
                      << "downstreamSubQueueId";
        return;
    }
    const bsl::shared_ptr<Downstream>& subStream = downstream(
        downstreamSubQueueId);
    unsigned int upstreamSubQueueId = subStream->d_upstreamSubQueueId;

    // If we previously hit the maxUnconfirmed and are now back to below the
    // lowWatermark for BOTH messages and bytes, then we will schedule a
    // delivery by indicating to the associated queue engine that this handle
    // is now back to being usable.  For this reason, we have the variable
    // 'resourceUsageStateChange' below, to capture such a transition if it
    // occurs.
    // Update unconfirmed messages list and stats

    updateMonitor(subStream, msgGUID, bmqp::EventType::e_CONFIRM);

    // Inform the queue about that confirm.
    // TBD: Consider doing these consistency checks at entry point (i.e. in
    // 'onConfirmEvent' in 'ClientSession' and 'Cluster')?
    d_queue_sp->confirmMessage(msgGUID, upstreamSubQueueId, this);
}

void QueueHandle::rejectMessageDispatched(const bmqt::MessageGUID& msgGUID,
                                          unsigned int downstreamSubQueueId)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !bmqt::QueueFlagsUtil::isReader(handleParameters().flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // NOTE: We should not be receiving a REJECT for a queue which was not
        //       opened with READ flag.
        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR "
                      << "Received a REJECT message from client '"
                      << d_clientContext_sp->client()->description()
                      << "' for queue '" << d_queue_sp->description()
                      << "' which doesn't have the READ "
                      << "flags";
        // NOTE: This is fine to return here and not do anything because there
        //       was supposed to be a rejection when the queue closed.
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !validateDownstreamId(downstreamSubQueueId))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR "
                      << "Received a REJECT message from client '"
                      << d_clientContext_sp->client()->description()
                      << "' for queue '" << d_queue_sp->description()
                      << "' which doesn't have the subQueue: "
                      << downstreamSubQueueId;
        return;
    }
    const bsl::shared_ptr<Downstream>& subStream = downstream(
        downstreamSubQueueId);

    // If we previously hit the maxUnconfirmed and are now back to below the
    // lowWatermark for BOTH messages and bytes, then we will schedule a
    // delivery by indicating to the associated queue engine that this handle
    // is now back to being usable.  For this reason, we have the variable
    // 'resourceUsageStateChange' below, to capture such a transition if it
    // occurs.
    // Update unconfirmed messages list and stats

    // Inform the queue about that reject.
    int counter = d_queue_sp->rejectMessage(msgGUID,
                                            subStream->d_upstreamSubQueueId,
                                            this);

    if (counter == 0) {
        updateMonitor(subStream, msgGUID, bmqp::EventType::e_REJECT);
        // That erases 'msgGUID' from 'd_unconfirmedMessages'
    }
}

mqbu::ResourceUsageMonitorStateTransition::Enum
QueueHandle::updateMonitor(const bsl::shared_ptr<Downstream>& subStream,
                           const bmqt::MessageGUID&           msgGUID,
                           bmqp::EventType::Enum              eventType)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    // TYPES

    /// Enum struct defining the type of state transition that a
    /// ResourceUsageMonitor may undergo.
    typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

    // Update unconfirmed messages list
    const mqbi::QueueHandle::RedeliverySp& messages = subStream->d_data;

    mqbi::QueueHandle::UnconfirmedMessageInfoMap::iterator it = messages->find(
        msgGUID);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it == messages->end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // The GUID was not found in the pending unconfirmed messages, one of
        // these happened (either rejection or confirmation):
        //
        // 1) the client already confirmed that message (a buggy consumer may
        //    be trying to confirm a message multiple times),
        // 2) the client confirmed an 'invalid' message (this could be either
        //    a corrupted GUID, a GUID obtained from a different
        //    'bmqa::Session', a saved GUID, ...),
        // 3) the client confirmed a valid GUID which was genuinely delivered
        //    to it, however the broker restarted (whether gracefully or
        //    ungracefully) between the message delivery and the confirmation
        //    from the client; therefore this QueueHandle was not aware of it.

        // For all those cases, we don't update the client's unconfirmed
        // messages stats, but we perform a validation with the storage and
        // update the domain stats if needed.  We also don't return but still
        // inform the queue to let it process that confirm (in the (3) case
        // when the broker restarted, this confirm is totally legit, and the
        // queue and associated storage will properly handle it; it will also
        // handle the case where the message no longer exists due to GC, TTL
        // expiration, admin deletion, ...).

        BSLS_ASSERT_SAFE(d_queue_sp->storage());

        int msgSize = -1;
        int rc      = d_queue_sp->storage()->getMessageSize(&msgSize, msgGUID);
        if (eventType == bmqp::EventType::e_CONFIRM && rc == 0) {
            // The message exist in the storage, we likely are in situation
            // (3), so it's a legit confirm and therefore, update the domain
            // stats.
            d_domainStats_p->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_CONFIRM,
                msgSize);
        }

        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "Received a " << eventType << " message for guid ["
                << msgGUID << "] from client '"
                << d_clientContext_sp->client()->description()
                << "' for queue '" << d_queue_sp->description()
                << "' for a message not in pending unconfirmed.";
            if (msgSize == -1) {
                BALL_LOG_OUTPUT_STREAM
                    << " Message was also not found in storage.";
            }
        }
        return RUMStateTransition::e_NO_CHANGE;  // RETURN
    }

    Subscriptions::iterator sit = d_subscriptions.find(
        it->second.d_subscriptionId);
    BSLS_ASSERT_SAFE(sit != d_subscriptions.end());

    const RUMStateTransition::Enum resourceUsageStateChange =
        updateMonitor(it, sit->second.get(), eventType, subStream->d_appId);
    messages->erase(it);

    // As mentioned above, if we hit the maxUnconfirmed and are now back to
    // below the lowWatermark for BOTH messages and bytes, schedule a delivery
    // by indicating to the associated queue engine that this handle is now
    // back to being usable.  Note that 'onHandleUsable()' (which invokes
    // 'deliverMessages()') is not a full cheap noop if there are no messages
    // pending delivery, but this is fine, this condition of going back to
    // normal should only be sporadically hit.
    //
    // For performance reasons, we don't want to send messages one by one as
    // they are confirmed, but rather let the receiver's buffer drain a bit and
    // then send a batch to full it up.  This is the raison-d'etre of the
    // k_WATERMARK_RATIO.  A simple scenario to explain why this is needed:
    // Let's consider there are no consumers of a queue, and a producer puts
    // 1000 messages into it.  Then, a consumer opens it with a
    // maxUnackedMessages of 10.  The queueHandle will deliver 10 messages, and
    // then wait for a message to be confirmed.  Without this ratio logic, we
    // would then deliver messages one by one as messages are being confirmed,
    // and those delivered messages will anyway sit in the receiver's buffer
    // for a little bit.  It is therefore better to let the buffer drain, and
    // then deliver again in a batch.  However, with clusterProxy, we can't use
    // a low ratio (such as 20%), because this could potentially starve a
    // consumer if there are multiple consumers under the proxy, with very
    // different maxUnconfirmed rates.  In practice though, because clients
    // should mostly open the queue with a decently big value, even a high
    // ratio should still provide a decent batching experience.

    if (resourceUsageStateChange == RUMStateTransition::e_LOW_WATERMARK) {
        BSLS_ASSERT_SAFE(d_queue_sp->queueEngine() &&
                         "Queue has no engine associated !");
        d_queue_sp->queueEngine()->onHandleUsable(
            this,
            sit->second.get()->d_upstreamId);
    }

    return resourceUsageStateChange;
}

mqbu::ResourceUsageMonitorStateTransition::Enum QueueHandle::updateMonitor(
    mqbi::QueueHandle::UnconfirmedMessageInfoMap::iterator it,
    Subscription*                                          subscription,
    bmqp::EventType::Enum                                  type,
    const bsl::string&                                     appId)
{
    const unsigned int msgSize = it->second.d_size;
    // NOTE: if we don't convert msgSize to Int64 and instead below just
    //       call d_unconfirmedMonitor.update(-msgSize, -1), then we get an
    //       overflow and the update actually increments by a very large
    //       value

    const bsls::Types::Int64 timeDelta = bmqsys::Time::highResolutionTimer() -
                                         it->second.d_timeStamp;
    BSLS_ASSERT_SAFE(timeDelta >= 0);

    mqbu::ResourceUsageMonitorStateTransition::Enum resourceUsageStateChange;

    if (subscription) {
        BSLS_ASSERT_SAFE(subscription->d_downstream);
        BSLS_ASSERT_SAFE(subscription->d_downstream->d_data);
        BSLS_ASSERT_SAFE(it != subscription->d_downstream->d_data->end());
        // Update resource usage and record usage state change (if any)
        bsls::Types::Int64 bytesDelta = msgSize;
        bytesDelta *= -1;

        resourceUsageStateChange =
            subscription->d_unconfirmedMonitor.update(bytesDelta, -1);
        BSLS_ASSERT_SAFE(subscription->d_unconfirmedMonitor.messages() >= 0 &&
                         subscription->d_unconfirmedMonitor.bytes() >= 0);
    }
    else {
        // The subscription is gone (by ConfigureStream)
        resourceUsageStateChange =
            mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE;
    }

    if (type == bmqp::EventType::e_CONFIRM) {
        // Update domain stats
        d_domainStats_p->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_CONFIRM,
            msgSize);

        // Report CONFIRM time only at first hop
        // Note that we update metric per entire queue and also per `appId`
        if (d_clientContext_sp->isFirstHop()) {
            d_domainStats_p->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_CONFIRM_TIME,
                timeDelta);
            d_domainStats_p->onEvent(
                mqbstat::QueueStatsDomain::EventType::e_CONFIRM_TIME,
                timeDelta,
                appId);
        }
    }

    return resourceUsageStateChange;
}

void QueueHandle::clearClientDispatched(bool hasLostClient)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    d_clientContext_sp.reset();

    if (!hasLostClient) {
        return;  // RETURN
    }

    // Iterate all unconfirmed messages and issue Reject for not-unlimited ones
    for (Downstreams::iterator itContext = d_downstreams.begin();
         itContext != d_downstreams.end();
         ++itContext) {
        bsl::shared_ptr<Downstream>& downstream = (*itContext);
        if (!downstream) {
            continue;  // CONTINUE
        }
        const mqbi::QueueHandle::RedeliverySp& messages = downstream->d_data;

        BALL_LOG_INFO << "Queue '" << d_queue_sp->description()
                      << "' rejecting " << messages->size()
                      << " messages because of a client crash.";

        for (mqbi::QueueHandle::UnconfirmedMessageInfoMap::iterator it =
                 messages->begin();
             it != messages->end();) {
            int counter = d_queue_sp->rejectMessage(
                it->first,
                (*itContext)->d_upstreamSubQueueId,
                this);

            if (counter == 0) {
                updateMonitor(
                    it,
                    d_subscriptions[it->second.d_subscriptionId].get(),
                    bmqp::EventType::e_REJECT,
                    downstream->d_appId);

                // Do not call 'onHandleUsable' because 'd_clientContext_sp' is
                // reset

                it = messages->erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void QueueHandle::deliverMessageImpl(
    const bsl::shared_ptr<bdlbb::Blob>&       message,
    const int                                 msgSize,
    const bmqt::MessageGUID&                  msgGUID,
    const mqbi::StorageMessageAttributes&     attributes,
    const bmqp::Protocol::MsgGroupId&         msgGroupId,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    bool                                      isOutOfOrder)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(
        bmqt::QueueFlagsUtil::isReader(handleParameters().flags()));
    BSLS_ASSERT_SAFE(subQueueInfos.size() >= 1 &&
                     subQueueInfos.size() <= d_subscriptions.size());

    d_domainStats_p->onEvent(mqbstat::QueueStatsDomain::EventType::e_PUSH,
                             msgSize);

    // Create an event to dispatch delivery of the message to the client
    mqbi::DispatcherClient* client = d_clientContext_sp->client();
    mqbi::DispatcherEvent*  event  = client->dispatcher()->getEvent(client);
    (*event)
        .setType(mqbi::DispatcherEventType::e_PUSH)
        .setSource(d_queue_sp.get())
        .setGuid(msgGUID)
        .setQueueId(id())
        .setMessagePropertiesInfo(d_queue_sp->schemaLearner().demultiplex(
            d_schemaLearnerPushContext,
            attributes.messagePropertiesInfo()))
        .setSubQueueInfos(subQueueInfos)
        .setMsgGroupId(msgGroupId)
        .setCompressionAlgorithmType(attributes.compressionAlgorithmType())
        .setOutOfOrderPush(isOutOfOrder);

    if (message) {
        event->setBlob(message);
    }

    client->dispatcher()->dispatchEvent(event, client);
}

QueueHandle::QueueHandle(
    const bsl::shared_ptr<mqbi::Queue>&                       queueSp,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    mqbstat::QueueStatsDomain*                                domainStats,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    bslma::Allocator*                                         allocator)
: d_queue_sp(queueSp)
, d_clientContext_sp(clientContext)
, d_domainStats_p(domainStats)
, d_handleParameters(allocator)  // call setHandleParameters to also do the
                                 // stats logic
, d_subscriptions(allocator)
, d_subStreamInfos(allocator)
, d_downstreams(allocator)
, d_isClientClusterMember(false)
, d_deconfigureChain(allocator)
, d_schemaLearnerPutContext(
      d_queue_sp ? d_queue_sp->schemaLearner().createContext() : 0)
, d_schemaLearnerPushContext(
      d_queue_sp ? d_queue_sp->schemaLearner().createContext() : 0)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queue_sp);
    BSLS_ASSERT_SAFE(d_clientContext_sp->client());

    d_throttledFailedAckMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    d_throttledDroppedPutMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 5 seconds

    setHandleParameters(handleParameters);
}

QueueHandle::~QueueHandle()
{
    // Disabling this assert as self node could be deleting a queue handle
    // which may have non-zero read/write/admin counts.  This could occur if
    // self's view of the downstream client has differed from what the client
    // itself thinks.  Client may sent a closeQueue request with 'isFinal' set
    // to true, in which case self node may end up force-deleting the handle,
    // and following assert will fire.  We may want to re-enable this assert
    // once the code base is mature enough.

    // PRECONDITIONS
    // BSLS_ASSERT_SAFE(   d_handleParameters.readCount()  == 0
    //                  && d_handleParameters.writeCount() == 0
    //                  && d_handleParameters.adminCount() == 0);
    // All resources should have been released prior to destruction of the
    // QueueHandle.
    d_deconfigureChain.stop();
    d_deconfigureChain.join();
}

void QueueHandle::registerSubStream(const bmqp_ctrlmsg::SubQueueIdInfo& stream,
                                    unsigned int upstreamSubQueueId,
                                    const mqbi::QueueCounts& counts)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(counts.d_readCount > 0 || counts.d_writeCount > 0);

    SubStreams::iterator infoIter = d_subStreamInfos.find(stream.appId());

    if (infoIter != d_subStreamInfos.end()) {
        // SubStream has already been registered
        BSLS_ASSERT_SAFE(infoIter->second.d_downstreamSubQueueId ==
                         stream.subId());

        infoIter->second.d_counts += counts;
        // 'subId' must uniquely identify 'appId'.
        // Update the 'upstreamSubQueueId'.  The previously registered stream
        // could be a producer and the new one - consumer with a new id.
        downstream(stream.subId())->d_upstreamSubQueueId = upstreamSubQueueId;
        return;  // RETURN
    }
    // Allocate spot

    BSLS_ASSERT_SAFE(!validateDownstreamId(stream.subId()));

    makeSubStream(stream.appId(), stream.subId(), upstreamSubQueueId);

    BALL_LOG_INFO << "QueueHandle [" << this << "] registering subQueue ["
                  << stream << "] with upstreamSubQueueId ["
                  << upstreamSubQueueId << "]";

    d_subStreamInfos.emplace(
        stream.appId(),
        StreamInfo(counts, stream.subId(), upstreamSubQueueId, d_allocator_p));
}

void QueueHandle::registerSubscription(unsigned int downstreamSubId,
                                       unsigned int downstreamId,
                                       const bmqp_ctrlmsg::ConsumerInfo& ci,
                                       unsigned int upstreamId)
{
    BMQ_LOGTHROTTLE_INFO() << "QueueHandle [" << this
                           << "] registering Subscription [id = "
                           << downstreamId
                           << ", downstreamSubQueueId = " << downstreamSubId
                           << ", upstreamId = " << upstreamId << "]";

    const bsl::shared_ptr<Downstream>& subStream = downstream(downstreamSubId);

    Subscriptions::iterator itSubscription = d_subscriptions.find(
        downstreamId);
    SubscriptionSp subscription;

    if (itSubscription == d_subscriptions.end()) {
        subscription.reset(
            new (*d_allocator_p)
                Subscription(downstreamSubId, subStream, upstreamId),
            d_allocator_p);
        d_subscriptions.emplace(downstreamId, subscription);
    }
    else {
        subscription = itSubscription->second;
        BSLS_ASSERT_SAFE(subscription->d_downstreamSubQueueId ==
                         downstreamSubId);

        // Upstream subscription Id can change as in the case when multiple
        // relay configure requests result in generating new ids.
        subscription->d_upstreamId = upstreamId;
    }

    // Ceil the limits values, so that if max redeliveries is 1, it will
    // compute ok
    const bsls::Types::Int64 lowWatermarkBytes =
        static_cast<bsls::Types::Int64>(
            bsl::ceil(ci.maxUnconfirmedBytes() * k_WATERMARK_RATIO));

    // We only care about whether we are at or above the `capacity`
    // bytes or at or below the `lowWatermark` bytes.
    const bsls::Types::Int64 highWatermarkBytes = lowWatermarkBytes;

    const bsls::Types::Int64 capacityBytes        = ci.maxUnconfirmedBytes();
    const bsls::Types::Int64 lowWatermarkMessages = bsl::ceil(
        ci.maxUnconfirmedMessages() * k_WATERMARK_RATIO);

    // We only care about whether we are at or above the `capacity`
    // messages or at or below the `lowWatermark` redeliveries.
    const bsls::Types::Int64 highWatermarkMessages = lowWatermarkMessages;
    const bsls::Types::Int64 capacityMessages = ci.maxUnconfirmedMessages();

    // After reconfiguring the limits of the monitor below, its state may
    // or may not be the same as its current state.  For example, if the
    // monitor is now in the 'full' state (reached the limits on
    // unconfirmed bytes and/or redeliveries) and a new reader comes in, the
    // messages limit should be incremented, which would potentially bring
    // down the state from full to high watermark; but however, since bytes
    // are not incremented by one unit increment (and the limits are more
    // soft than hard limit, allowing to go beyond them), we may already
    // have a value beyond capacity and the new reader will not add enough
    // capacity to lower the state to high watermark.  The implication is
    // that if we remain in the 'full' state, the new reader may not
    // receive any messages until enough messages are confirmed to put our
    // resource usage (both bytes and redeliveries) at or below 'lowWatermark'.
    // Fortunately, this is not the common case, so we expect that most
    // re-configurations that involve a new reader will result in a monitor
    // that is NOT in the 'full' state.
    subscription->d_unconfirmedMonitor.reconfigure(capacityBytes,
                                                   capacityMessages,
                                                   lowWatermarkBytes,
                                                   highWatermarkBytes,
                                                   lowWatermarkMessages,
                                                   highWatermarkMessages);
}

bool QueueHandle::unregisterSubStream(
    const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo,
    const mqbi::QueueCounts&            counts,
    bool                                isFinal)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    SubStreams::iterator infoIter = d_subStreamInfos.find(
        subStreamInfo.appId());

    BSLS_ASSERT_SAFE(infoIter != d_subStreamInfos.end());

    infoIter->second.d_counts -= counts;

    if ((0 == infoIter->second.d_counts.d_readCount &&
         0 == infoIter->second.d_counts.d_writeCount) ||
        isFinal) {
        unsigned int downstreamSubQueueId = subStreamInfo.subId();
        BSLS_ASSERT_SAFE(validateDownstreamId(downstreamSubQueueId));

        BALL_LOG_INFO
            << "QueueHandle [" << this << "] unregistering subQueue ["
            << subStreamInfo << "] with upstreamSubQueueId ["
            << d_downstreams[downstreamSubQueueId]->d_upstreamSubQueueId
            << "]";

        for (Subscriptions::iterator itSubscription = d_subscriptions.begin();
             itSubscription != d_subscriptions.end();) {
            const SubscriptionSp& subscription = itSubscription->second;
            if (subscription->d_downstreamSubQueueId == downstreamSubQueueId) {
                BMQ_LOGTHROTTLE_INFO()
                    << "Queue '" << d_queue_sp->description() << "' handle "
                    << this << " removing Subscription "
                    << itSubscription->first;
                itSubscription = d_subscriptions.erase(itSubscription);
            }
            else {
                ++itSubscription;
            }
        }
        d_downstreams[downstreamSubQueueId].reset();
        d_subStreamInfos.erase(infoIter);
        return true;  // RETURN
    }
    return false;
}

void QueueHandle::confirmMessage(const bmqt::MessageGUID& msgGUID,
                                 unsigned int             downstreamSubQueueId)
{
    // executed by *ANY* thread

    // NOTE: We don't verify here that the queue was opened with 'e_READ' flag,
    //       refer to 'confirmMessageDispatched'.

    // Enqueue an event to process the confirm on the queue thread

    // REVISIT: THis is based on the assumption that `bslstl::function` will
    // NOT allocate memory (its sizeof(InplaceBuffer) is 48).  Otherwise, this
    // becomes performance bottleneck.

    // A more generic approach would be to maintain a queue of CONFIRMs per
    // queue (outside of the dispatcher) and process it separately (on idle?).

    mqbi::DispatcherEvent* queueEvent = d_queue_sp->dispatcher()->getEvent(
        mqbi::DispatcherClientType::e_QUEUE);

    (*queueEvent).setType(mqbi::DispatcherEventType::e_CALLBACK);

    new (queueEvent->callback().place<QueueHandle::ConfirmFunctor>())
        QueueHandle::ConfirmFunctor(this, msgGUID, downstreamSubQueueId);

    d_queue_sp->dispatcher()->dispatchEvent(queueEvent, d_queue_sp.get());
}

void QueueHandle::rejectMessage(const bmqt::MessageGUID& msgGUID,
                                unsigned int             downstreamSubQueueId)
{
    // executed by *ANY* thread

    // Enqueue an event to process the confirm on the queue thread
    d_queue_sp->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueHandle::rejectMessageDispatched,
                             this,
                             msgGUID,
                             downstreamSubQueueId),
        d_queue_sp.get());
}

void QueueHandle::deliverMessageNoTrack(
    const bsl::shared_ptr<bdlbb::Blob>&       message,
    const bmqt::MessageGUID&                  msgGUID,
    const mqbi::StorageMessageAttributes&     attributes,
    const bmqp::Protocol::MsgGroupId&         msgGroupId,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(
        bmqt::QueueFlagsUtil::isReader(handleParameters().flags()));

    deliverMessageImpl(message,
                       message->length(),
                       msgGUID,
                       attributes,
                       msgGroupId,
                       subQueueInfos,
                       false);
}

void QueueHandle::deliverMessage(
    const bsl::shared_ptr<bdlbb::Blob>&       message,
    const bmqt::MessageGUID&                  msgGUID,
    const mqbi::StorageMessageAttributes&     attributes,
    const bmqp::Protocol::MsgGroupId&         msgGroupId,
    const bmqp::Protocol::SubQueueInfosArray& subscriptions,
    bool                                      isOutOfOrder)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(
        bmqt::QueueFlagsUtil::isReader(handleParameters().flags()));

    const int                          msgSize = message->length();
    bmqp::Protocol::SubQueueInfosArray targetSubscriptions;
    bsls::Types::Int64 now = bmqsys::Time::highResolutionTimer();
    for (size_t i = 0; i < subscriptions.size(); ++i) {
        unsigned int          subscriptionId = subscriptions[i].id();
        const SubscriptionSp& subscription   = d_subscriptions[subscriptionId];

        BSLS_ASSERT_SAFE(canDeliver(subscriptionId));
        BSLS_ASSERT_SAFE(subscription);

        mqbi::QueueHandle::RedeliverySp& messages =
            subscription->d_downstream->d_data;

        // Add the message to the unconfirmed messages list

        bsl::pair<mqbi::QueueHandle::UnconfirmedMessageInfoMap::iterator, bool>
            rc = messages->insert(bsl::make_pair(
                msgGUID,
                mqbi::UnconfirmedMessageInfo(msgSize, now, subscriptionId)));

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc.second == false)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // This message was already delivered to this specific client.
            // This could happen in case of primary switch, where the new
            // primary will redistribute outstanding messages.  If (by luck)
            // primary ends up redelivering the message to the task to which it
            // initially was delivered to (that is exactly what is happening
            // here), then instead of redelivering the message with a hint of
            // potential duplicate, we can just skip delivery all together.

            // Pretend the message was successfully delivered; for now this is
            // fine because none of the call path to deliverMessage are doing
            // any sort of statistics increment or so; but at some point, if
            // needed we way have to change this method to return a tri-value
            // (delivered, non-delivered, skipped).
            if (d_throttledDroppedPutMessages.requestPermission()) {
                BALL_LOG_INFO << "Queue '" << d_queue_sp->description()
                              << "' skipping duplicate PUSH " << msgGUID;
            }
            continue;  // CONTINUE
        }

        targetSubscriptions.push_back(subscriptions[i]);

        // NOTE: Updating the unconfirmedMonitors may cause the state of the
        //       monitor to change to 'STATE_FULL' and thus may impact the
        //       value returned by 'canDeliver'.
        subscription->d_unconfirmedMonitor.update(msgSize, 1);

        // NOTE: To simplify the logic, we always send at least one message
        //       (from the loop in the callers of this method), even if that
        //       would take us beyond the maxUnconfirmed number of outstanding
        //       bytes.  This is to prevent deadlock situation where a client
        //       would be receiving a message bigger than it's
        //       maxUnconfirmedBytes.

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                subscription->d_unconfirmedMonitor.state() ==
                    mqbu::ResourceUsageMonitorState::e_STATE_FULL &&
                subscription->d_unconfirmedMonitor.messageCapacity() > 1)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // We do NOT log when 'maxUnconfirmedMessages == 1' because this is
            // a 'legitimate' special case that consumers willing to have more
            // fine grained control over message distribution load may use
            // (especially in the case of long to process messages).

            // Increasing resource usage ('update()' above) made us hit our
            // maxUnconfirmed limit.
            BMQ_LOGTHROTTLE_INFO()
                << "Queue '" << d_queue_sp->description()
                << "' with subscription [" << subscriptions[i] << "]"
                << " of client '"
                << d_clientContext_sp->client()->description()
                << "' has too many outstanding data ["
                << bmqu::PrintUtil::prettyNumber(
                       subscription->d_unconfirmedMonitor.messages())
                << " msgs (max: "
                << bmqu::PrintUtil::prettyNumber(
                       subscription->d_unconfirmedMonitor.messageCapacity())
                << "), "
                << bmqu::PrintUtil::prettyBytes(
                       subscription->d_unconfirmedMonitor.bytes())
                << " (max: "
                << bmqu::PrintUtil::prettyBytes(
                       subscription->d_unconfirmedMonitor.byteCapacity())
                << ")], I'm taking a break!";
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(targetSubscriptions.empty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    deliverMessageImpl(isClientClusterMember() ? bsl::shared_ptr<bdlbb::Blob>()
                                               : message,
                       msgSize,
                       msgGUID,
                       attributes,
                       msgGroupId,
                       targetSubscriptions,
                       isOutOfOrder);
}

void QueueHandle::postMessage(const bmqp::PutHeader&              putHeader,
                              const bsl::shared_ptr<bdlbb::Blob>& appData,
                              const bsl::shared_ptr<bdlbb::Blob>& options)
{
    // executed by the *CLUSTER_DISPATCHER* or *CLIENT_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clientContext_sp->client()->dispatcher()->inDispatcherThread(
            d_clientContext_sp->client()));

    // cannot check 'd_subscriptions' unless in the QUEUE dispatcher thread

    mqbi::DispatcherEvent* event = d_queue_sp->dispatcher()->getEvent(
        d_queue_sp.get());

    (*event)
        .setType(mqbi::DispatcherEventType::e_PUT)
        .setSource(d_clientContext_sp->client())
        .setBlob(appData)
        .setOptions(options)
        .setPutHeader(putHeader)
        .setQueueHandle(this);

    d_queue_sp->dispatcher()->dispatchEvent(event, d_queue_sp.get());
}

void QueueHandle::configure(
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by *ANY* thread

    d_queue_sp->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueHandle::configureDispatched,
                             this,
                             streamParameters,
                             configuredCb),
        d_queue_sp.get());
}

void QueueHandle::configureDispatched(
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(d_clientContext_sp);

    BALL_LOG_INFO << "Client [" << d_clientContext_sp->client()->description()
                  << "] requested to configure queue [" << d_queue_sp->uri()
                  << ", id: " << id()
                  << ", appId: " << streamParameters.appId()
                  << "] with streamParameters: " << streamParameters;

    d_queue_sp->configureHandle(this, streamParameters, configuredCb);
}

void QueueHandle::deconfigureAll(
    const mqbi::QueueHandle::VoidFunctor& deconfiguredCb)
{
    // executed by *ANY* thread

    d_queue_sp->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueHandle::deconfigureDispatched,
                             this,
                             deconfiguredCb),
        d_queue_sp.get(),
        mqbi::DispatcherEventType::e_DISPATCHER);

    // Use 'mqbi::DispatcherEventType::e_DISPATCHER' to avoid (re)enabling
    // 'd_flushList'
}

void QueueHandle::deconfigureDispatched(
    const mqbi::QueueHandle::VoidFunctor& deconfiguredCb)
{
    // executed by *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(d_clientContext_sp);

    // Fill the first link with deconfigure operations
    bmqu::OperationChainLink link(d_deconfigureChain.allocator());

    mqbi::QueueHandle::SubStreams::const_iterator citer =
        d_subStreamInfos.begin();
    for (; citer != d_subStreamInfos.end(); ++citer) {
        bmqp_ctrlmsg::StreamParameters nullStreamParameters;
        nullStreamParameters.appId() = citer->first;

        link.insert(bdlf::BindUtil::bind(&QueueHandle::configureDispatched,
                                         this,
                                         nullStreamParameters,
                                         bdlf::PlaceHolders::_1));
    }
    // No-op if the link is empty
    d_deconfigureChain.append(&link);

    // Add the final operation that invokes the caller callback once all the
    // substreams are deconfigured
    d_deconfigureChain.appendInplace(
        bdlf::BindUtil::bind(&allSubstreamsDeconfigured,
                             bdlf::PlaceHolders::_1),
        deconfiguredCb);

    // execute operations in the chain
    d_deconfigureChain.start();
}

void QueueHandle::release(
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by *ANY* thread

    d_queue_sp->releaseHandle(this, handleParameters, isFinal, releasedCb);
    // Above call may delete 'this'.
}

void QueueHandle::drop(bool doDeconfigure)
{
    // executed by *ANY* thread

    d_queue_sp->dropHandle(this, doDeconfigure);
    // Above call may delete 'this'.
}

void QueueHandle::clearClient(bool hasLostClient)
{
    // executed by the *CLIENT_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_clientContext_sp->client()->dispatcher()->inDispatcherThread(
            d_clientContext_sp->client()));

    // Enqueue an event to process this event in the queue thread.

    d_queue_sp->dispatcher()->execute(
        bdlf::BindUtil::bind(&QueueHandle::clearClientDispatched,
                             this,
                             hasLostClient),
        d_queue_sp.get());
}

int QueueHandle::transferUnconfirmedMessageGUID(
    const mqbi::RedeliveryVisitor& out,
    unsigned int                   subQueueId)
{
    // TEMPORARILY: transfer/redeliver entire subStream (vs. Subscription)
    const bsl::shared_ptr<Downstream>& subStream = downstream(subQueueId);
    mqbi::QueueHandle::RedeliverySp&   data      = subStream->d_data;

    BSLS_ASSERT_SAFE(data);
    int result = data->size();

    if (out) {
        for (mqbi::QueueHandle::UnconfirmedMessageInfoMap::const_iterator it =
                 data->begin();
             it != data->end();
             ++it) {
            out(it->first);
        }
    }

    // Reset unconfirmed messages and associated state
    data.reset(new (*d_allocator_p)
                   mqbi::QueueHandle::UnconfirmedMessageInfoMap(d_allocator_p),
               d_allocator_p);

    // Reset unconfirmed messages and associated state

    for (Subscriptions::iterator itSubscription = d_subscriptions.begin();
         itSubscription != d_subscriptions.end();
         ++itSubscription) {
        const SubscriptionSp& subscription = itSubscription->second;
        if (subscription->d_downstreamSubQueueId == subQueueId) {
            subscription->d_unconfirmedMonitor.reset();
        }
    }

    return result;
}

mqbi::QueueHandle* QueueHandle::setHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));
    BSLS_ASSERT_SAFE(handleParameters.uri() ==
                     bmqt::Uri(handleParameters.uri()).canonical());
    // Should only set handleParameters having a canonical URI because it
    // will be used across all the subStreams of this handle.

    d_handleParameters = handleParameters;

    return this;
}

mqbi::QueueHandle* QueueHandle::setStreamParameters(
    const bmqp_ctrlmsg::StreamParameters& streamParameters)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    // Merge all Subscriptions into 'd_subscriptions'.
    // Ignore all priorities but the highest.
    const bsl::string&   appId    = streamParameters.appId();
    SubStreams::iterator infoIter = d_subStreamInfos.find(appId);

    BSLS_ASSERT_SAFE(infoIter != d_subStreamInfos.end());

    // Set the streamParameters
    infoIter->second.d_streamParameters = streamParameters;

    return this;
}

void QueueHandle::onAckMessage(const bmqp::AckMessage& ackMessage)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    // NOTE: ACK comes from upstream, and client may have gone away, so we
    //       check for it here.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_clientContext_sp)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (d_throttledFailedAckMessages.requestPermission()) {
            BALL_LOG_INFO << "#CLIENT_ACK_FAILURE: Received an ACK from "
                          << "upstream for unavailable client "
                          << "[queue: " << d_queue_sp->description()
                          << ", upstream queueId: " << ackMessage.queueId()
                          << ", downstream queueId: " << id()
                          << ", guid: " << ackMessage.messageGUID()
                          << ", status: " << ackMessage.status()
                          << ", correlationId: " << ackMessage.correlationId()
                          << "]";
        }
        return;  // RETURN
    }

    // Do not check if 'd_subscriptions' still have k_DEFAULT_SUBQUEUE_ID
    // as the producer may be gone.

    // Queue invoked this method on the queue handle.  This implies that Ack
    // was requested or queue failed to post the message or both.  Also note
    // that we need to reset the queueId in ack message to the one which is
    // known downstream.
    mqbi::DispatcherClient* client = d_clientContext_sp->client();
    mqbi::DispatcherEvent*  event  = client->dispatcher()->getEvent(client);
    (*event)
        .setType(mqbi::DispatcherEventType::e_ACK)
        .setSource(d_queue_sp.get())
        .setAckMessage(ackMessage);

    // Override with correct downstream queueId
    const mqbi::DispatcherAckEvent* ackEvent = event->asAckEvent();

    bmqp::AckMessage& ackMsg = const_cast<bmqp::AckMessage&>(
        ackEvent->ackMessage());
    ackMsg.setQueueId(id());

    client->dispatcher()->dispatchEvent(event, client);
    d_domainStats_p->onEvent(mqbstat::QueueStatsDomain::EventType::e_ACK, 1);
}

bool QueueHandle::canDeliver(unsigned int downstreamSubscriptionId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    Subscriptions::const_iterator cit = d_subscriptions.find(
        downstreamSubscriptionId);

    BSLS_ASSERT_SAFE(cit != d_subscriptions.end());

    return d_clientContext_sp &&
           (cit->second->d_unconfirmedMonitor.state() !=
            mqbu::ResourceUsageMonitorState::e_STATE_FULL);
}

const bsl::vector<const mqbu::ResourceUsageMonitor*>
QueueHandle::unconfirmedMonitors(const bsl::string& appId) const
{
    bsl::vector<const mqbu::ResourceUsageMonitor*> out(d_allocator_p);

    SubStreams::const_iterator infoCiter = d_subStreamInfos.find(appId);

    if (infoCiter != d_subStreamInfos.end()) {
        for (Subscriptions::const_iterator cit = d_subscriptions.begin();
             cit != d_subscriptions.end();
             ++cit) {
            if (cit->second->appId() == appId) {
                out.push_back(&cit->second->d_unconfirmedMonitor);
            }
        }
    }

    return out;
}

bsls::Types::Int64 QueueHandle::countUnconfirmed(unsigned int subQueueId) const
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    bsls::Types::Int64 result = 0;
    if (subQueueId == bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) {
        for (Downstreams::const_iterator itStream = d_downstreams.begin();
             itStream != d_downstreams.end();
             ++itStream) {
            const bsl::shared_ptr<Downstream>& downstream = (*itStream);
            if (downstream) {
                if (downstream->d_data) {
                    result += downstream->d_data->size();
                }
            }
        }
    }
    else if (validateDownstreamId(subQueueId)) {
        if (d_downstreams[subQueueId]->d_data) {
            result += d_downstreams[subQueueId]->d_data->size();
        }
    }
    return result;
}

void QueueHandle::loadInternals(mqbcmd::QueueHandle* out) const
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        d_queue_sp->dispatcher()->inDispatcherThread(d_queue_sp.get()));

    bmqu::MemOutStream os;
    os << d_handleParameters;
    out->parametersJson()        = os.str();
    out->isClientClusterMember() = d_isClientClusterMember;

    bsl::vector<mqbcmd::QueueHandleSubStream>& subStreams = out->subStreams();
    subStreams.reserve(d_subStreamInfos.size());
    for (SubStreams::const_iterator infoCiter = d_subStreamInfos.begin();
         infoCiter != d_subStreamInfos.end();
         ++infoCiter) {
        subStreams.resize(subStreams.size() + 1);
        mqbcmd::QueueHandleSubStream& subStream = subStreams.back();
        unsigned int       subId = infoCiter->second.d_downstreamSubQueueId;
        const bsl::string& appId = infoCiter->first;
        bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo;

        subStreamInfo.appId() = appId;
        subStreamInfo.subId() = subId;

        subStream.subId() = subId;
        subStream.appId().makeValue(appId);

        os.reset();
        os << infoCiter->second.d_streamParameters;
        subStream.parametersJson() = os.str();

        if (!bmqt::QueueFlagsUtil::isReader(handleParameters().flags())) {
            continue;  // CONTINUE
        }

        mqbi::QueueHandle::RedeliverySp& data = downstream(subId)->d_data;
        BSLS_ASSERT_SAFE(data);

        subStream.numUnconfirmedMessages().makeValue(data->size());

        bsl::vector<mqbcmd::ResourceUsageMonitor>& monitors =
            subStream.unconfirmedMonitors();

        for (Subscriptions::const_iterator citSubscription =
                 d_subscriptions.begin();
             citSubscription != d_subscriptions.end();
             ++citSubscription) {
            if (citSubscription->second->d_downstreamSubQueueId != subId) {
                continue;  // CONTINUE
            }
            monitors.resize(monitors.size() + 1);
            mqbcmd::ResourceUsageMonitor& unconfirmedMonitorResult =
                monitors.back();
            mqbu::ResourceUsageMonitor& unconfirmedMonitor =
                citSubscription->second->d_unconfirmedMonitor;

            // TODO: Add assert here to make sure state exists
            mqbcmd::ResourceUsageMonitorState::fromInt(
                &unconfirmedMonitorResult.state(),
                unconfirmedMonitor.state());
            mqbcmd::ResourceUsageMonitorState::fromInt(
                &unconfirmedMonitorResult.messagesState(),
                unconfirmedMonitor.messageState());
            mqbcmd::ResourceUsageMonitorState::fromInt(
                &unconfirmedMonitorResult.bytesState(),
                unconfirmedMonitor.byteState());

            unconfirmedMonitorResult.numMessages() =
                unconfirmedMonitor.messages();
            unconfirmedMonitorResult.messagesLowWatermarkRatio() =
                unconfirmedMonitor.messageLowWatermarkRatio();
            unconfirmedMonitorResult.messagesHighWatermarkRatio() =
                unconfirmedMonitor.messageHighWatermarkRatio();
            unconfirmedMonitorResult.messagesCapacity() =
                unconfirmedMonitor.messageCapacity();

            unconfirmedMonitorResult.numBytes() = unconfirmedMonitor.bytes();
            unconfirmedMonitorResult.bytesLowWatermarkRatio() =
                unconfirmedMonitor.byteLowWatermarkRatio();
            unconfirmedMonitorResult.bytesHighWatermarkRatio() =
                unconfirmedMonitor.byteHighWatermarkRatio();
            unconfirmedMonitorResult.bytesCapacity() =
                unconfirmedMonitor.byteCapacity();
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
