// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbblp_queueengineutil.cpp                                         -*-C++-*-
#include <mqbblp_queueengineutil.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// MQB
#include <mqbblp_queuestate.h>
#include <mqbblp_routers.h>
#include <mqbcfg_brokerconfig.h>
#include <mqbcmd_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbi_queueengine.h>
#include <mqbs_storageutil.h>
#include <mqbstat_queuestats.h>

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_blob.h>
#include <mwcu_printutil.h>
#include <mwcu_temputil.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdls_filesystemutil.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>

// SYS
#include <sys/stat.h>

namespace BloombergLP {
namespace mqbblp {
namespace {

const bsls::Types::Int64 k_MAX_SECONDS =
    bsl::numeric_limits<bsls::Types::Int64>::max();

const int k_MAX_NANOSECONDS = 999999999;

/// Dummy method enqueued to the associated client's dispatcher thread when
/// the specified `handle` was dropped and deleted without providing a
/// `releasedCb`, in order to delay its destruction until after the client's
/// dispatcher thread queue has been drained.
void queueHandleHolderDummy(const bsl::shared_ptr<mqbi::QueueHandle>& handle)
{
    BALL_LOG_SET_CATEGORY("MQBBLP.PRIORITYQUEUEENGINE");
    BALL_LOG_TRACE << "queueHandleHolderDummy of '" << handle->queue()->uri()
                   << "'";
}

/// Method to release the specified `handle` before executing function `fn`.
void releaseHandleAndInvoke(bdlmt::EventSchedulerEventHandle* handle,
                            bsls::AtomicBool*                 isScheduled,
                            const bsl::function<void()>&      fn)
{
    handle->release();
    if (isScheduled->load()) {
        isScheduled->store(false);
        fn();
    }
}

/// Callback to use in `QueueEngineUtil_AppState::tryDeliverOneMessage`
struct Visitor {
    mqbi::QueueHandle* d_handle;
    unsigned int       d_downstreamSubscriptionId;
    Routers::Consumer* d_consumer;
    bsls::TimeInterval d_lowestDelay;

    Visitor()
    : d_handle(0)
    , d_downstreamSubscriptionId(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID)
    , d_consumer(0)
    , d_lowestDelay(k_MAX_SECONDS, k_MAX_NANOSECONDS)
    {
        // NOTHING
    }
    bool oneConsumer(const Routers::Subscription* subscription)
    {
        d_downstreamSubscriptionId = subscription->d_downstreamSubscriptionId;
        d_consumer                 = subscription->consumer();
        d_handle                   = subscription->handle();

        return true;
    }
    bool minDelayConsumer(bsls::TimeInterval*          delay,
                          const Routers::Subscription* subscription,
                          const bsls::TimeInterval&    messageDelay,
                          const bsls::TimeInterval&    now)
    {
        BSLS_ASSERT_SAFE(subscription);

        bsls::TimeInterval delayLeft =
            subscription->consumer()->d_timeLastMessageSent + messageDelay -
            now;

        if (delayLeft <= 0) {
            d_handle   = subscription->handle();
            d_consumer = subscription->consumer();
            d_downstreamSubscriptionId =
                subscription->d_downstreamSubscriptionId;

            return true;
        }
        if (d_lowestDelay > delayLeft) {
            *delay = d_lowestDelay = delayLeft;
        }
        return false;
    }
};

}  // close unnamed namespace

// ----------------------
// struct QueueEngineUtil
// ----------------------

bool QueueEngineUtil::consumerAndProducerLimitsAreValid(
    QueueState*                                queueState,
    bsl::ostream&                              errorDescription,
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    BSLS_ASSERT_SAFE(queueState);

    bool              isValid = true;
    mqbi::QueueCounts counts  = queueState->consumerAndProducerCounts(
        handleParameters);
    const mqbconfm::Domain& domainConfig =
        queueState->queue()->domain()->config();

    if (domainConfig.maxProducers() &&
        ((handleParameters.writeCount() + counts.d_writeCount) >
         domainConfig.maxProducers())) {
        errorDescription << "Client would exceed the limit of "
                         << domainConfig.maxProducers() << " producer(s)";

        isValid = false;
    }

    if (domainConfig.maxConsumers() &&
        ((handleParameters.readCount() + counts.d_readCount) >
         domainConfig.maxConsumers())) {
        errorDescription << (isValid ? "Client would exceed the limit of "
                                     : " and the limit of ")
                         << domainConfig.maxConsumers() << " consumer(s)";

        isValid = false;
    }

    return isValid;
}

int QueueEngineUtil::validateUri(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    mqbi::QueueHandle*                         handle,
    const mqbi::QueueHandleRequesterContext&   clientContext)
{
    bmqt::Uri   uri;
    bsl::string error;
    int rc = bmqt::UriParser::parse(&uri, &error, handleParameters.uri());
    (void)rc;  // compiler happiness
    if (handle->queue()->uri().canonical() != uri.canonical()) {
        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "#CLIENT_IMPROPER_BEHAVIOR "
                << "Mismatched queue URIs for same queueId for a "
                << "client. Rejecting request.";
            if (clientContext.requesterId() !=
                mqbi::QueueHandleRequesterContext::k_INVALID_REQUESTER_ID) {
                BALL_LOG_OUTPUT_STREAM
                    << " ClientPtr '" << clientContext.client()
                    << "', requesterId '" << clientContext.requesterId()
                    << "',";
            }
            BALL_LOG_OUTPUT_STREAM
                << " queue handlePtr '" << handle << "'. Queue handle's URI '"
                << handle->queue()->uri()
                << "', specified handle parameters: " << handleParameters
                << ".";
        }
        return -1;  // RETURN
    }

    return 0;
}

void QueueEngineUtil::reportQueueTimeMetric(
    mqbstat::QueueStatsDomain*            domainStats,
    const mqbi::StorageMessageAttributes& attributes,
    const bsl::string&                    appId)
{
    // Report delivered message's queue-time.
    bsls::Types::Int64 timeDelta;
    mqbs::StorageUtil::loadArrivalTimeDelta(&timeDelta, attributes);

    domainStats->reportQueueTime(timeDelta, appId);
}

// static
bool QueueEngineUtil::loadMessageDelay(
    const bmqp::RdaInfo&                 rdaInfo,
    const mqbcfg::MessageThrottleConfig& messageThrottleConfig,
    bsls::TimeInterval*                  delay)
{
    unsigned int counter = rdaInfo.counter();
    if (!rdaInfo.isPotentiallyPoisonous() ||
        counter > messageThrottleConfig.highThreshold()) {
        // No delay if poison pill is disabled or the counter is above
        // a threshold.
        return false;  // RETURN
    }
    else if (counter > messageThrottleConfig.lowThreshold()) {
        // If the counter is at the threshold, we add a shorter delay.
        delay->addMilliseconds(messageThrottleConfig.lowInterval());
        return true;  // RETURN
    }
    else {
        // If the counter is below the threshold, we add a longer delay.
        delay->addMilliseconds(messageThrottleConfig.highInterval());
        return true;  // RETURN
    }
}

int QueueEngineUtil::dumpMessageInTempfile(
    bsl::string*                   filepath,
    const bdlbb::Blob&             payload,
    const bmqp::MessageProperties* properties)
{
    enum RcEnum {
        // Return values are part of function contract.  Do not change them
        // without updating the callers.
        rc_SUCCESS               = 0,
        rc_FILE_CREATION_FAILURE = -1,
        rc_FILE_OPEN_FAILURE     = -2
    };

    const bsl::string                    prefix = mwcu::TempUtil::tempDir();
    bdls::FilesystemUtil::FileDescriptor fileDescriptor =
        bdls::FilesystemUtil::createTemporaryFile(filepath, prefix);
    if (fileDescriptor == bdls::FilesystemUtil::k_INVALID_FD) {
        return rc_FILE_CREATION_FAILURE;  // RETURN
    }

    ::fchmod(fileDescriptor, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    // Ignore rc above

    bsl::ofstream msg(filepath->c_str());
    if (!msg.is_open()) {
        bdls::FilesystemUtil::remove(*filepath);  // ignore rc
        return rc_FILE_OPEN_FAILURE;              // RETURN
    }

    if (properties) {
        msg << "Message Properties:\n\n"
            << *properties << "\n\n\nMessage Payload:\n\n"
            << bdlbb::BlobUtilHexDumper(&payload);
    }
    else {
        msg << "Application Data:\n\n" << bdlbb::BlobUtilHexDumper(&payload);
    }
    msg.close();

    return rc_SUCCESS;
}

void QueueEngineUtil::logRejectMessage(
    const bmqt::MessageGUID&              msgGUID,
    const bsl::string&                    appId,
    unsigned int                          subQueueId,
    const bsl::shared_ptr<bdlbb::Blob>&   appData,
    const mqbi::StorageMessageAttributes& attributes,
    const QueueState*                     queueState,
    bslma::Allocator*                     allocator)
{
    bdlbb::Blob payload(queueState->blobBufferFactory(), allocator);
    bmqp::MessageProperties properties(allocator);
    int                     attemptedDeliveries =
        queueState->domain()->config().maxDeliveryAttempts();
    bdlbb::Blob msgProperties(queueState->blobBufferFactory(), allocator);
    int         messagePropertiesSize = 0;
    int         rc                    = bmqp::ProtocolUtil::parse(
        &msgProperties,
        &messagePropertiesSize,
        &payload,
        *appData,
        appData->length(),
        true,
        mwcu::BlobPosition(),
        attributes.messagePropertiesInfo().isPresent(),
        attributes.messagePropertiesInfo().isExtended(),
        attributes.compressionAlgorithmType(),
        queueState->blobBufferFactory(),
        allocator);

    if (rc == 0 && attributes.messagePropertiesInfo().isPresent()) {
        rc = properties.streamIn(
            msgProperties,
            attributes.messagePropertiesInfo().isExtended());
    }

    bsl::string filepath;

    if (rc != 0) {
        MWCTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was "
            << "transmitted a total of " << attemptedDeliveries
            << " times to consumer(s). BlazingMQ failed to load message "
            << "properties with internal error code " << rc
            << "Dumping raw message." << MWCTSK_ALARMLOG_END;
        rc = dumpMessageInTempfile(&filepath, *appData, 0);
        // decoding failed
    }
    else {
        rc = dumpMessageInTempfile(&filepath, payload, &properties);
    }

    if (rc == -1) {
        MWCTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was transmitted a total of " << attemptedDeliveries
            << " times to consumer(s). Attempt to dump message in a file "
            << "failed because file could not be created."
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (rc == -2) {
        MWCTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was transmitted a total of " << attemptedDeliveries
            << " times to consumer(s). Attempt to dump message in a file "
            << "failed because file could not be opened."
            << MWCTSK_ALARMLOG_END;
        return;  // RETURN
    }

    MWCTSK_ALARMLOG_ALARM("POISON_PILL")
        << "A poison pill message was detected and purged from the queue "
        << queueState->uri() << " [GUID: " << msgGUID << ", appId: " << appId
        << ", subQueueId: " << subQueueId
        << "]. Message was transmitted a total of " << attemptedDeliveries
        << " times to consumer(s). Message was dumped in file at location ["
        << filepath << "] on this machine. Please copy file before it gets "
        << "deleted." << MWCTSK_ALARMLOG_END;
}

// -------------------------------------------
// struct QueueEngineUtil_ReleaseHandleProctor
// -------------------------------------------

QueueEngineUtil_ReleaseHandleProctor::QueueEngineUtil_ReleaseHandleProctor(
    QueueState*                                      queueState,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
: d_queueState_p(queueState)
, d_isFinal(isFinal)
, d_releasedCb(releasedCb)
, d_disableCallback(false)
{
}

mqbi::QueueCounts QueueEngineUtil_ReleaseHandleProctor::countDiff(
    const bmqp_ctrlmsg::SubQueueIdInfo& info,
    int                                 readCount,
    int                                 writeCount) const
{
    BSLS_ASSERT_SAFE(d_handleSp);

    mqbi::QueueHandle::SubStreams::const_iterator itSubStream =
        d_handleSp->subStreamInfos().find(info.appId());

    BSLS_ASSERT_SAFE(itSubStream != d_handleSp->subStreamInfos().end());

    mqbi::QueueCounts result(
        itSubStream->second.d_counts.d_readCount - readCount,
        itSubStream->second.d_counts.d_writeCount - writeCount);

    BSLS_ASSERT_SAFE(result.isValid());

    return result;
}

int QueueEngineUtil_ReleaseHandleProctor::releaseHandle(
    mqbi::QueueHandle*                         handle,
    const bmqp_ctrlmsg::QueueHandleParameters& params)
{
    bsls::Types::Uint64 lostFlags = 0;
    int rc = d_queueState_p->handleCatalog().releaseHandleHelper(&d_handleSp,
                                                                 &lostFlags,
                                                                 handle,
                                                                 params,
                                                                 d_isFinal);
    if (rc != 0) {
        return rc;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_handleSp);

    if (bmqp::QueueUtil::isEmpty(handle->handleParameters())) {
        d_isFinal = true;
    }

    if (d_isFinal) {
        // no more consumers or producers for all subStream for this handle
        d_result.makeNoHandleClients();
    }

    if (bmqt::QueueFlagsUtil::isReader(lostFlags)) {
        // no more consumers for any subStreams for this handle
        d_result.makeNoHandleStreamConsumers();
    }

    if (bmqt::QueueFlagsUtil::isWriter(lostFlags)) {
        // no more producers for any subStreams for this handle
        d_result.makeNoHandleStreamProducers();
    }

    return 0;
}

mqbi::QueueHandleReleaseResult
QueueEngineUtil_ReleaseHandleProctor::releaseQueueStream(
    const bmqp_ctrlmsg::QueueHandleParameters& params)
{
    BSLS_ASSERT_SAFE(d_handleSp);

    mqbi::QueueHandleReleaseResult result;
    // Update queue's aggregated parameters.
    mqbi::QueueCounts cumulative = d_queueState_p->subtract(params);

    if (cumulative.d_readCount == 0) {
        // no more consumers for this subStream across all handles
        result.makeNoQueueStreamConsumers();
    }
    if (cumulative.d_writeCount == 0) {
        // no more producers for this subStream across all handles
        result.makeNoQueueStreamProducers();
    }
    return result;
}

mqbi::QueueHandleReleaseResult
QueueEngineUtil_ReleaseHandleProctor::releaseStream(
    const bmqp_ctrlmsg::QueueHandleParameters& params)
{
    const bmqp_ctrlmsg::SubQueueIdInfo& info =
        bmqp::QueueUtil::extractSubQueueInfo(params);

    mqbi::QueueCounts diff = countDiff(info,
                                       params.readCount(),
                                       params.writeCount());

    // `d_result` is accumulated result in the case of wildcard.
    // `result` is about the given stream only.
    mqbi::QueueHandleReleaseResult result;

    if (0 == diff.d_readCount) {
        result.makeNoHandleStreamConsumers();
    }
    if (0 == diff.d_writeCount) {
        result.makeNoHandleStreamProducers();
    }
    result.apply(releaseQueueStream(params));

    if (d_isFinal && (diff.d_readCount || diff.d_writeCount)) {
        // mismatch between isFinal and actual counts in the closeQueue
        // request
        bmqp_ctrlmsg::QueueHandleParameters copy(params);
        if (diff.d_readCount) {
            copy.flags() |= bmqt::QueueFlags::e_READ;
        }
        if (diff.d_writeCount) {
            copy.flags() |= bmqt::QueueFlags::e_WRITE;
        }
        copy.readCount()  = diff.d_readCount;
        copy.writeCount() = diff.d_writeCount;

        result.apply(releaseQueueStream(copy));

        result.makeNoHandleStreamConsumers();
        result.makeNoHandleStreamProducers();
    }
    d_result.apply(result);

    return result;
}

void QueueEngineUtil_ReleaseHandleProctor::addRef()
{
    ++d_refCount;
}

void QueueEngineUtil_ReleaseHandleProctor::release()
{
    BSLS_ASSERT_SAFE(d_refCount);

    --d_refCount;

    invokeCallback();
}

void QueueEngineUtil_ReleaseHandleProctor::invokeCallback()
{
    if (d_refCount) {
        return;  // RETURN
    }

    if (d_disableCallback) {
        return;  // RETURN
    }

    if (d_releasedCb) {
        d_disableCallback = true;
        d_releasedCb(d_handleSp, d_result);
    }
    else {
        if (d_isFinal && d_handleSp) {
            // If no 'releasedCb' was provided, this likely means this release
            // was a 'drop'.  If the handle is to be deleted, we need to keep
            // it alive until the associated client's dispatcher thread queue
            // has been drained: there could still be messages in transit (such
            // as PUSH messages) targeted for that client; so enqueue a dummy
            // event to the client's dispatcher thread which will do nothing
            // but hold on the handle shared pointer, effectively postponing
            // its destruction.  Note that we don't need to do any special
            // bookkeeping and flagging of that 'dropped' state (for example to
            // block communicating anymore with the client in the queue handle)
            // because the client itself makes sure to synchronize on the queue
            // dispatcher thread after calling drop, and properly handle (or
            // discard) any messages and events.

            // Client may or may not be alive at this point.  If it is alive at
            // this time, it may get destroyed by the time a dispatcher event
            // targeted to it (the client object) is processed by client
            // dispatcher thread.  In order to take care of both of these
            // scenarios, we enqueue the dummy callback containing handleSp to
            // all client session dispatcher threads.

            // Lastly, a client may be represented by 'mqba::ClientSession' or
            // 'mqbblp::ClusterNodeSession'.  So we enqueue to e_SESSION (which
            // represents 'mqba::ClientSession') as well as e_CLUSTER (which
            // represents 'mqbblp::ClusterNodeSession').

            d_queueState_p->queue()->dispatcher()->execute(
                mqbi::Dispatcher::ProcessorFunctor(),
                mqbi::DispatcherClientType::e_SESSION,
                bdlf::BindUtil::bind(&queueHandleHolderDummy, d_handleSp));

            d_queueState_p->queue()->dispatcher()->execute(
                mqbi::Dispatcher::ProcessorFunctor(),
                mqbi::DispatcherClientType::e_CLUSTER,
                bdlf::BindUtil::bind(&queueHandleHolderDummy, d_handleSp));
        }
    }
}

QueueEngineUtil_ReleaseHandleProctor::~QueueEngineUtil_ReleaseHandleProctor()
{
    invokeCallback();
    // 'd_refCount' can be non-zero in the case when RelayQE gets destroyed as
    // a result of conversion to Primary before CloseQueue response.
}

// ------------------------------------------
// struct QueueEngineUtil_AppsDeliveryContext
// ------------------------------------------

QueueEngineUtil_AppsDeliveryContext::QueueEngineUtil_AppsDeliveryContext(
    mqbi::Queue*      queue,
    bslma::Allocator* allocator)
: d_consumers(allocator)
, d_doRepeat(true)
, d_currentMessage(0)
, d_queue_p(queue)
, d_activeAppIds(allocator)
{
    // NOTHING
}

void QueueEngineUtil_AppsDeliveryContext::reset()
{
    d_doRepeat       = false;
    d_currentMessage = 0;
    d_consumers.clear();
    d_activeAppIds.clear();
}

bool QueueEngineUtil_AppsDeliveryContext::processApp(
    QueueEngineUtil_AppState& app)
{
    // For each App:
    //   1. Pick next message to send (next message of first 'not at the end'
    //      subStream).
    //   2. Accumulate subQueueIds of all subStreams if their next message is
    //      the same.
    //   3. Deliver the message.
    //   4. Advance all affected subStreams.
    //   5. If there was a different message somewhere or if an advance
    //      indicated there are other messages, repeat.

    BSLS_ASSERT_SAFE(app.d_storageIter_mp);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !app.d_storageIter_mp->hasReceipt())) {
        // An unregistered App Id has void iterator that always points
        // at the end.
        // Or, still waiting for Receipt
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }
    if (d_currentMessage) {
        if (d_currentMessage->guid() != app.d_storageIter_mp->guid()) {
            // This app needs to deliver another message.  It will be picked up
            // in next iteration(s).
            d_doRepeat = true;
            return false;  // RETURN
        }
    }
    // This app has a message to deliver that is either the first
    // message in the current iteration or an identical message.

    const bool isBroadcast = d_queue_p->isDeliverAll();
    bool       isSelected  = false;

    if (!isBroadcast) {
        // non-broadcast
        Routers::Result result = app.selectConsumer(
            bdlf::BindUtil::bind(&QueueEngineUtil_AppsDeliveryContext::visit,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 app.d_storageIter_mp.get()),
            app.d_storageIter_mp.get());

        if (result == Routers::e_SUCCESS) {
            isSelected = true;
        }
        else if (result == Routers::e_NO_CAPACITY_ALL) {
            // Do not want to advance the iterator
            return false;  // RETURN
        }
        else {
            // NOTE: Above, we do not call 'StorageIterator::appData'
            // associated with the message.  This is being done so that we
            // don't end up aliasing the corresponding message and options area
            // to the mapped file, which will otherwise increment the aliased
            // counter of the mapped file, which can delay the unmapping of
            // files in case this message has a huge TTL and there are no
            // consumers for this message. Also see internal-ticket D164392124
            // for additional context.

            // This app does not have capacity to deliver
            app.d_putAsideList.add(app.d_storageIter_mp->guid());
        }
    }
    else {
        isSelected = true;
    }
    // This app has a message to deliver.  It is not at the end, has
    // capacity (if it is a non-broadcast).

    if (0 == d_currentMessage && isSelected) {
        // This is the first message to deliver in the current Apps iteration
        d_currentMessage = app.d_storageIter_mp.get();
        // Do not advance current; will need to call messageIterator->appData()
    }
    else if (app.d_storageIter_mp->advance()) {
        // Advance the storageIter of this subStream.  Ideally this should be
        // done *after* 'QueueHandle::deliverMessage' has been invoked, but
        // there is no failure path b/w here and that code (apart from the fact
        // that 'getMessageDetails' could fail, at which point, it is probably
        // a good idea to advance the iterator since that message cannot be
        // recovered from the storage).

        // There is at least one more message to deliver
        d_doRepeat = true;
    }

    if (isBroadcast) {
        // collect all handles
        app.d_routing_sp->iterateConsumers(
            bdlf::BindUtil::bind(
                &QueueEngineUtil_AppsDeliveryContext::visitBroadcast,
                this,
                bdlf::PlaceHolders::_1),
            app.d_storageIter_mp.get());
    }
    else {
        // Store appId of active consumer for domain stats (reporting queue
        // time metric)
        d_activeAppIds.push_back(app.d_appId);
    }

    return isSelected;
}

bool QueueEngineUtil_AppsDeliveryContext::visit(
    const Routers::Subscription* subscription,
    const mqbi::StorageIterator* message)
{
    BSLS_ASSERT_SAFE(subscription);

    d_consumers[subscription->handle()].push_back(
        bmqp::SubQueueInfo(subscription->d_downstreamSubscriptionId,
                           message->rdaInfo()));

    return true;
}

bool QueueEngineUtil_AppsDeliveryContext::visitBroadcast(
    const Routers::Subscription* subscription)
{
    BSLS_ASSERT_SAFE(subscription);

    d_consumers[subscription->handle()].push_back(
        bmqp::SubQueueInfo(subscription->d_downstreamSubscriptionId));

    return false;
}

void QueueEngineUtil_AppsDeliveryContext::deliverMessage()
{
    if (d_currentMessage) {
        const mqbi::StorageMessageAttributes& attributes =
            d_currentMessage->attributes();
        for (Consumers::const_iterator it = d_consumers.begin();
             it != d_consumers.end();
             ++it) {
            BSLS_ASSERT_SAFE(!it->second.empty());

            if (QueueEngineUtil::isBroadcastMode(d_queue_p)) {
                it->first->deliverMessageNoTrack(d_currentMessage->appData(),
                                                 d_currentMessage->guid(),
                                                 attributes,
                                                 "",  // msgGroupId,
                                                 it->second);
            }
            else {
                it->first->deliverMessage(d_currentMessage->appData(),
                                          d_currentMessage->guid(),
                                          attributes,
                                          "",  // msgGroupId,
                                          it->second);
            }
        }

        if (bmqp::QueueId::k_PRIMARY_QUEUE_ID == d_queue_p->id()) {
            // Report 'queue time' metric for all active appIds
            bsl::vector<bslstl::StringRef>::const_iterator it =
                d_activeAppIds.begin();
            for (; it != d_activeAppIds.cend(); ++it) {
                QueueEngineUtil::reportQueueTimeMetric(d_queue_p->stats(),
                                                       attributes,
                                                       *it  // appId
                );
            }
        }

        if (d_currentMessage->advance()) {
            // There is at least one more message to deliver
            d_doRepeat = true;
        }
    }
}

// -------------------------
// struct AppConsumers_State
// -------------------------

// CREATORS
QueueEngineUtil_AppState::QueueEngineUtil_AppState(
    bslma::ManagedPtr<mqbi::StorageIterator> iterator,
    mqbi::Queue*                             queue,
    bdlmt::EventScheduler*                   scheduler,
    bool                                     isAuthorized,
    Routers::QueueRoutingContext&            queueContext,
    unsigned int                             upstreamSubQueueId,
    const bsl::string&                       appId,
    const mqbu::StorageKey&                  appKey,
    bslma::Allocator*                        allocator)
: d_storageIter_mp(iterator)
, d_routing_sp(new (*allocator) Routers::AppContext(queueContext, allocator),
               allocator)
, d_redeliveryList(allocator)
, d_putAsideList(allocator)
, d_priorityCount(0)
, d_queue_p(queue)
, d_isAuthorized(isAuthorized)
, d_scheduler_p(scheduler)
, d_appKey(appKey)
, d_appId(appId)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_isScheduled(false)
{
    // Above, we retrieve domain config from 'queue' only if self node is a
    // cluster member, and pass a dummy config if self is proxy, because proxy
    // nodes don't load the domain config.
    BSLS_ASSERT_SAFE(d_scheduler_p);

    d_autoSubscription.d_evaluationContext_p =
        &d_routing_sp->d_queue.d_evaluationContext;

    d_throttledEarlyExits.initialize(1, 5 * bdlt::TimeUnitRatio::k_NS_PER_S);
}

QueueEngineUtil_AppState::~QueueEngineUtil_AppState()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!hasConsumers());
    BSLS_ASSERT_SAFE(!d_throttleEventHandle);
}

size_t
QueueEngineUtil_AppState::deliverMessages(bsls::TimeInterval*     delay,
                                          const mqbu::StorageKey& appKey,
                                          mqbi::Storage&          storage,
                                          const bsl::string&      appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasConsumers())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    size_t numMessages = processDeliveryLists(delay, appKey, storage, appId);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(redeliveryListSize())) {
        // We only attempt to deliver new messages if we successfully
        // redelivered all messages in the redelivery list.
        return numMessages;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_storageIter_mp);

    // Deliver messages until either:
    //   1. End of storage; or
    //   2. subStream's capacity is saturated
    mqbi::StorageIterator* storageIter_p = d_storageIter_mp.get();

    while (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(storageIter_p->hasReceipt())) {
        Routers::Result result = Routers::e_SUCCESS;

        if (QueueEngineUtil::isBroadcastMode(d_queue_p)) {
            broadcastOneMessage(storageIter_p);
        }
        else {
            result = tryDeliverOneMessage(delay, storageIter_p);

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    result == Routers::e_NO_CAPACITY ||
                    result == Routers::e_NO_SUBSCRIPTION)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                d_putAsideList.add(storageIter_p->guid());
                // Do not block other Subscriptions. Continue.
            }
            else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                         result != Routers::e_SUCCESS)) {
                break;
            }
        }

        if (result == Routers::e_SUCCESS) {
            if (bmqp::QueueId::k_PRIMARY_QUEUE_ID == d_queue_p->id()) {
                QueueEngineUtil::reportQueueTimeMetric(
                    d_queue_p->stats(),
                    storageIter_p->attributes(),
                    appId);
            }
            ++numMessages;
        }

        storageIter_p->advance();
    }
    return numMessages;
}

Routers::Result QueueEngineUtil_AppState::tryDeliverOneMessage(
    bsls::TimeInterval*          delay,
    const mqbi::StorageIterator* message)
{
    // In order to try and deliver a message, we need to:
    //      1. Determine if a message has a delay based on its rdaInfo.
    //      2. Use the router to get candidate handle for the message.
    //      3. If the message doesn't have a delay, there's no point in going
    //         through the loop since the message is ready to be sent right
    //         away (the message throttling is only a function of the delay
    //         on the current message). We will then use the handle we
    //         obtained.
    //      4. If the message has a delay, determine if we can send a message
    //         by seeing if the specified 'messageDelay' has passed between
    //         'now' and when the last message was sent through the handle
    //         (queueHandleContext.d_timeLastMessageSent).
    //         If the delay has passed, we can use that handle to send the
    //         message right away. Else, we go back to the router and try the
    //         same thing with different handles until we either find a
    //         suitable handle or we exhaust all handles.
    //      5. If we find a suitable handle, we send the message through that
    //         handle, update its queueHandleContext, and return true.
    //         If all the handles have a delay, we will load 'lowestDelay'
    //         (how long it will take for a handle to be available) into
    //         'delay' and return false.

    bsls::TimeInterval messageDelay;
    bsls::TimeInterval now = mwcsys::Time::nowMonotonicClock();
    Visitor            visitor;
    Routers::Result    result = Routers::e_SUCCESS;

    if (!QueueEngineUtil::loadMessageDelay(message->rdaInfo(),
                                           d_queue_p->messageThrottleConfig(),
                                           &messageDelay)) {
        result = selectConsumer(bdlf::BindUtil::bind(&Visitor::oneConsumer,
                                                     &visitor,
                                                     bdlf::PlaceHolders::_1),
                                message);
    }
    else {
        // Iterate all highest priority consumers and find the lowest delay
        if (!d_routing_sp->iterateConsumers(
                bdlf::BindUtil::bind(&Visitor::minDelayConsumer,
                                     &visitor,
                                     delay,
                                     bdlf::PlaceHolders::_1,
                                     messageDelay,
                                     now),
                message)) {
            result = Routers::e_DELAY;
        }

        // If an available handle doesn't cause us to break out of the loop, we
        // will populate the delay with the min waiting time before a handle is
        // available. We also have to make sure we iterated through at least
        // one consumer so we don't return a lowestDelay set to its max value.
    }

    if (!visitor.d_handle) {
        return result;  // RETURN
    }
    BSLS_ASSERT_SAFE(visitor.d_consumer);

    BSLS_ASSERT_SAFE(result == Routers::e_SUCCESS);

    const bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        1,
        bmqp::SubQueueInfo(visitor.d_downstreamSubscriptionId,
                           message->rdaInfo()));
    visitor.d_handle->deliverMessage(message->appData(),
                                     message->guid(),
                                     message->attributes(),
                                     "",  // msgGroupId
                                     subQueueInfos);

    visitor.d_consumer->d_timeLastMessageSent = now;
    visitor.d_consumer->d_lastSentMessage     = message->guid();

    return result;
}

void QueueEngineUtil_AppState::beforeMessageRemoved(
    const bmqt::MessageGUID& msgGUID,
    bool                     isExpired)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_storageIter_mp);

    if (!d_storageIter_mp->atEnd() && (d_storageIter_mp->guid() == msgGUID)) {
        d_storageIter_mp->advance();
    }

    d_redeliveryList.erase(msgGUID);

    if (isExpired) {
        d_putAsideList.erase(msgGUID);
    }
}

void QueueEngineUtil_AppState::broadcastOneMessage(
    const mqbi::StorageIterator* storageIter)
{
    d_routing_sp->iterateConsumers(
        bdlf::BindUtil::bind(&QueueEngineUtil_AppState::visitBroadcast,
                             this,
                             storageIter,
                             bdlf::PlaceHolders::_1),
        storageIter);
}

bool QueueEngineUtil_AppState::visitBroadcast(
    const mqbi::StorageIterator* message,
    const Routers::Subscription* subscription)
{
    mqbi::QueueHandle* handle = subscription->handle();
    BSLS_ASSERT_SAFE(handle);
    // TBD: groupId: send 'options' as well...
    handle->deliverMessageNoTrack(
        message->appData(),
        message->guid(),
        message->attributes(),
        "",  // msgGroupId
        bmqp::Protocol::SubQueueInfosArray(
            1,
            bmqp::SubQueueInfo(subscription->d_downstreamSubscriptionId)));

    return false;
}

size_t
QueueEngineUtil_AppState::processDeliveryLists(bsls::TimeInterval*     delay,
                                               const mqbu::StorageKey& appKey,
                                               mqbi::Storage&          storage,
                                               const bsl::string&      appId)
{
    size_t numMessages =
        processDeliveryList(delay, appKey, storage, appId, d_redeliveryList);
    if (*delay == bsls::TimeInterval()) {
        // The only excuse for stopping the iteration is poisonous message
        numMessages =
            processDeliveryList(delay, appKey, storage, appId, d_putAsideList);
    }
    return numMessages;
}

size_t
QueueEngineUtil_AppState::processDeliveryList(bsls::TimeInterval*     delay,
                                              const mqbu::StorageKey& appKey,
                                              mqbi::Storage&          storage,
                                              const bsl::string&      appId,
                                              RedeliveryList&         list)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(list.empty())) {
        return 0;  // RETURN
    }

    // For each message in the pending redelivery list
    RedeliveryList::iterator it          = list.begin();
    bmqt::MessageGUID        firstGuid   = *it;
    size_t                   numMessages = 0;

    while (!list.isEnd(it)) {
        // Retrieve message from storage
        bslma::ManagedPtr<mqbi::StorageIterator> message;

        mqbi::StorageResult::Enum rc = storage.getIterator(&message,
                                                           appKey,
                                                           *it);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                rc != mqbi::StorageResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_WARN
                << "#STORAGE_UNKNOWN_MESSAGE "
                << "Error '" << rc << "' while retrieving msg from queue: '"
                << d_queue_p->description()
                << "', while attempting to re-distribute unconfirmed  message "
                << "with GUID: '" << *it << "'";

            // Couldn't send it, but will never be able to do so; just consider
            // it as sent.
            it = list.erase(it);
            continue;  // CONTINUE
        }
        // TEMPORARILY handling unknown 'Group's in RelayQE by reevaluating.
        // Instead, should communicate them upstream either in CloseQueue or in
        // Rejects.

        Routers::Result result = tryDeliverOneMessage(delay, message.get());

        if (result == Routers::e_NO_CAPACITY_ALL) {
            break;  // BREAK
        }
        else if (result == Routers::e_DELAY) {
            break;  // BREAK
        }
        else if (result == Routers::e_SUCCESS) {
            // Remove from the redeliveryList
            it = list.erase(it);

            ++numMessages;
            if (bmqp::QueueId::k_PRIMARY_QUEUE_ID == d_queue_p->id()) {
                QueueEngineUtil::reportQueueTimeMetric(d_queue_p->stats(),
                                                       message->attributes(),
                                                       appId);
            }
        }
        else {
            if (result == Routers::e_NO_SUBSCRIPTION) {
                // Skip 'it' until config changes
                list.disable(&it);
            }
            list.next(&it);
        }
    };

    if (numMessages) {
        BALL_LOG_INFO << "Queue '" << d_queue_p->description()
                      << "', appId = '" << appId << " (re)delivered "
                      << numMessages << " messages starting from " << firstGuid
                      << ".";
    }
    return numMessages;
}

void QueueEngineUtil_AppState::tryCancelThrottle(
    mqbi::QueueHandle*       handle,
    const bmqt::MessageGUID& msgGUID)
{
    Routers::Consumer* queueHandleContext = findQueueHandleContext(handle);
    if (!queueHandleContext) {
        // This is the case where a handle can be deconfigured but then later
        // confirm a message. Since there will be no more outbound messages,
        // there's nothing to throttle on this handle.
        return;  // RETURN
    }

    if (queueHandleContext->d_lastSentMessage != msgGUID) {
        return;  // RETURN
    }

    queueHandleContext->d_timeLastMessageSent = 0;
    if (d_throttleEventHandle) {
        // No need to wait anymore.  Can deliver the next message (if any)
        // immediately.
        d_scheduler_p->rescheduleEvent(d_throttleEventHandle,
                                       bsls::TimeInterval());
        // Do not check the return value.  If the event is already dispatched,
        // then there is no need to reschedule.
    }
}

void QueueEngineUtil_AppState::scheduleThrottle(
    bsls::TimeInterval           executionTime,
    const bsl::function<void()>& deliverMessageFn)
{
    int shouldSchedule = -1;

    d_isScheduled.store(true);

    if (d_throttleEventHandle) {
        // We need to check the return code for the case that event was already
        // dispatched in the event scheduler's thread but the handle wasn't
        // released yet.
        shouldSchedule = d_scheduler_p->rescheduleEvent(d_throttleEventHandle,
                                                        executionTime);
    }

    if (shouldSchedule != 0) {
        d_scheduler_p->scheduleEvent(
            &d_throttleEventHandle,
            executionTime,
            bdlf::BindUtil::bind(
                &QueueEngineUtil_AppState::executeInQueueDispatcher,
                this,
                deliverMessageFn));
    }
}

void QueueEngineUtil_AppState::executeInQueueDispatcher(
    const bsl::function<void()>& deliverMessageFn)
{
    // executed by the *SCHEDULER DISPATCHER* thread
    if (d_isScheduled.load()) {
        d_queue_p->dispatcher()->execute(
            bdlf::BindUtil::bind(&releaseHandleAndInvoke,
                                 &d_throttleEventHandle,
                                 &d_isScheduled,
                                 deliverMessageFn),
            d_queue_p);
    }
}

void QueueEngineUtil_AppState::cancelThrottle()
{
    d_isScheduled.store(false);

    if (d_throttleEventHandle) {
        d_scheduler_p->cancelEventAndWait(&d_throttleEventHandle);
    }
}

void QueueEngineUtil_AppState::reset()
{
    if (!hasConsumers()) {
        // This is the first consumer, reset the storage to point to the
        // first message: we do so because when the last consumer went
        // down, as an optimization, instead of enqueueing all its pending
        // messages to the redelivery list, we did nothing, relying on this
        // reset to point back to the beginning of the queue, which should
        // be the first un-confirmed message.
        d_storageIter_mp->reset();
        d_putAsideList.clear();
    }
    d_priorityCount = 0;
    cancelThrottle();

    d_redeliveryList.touch();
    d_putAsideList.touch();
}

void QueueEngineUtil_AppState::rebuildConsumers(
    const char*                                 appId,
    bsl::ostream*                               errorStream,
    const QueueState*                           queueState,
    const bsl::shared_ptr<Routers::AppContext>& replacement)
{
    // Rebuild ConsumersState for this app
    // Prepare the app for rebuilding consumers
    reset();

    bsl::shared_ptr<Routers::AppContext> previous = d_routing_sp;
    d_routing_sp                                  = replacement;

    queueState->handleCatalog().iterateConsumers(
        bdlf::BindUtil::bind(&Routers::AppContext::loadApp,
                             d_routing_sp.get(),
                             appId,
                             bdlf::PlaceHolders::_1,
                             // handle
                             errorStream,
                             bdlf::PlaceHolders::_2,
                             // info
                             previous.get()));

    d_priorityCount = d_routing_sp->finalize();

    d_routing_sp->apply();
    d_routing_sp->registerSubscriptions();
}

bool QueueEngineUtil_AppState::transferUnconfirmedMessages(
    mqbi::QueueHandle*                  handle,
    const bmqp_ctrlmsg::SubQueueIdInfo& subQueue)
{
    if (!hasConsumers()) {
        // This handle was the last consumer; no need to transfer
        // the 'unconfirmedMessageGUID' to the redelivery list;
        // we'll simply reset the storage iterator to the beginning
        // of the queue at the next consumer coming up.  However,
        // we need to clear its 'unconfirmedMessageGUID' list: this
        // is needed because the handle may not be deleted (it may
        // still have 'write' capacity); and could potentially
        // later get 'read' capacity again.  Also, we need to clear
        // the redelivery list because when a reader comes up again
        // we reset our storage iterator to point to the beginning
        // of all unconfirmed messages.

        BSLS_ASSERT_SAFE(d_storageIter_mp);
        BSLS_ASSERT_SAFE(d_priorityCount == 0);

        cancelThrottle();

        d_storageIter_mp->reset();

        handle->transferUnconfirmedMessageGUID(0, subQueue.subId());
        d_redeliveryList.clear();

        d_putAsideList.clear();

        return false;  // RETURN
    }                  // else there are other consumers of the given appId

    // Redistribute messages: append all pending messages to
    // the redelivery list.

    int numMsgs = handle->transferUnconfirmedMessageGUID(
        bdlf::BindUtil::bind(&RedeliveryList::add,
                             &d_redeliveryList,
                             bdlf::PlaceHolders::_1),
        subQueue.subId());

    // We lost a reader, try to redeliver any potential messages
    // that need redelivery.
    BALL_LOG_INFO << "Lost a reader for queue '" << d_queue_p->description()
                  << "', redelivering " << numMsgs << " message(s) to "
                  << consumers().size() << " remaining readers.";
    return true;
}

Routers::Result QueueEngineUtil_AppState::selectConsumer(
    const Routers::Visitor&      visitor,
    const mqbi::StorageIterator* currentMessage)
{
    Routers::Result result = d_routing_sp->selectConsumer(visitor,
                                                          currentMessage);
    if (result == Routers::e_NO_CAPACITY_ALL) {
        const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
        if (brkrCfg.brokerVersion() == bmqp::Protocol::k_DEV_VERSION ||
            d_throttledEarlyExits.requestPermission()) {
            BALL_LOG_INFO << "Queue '" << d_queue_p->description()
                          << "', appId = '" << d_appId
                          << "' does not have any subscription "
                             "capacity; early exits delivery at "
                          << currentMessage->guid();
        }
    }

    return result;
}

int QueueEngineUtil_AppState::setSubscription(
    const mqbconfm::Expression& value)
{
    d_subcriptionExpression = value;

    if (mqbconfm::ExpressionVersion::E_VERSION_1 == value.version()) {
        if (d_subcriptionExpression.text().length()) {
            int rc = d_autoSubscription.d_evaluator.compile(
                d_subcriptionExpression.text(),
                d_routing_sp->d_compilationContext);
            return rc;  // RETURN
        }
    }
    // Reset
    d_autoSubscription.d_evaluator = bmqeval::SimpleEvaluator();

    return 0;
}

bool QueueEngineUtil_AppState::evaluateAutoSubcription()
{
    return d_autoSubscription.evaluate();
}

bslma::ManagedPtr<mqbi::StorageIterator> QueueEngineUtil_AppState::head() const
{
    bslma::ManagedPtr<mqbi::StorageIterator> out;

    if (!d_putAsideList.empty()) {
        d_queue_p->storage()->getIterator(&out,
                                          d_appKey,
                                          d_putAsideList.first());
    }
    else if (!d_storageIter_mp->atEnd()) {
        d_queue_p->storage()->getIterator(&out,
                                          d_appKey,
                                          d_storageIter_mp->guid());
    }

    return out;
}

void QueueEngineUtil_AppState::loadInternals(mqbcmd::AppState* out) const
{
    out->appId()                = d_appId;
    out->numConsumers()         = d_routing_sp->d_consumers.size();
    out->redeliveryListLength() = d_redeliveryList.size();
    d_routing_sp->loadInternals(&out->roundRobinRouter());
}

}  // close namespace mqbblp
}  // close namespace BloombergLP
