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

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_printutil.h>
#include <bmqu_temputil.h>

// BDE
#include <ball_log.h>
#include <ball_logthrottle.h>
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

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;
// Time interval between messages logged with throttling.

#define BMQ_LOGTHROTTLE_INFO()                                                \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

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

// Return delivered message's queue-time from the specified 'attributes'
bsls::Types::Int64
getMessageQueueTime(const mqbi::StorageMessageAttributes& attributes)
{
    bsls::Types::Int64 timeDelta;
    mqbs::StorageUtil::loadArrivalTimeDelta(&timeDelta, attributes);
    return timeDelta;
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
    const bmqp::MessageProperties* properties,
    bdlbb::BlobBufferFactory*      blobBufferFactory)
{
    enum RcEnum {
        // Return values are part of function contract.  Do not change them
        // without updating the callers.
        rc_SUCCESS               = 0,
        rc_FILE_CREATION_FAILURE = -1,
        rc_FILE_OPEN_FAILURE     = -2
    };

    const bsl::string                    prefix = bmqu::TempUtil::tempDir();
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

    if (properties && properties->numProperties() > 0) {
        msg << "Message Properties:\n\n" << *properties;

        // Serialize properties into the blob and hexdump it
        const bdlbb::Blob& blob = properties->streamOut(
            blobBufferFactory,
            bmqp::MessagePropertiesInfo::makeNoSchema());
        msg << "\n\n\nMessage Properties hexdump:\n\n"
            << bdlbb::BlobUtilHexDumper(&blob);

        msg << "\n\nMessage Payload:\n\n"
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
        bmqu::BlobPosition(),
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
        BMQTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was " << "transmitted a total of "
            << attemptedDeliveries
            << " times to consumer(s). BlazingMQ failed to load message "
            << "properties with internal error code " << rc
            << "Dumping raw message." << BMQTSK_ALARMLOG_END;
        rc = dumpMessageInTempfile(&filepath,
                                   *appData,
                                   0,
                                   queueState->blobBufferFactory());
        // decoding failed
    }
    else {
        rc = dumpMessageInTempfile(&filepath,
                                   payload,
                                   &properties,
                                   queueState->blobBufferFactory());
    }

    if (rc == -1) {
        BMQTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was transmitted a total of " << attemptedDeliveries
            << " times to consumer(s). Attempt to dump message in a file "
            << "failed because file could not be created."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    if (rc == -2) {
        BMQTSK_ALARMLOG_ALARM("POISON_PILL")
            << "A poison pill message was detected and purged from the queue "
            << queueState->uri() << " [GUID: " << msgGUID
            << ", appId: " << appId << ", subQueueId: " << subQueueId
            << "]. Message was transmitted a total of " << attemptedDeliveries
            << " times to consumer(s). Attempt to dump message in a file "
            << "failed because file could not be opened."
            << BMQTSK_ALARMLOG_END;
        return;  // RETURN
    }

    BMQTSK_ALARMLOG_ALARM("POISON_PILL")
        << "A poison pill message was detected and purged from the queue "
        << queueState->uri() << " [GUID: " << msgGUID << ", appId: " << appId
        << ", subQueueId: " << subQueueId
        << "]. Message was transmitted a total of " << attemptedDeliveries
        << " times to consumer(s). Message was dumped in file at location ["
        << filepath << "] on this machine. Please copy file before it gets "
        << "deleted." << BMQTSK_ALARMLOG_END;
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
                mqbi::Dispatcher::VoidFunctor(),
                mqbi::DispatcherClientType::e_SESSION,
                bdlf::BindUtil::bind(&queueHandleHolderDummy, d_handleSp));

            d_queueState_p->queue()->dispatcher()->execute(
                mqbi::Dispatcher::VoidFunctor(),
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
, d_numApps(0)
, d_numStops(0)
, d_currentMessage(0)
, d_queue_p(queue)
, d_timeDelta()
, d_currentAppView_p(0)
, d_visitVisitor(
      bdlf::BindUtil::bindS(allocator,
                            &QueueEngineUtil_AppsDeliveryContext::visit,
                            this,
                            bdlf::PlaceHolders::_1))
, d_broadcastVisitor(bdlf::BindUtil::bindS(
      allocator,
      &QueueEngineUtil_AppsDeliveryContext::visitBroadcast,
      this,
      bdlf::PlaceHolders::_1))
{
    BSLS_ASSERT_SAFE(queue);
}

bool QueueEngineUtil_AppsDeliveryContext::reset(
    mqbi::StorageIterator* currentMessage)
{
    d_consumers.clear();
    d_timeDelta.reset();

    bool result = false;

    if (haveProgress() && currentMessage && currentMessage->hasReceipt()) {
        d_currentMessage = currentMessage;
        result           = true;
    }
    else {
        d_currentMessage = 0;
    }

    d_numApps  = 0;
    d_numStops = 0;

    return result;
}

bool QueueEngineUtil_AppsDeliveryContext::processApp(
    QueueEngineUtil_AppState& app,
    unsigned int              ordinal)
{
    BSLS_ASSERT_SAFE(d_currentMessage->hasReceipt());

    ++d_numApps;

    if (d_queue_p->isDeliverAll()) {
        // collect all handles
        app.routing()->iterateConsumers(d_broadcastVisitor, d_currentMessage);

        // Broadcast does not need stats nor any special per-message treatment.
        return false;  // RETURN
    }

    if (!app.isReadyForDelivery()) {
        if (app.resumePoint().isUnset() && app.isAuthorized()) {
            // The 'app' needs to resume (in 'deliverMessages').
            // The queue iterator can advance leaving the 'app' behind.
            app.setResumePoint(d_currentMessage->guid());
        }
        ++d_numStops;
        // else the existing resumePoint is earlier (if authorized)
        return false;  // RETURN
    }

    const mqbi::AppMessage& appView = d_currentMessage->appMessageView(
        ordinal);

    if (!appView.isNew()) {
        return true;  // RETURN
    }

    // NOTE: We avoid calling 'StorageIterator::appData'associated with the
    // message unless necessary.  This is being done so that we don't end
    // up aliasing the corresponding message and options area to the mapped
    // file, which will otherwise increment the aliased counter of the
    // mapped file, which can delay the unmapping of files in case this
    // message has a huge TTL and there are no consumers for this message.

    d_currentAppView_p     = &appView;
    Routers::Result result = app.selectConsumer(d_visitVisitor,
                                                d_currentMessage,
                                                ordinal);
    // We use this pointer only from `d_visitVisitor`, so not cleaning it is
    // okay, but we clean it to keep contract that `d_currentAppView_p` only
    // points at the actual AppView.
    d_currentAppView_p = NULL;

    if (result == Routers::e_SUCCESS) {
        // RootQueueEngine makes stat reports
    }
    else if (result == Routers::e_NO_CAPACITY_ALL) {
        // All subscriptions of thes App are at capacity
        // Do not grow the 'd_putAsideList'
        // Instead, wait for 'onHandleUsable' event and then catch up
        // from this resume point.

        app.setResumePoint(d_currentMessage->guid());

        // Early return.
        // If all Apps return 'e_NO_CAPACITY_ALL', stop the iteration
        // (d_numApps == 0).

        ++d_numStops;

        return false;  // RETURN
    }
    else {
        BSLS_ASSERT_SAFE(result == Routers::e_NO_SUBSCRIPTION ||
                         result == Routers::e_NO_CAPACITY);

        // This app does not have capacity to deliver.  Still, move on and
        // consider (evaluate) subsequent messages for the 'app'.
        app.putAside(d_currentMessage->guid());
    }

    // Still making progress (result != Routers::e_NO_CAPACITY_ALL)

    return (result == Routers::e_SUCCESS);
}

bool QueueEngineUtil_AppsDeliveryContext::visit(
    const Routers::Subscription* subscription)
{
    BSLS_ASSERT_SAFE(subscription);
    BSLS_ASSERT_SAFE(
        d_currentAppView_p &&
        "`d_currentAppView_p` must be assigned before calling this function");

    d_consumers[subscription->handle()].push_back(
        bmqp::SubQueueInfo(subscription->d_downstreamSubscriptionId,
                           d_currentAppView_p->d_rdaInfo));

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
    BSLS_ASSERT_SAFE(d_currentMessage);

    if (!d_consumers.empty()) {
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
                                          it->second,
                                          false);
            }
        }
    }

    if (haveProgress()) {
        d_currentMessage->advance();
    }

    d_currentMessage = 0;
}

bool QueueEngineUtil_AppsDeliveryContext::isEmpty() const
{
    return d_consumers.empty();
}

bool QueueEngineUtil_AppsDeliveryContext::haveProgress() const
{
    return (d_numStops < d_numApps || d_numApps == 0);
}

bsls::Types::Int64 QueueEngineUtil_AppsDeliveryContext::timeDelta()
{
    if (!d_timeDelta.has_value()) {
        d_timeDelta = getMessageQueueTime(d_currentMessage->attributes());
    }
    return d_timeDelta.value();
}

// -------------------------
// struct AppConsumers_State
// -------------------------

// CREATORS
QueueEngineUtil_AppState::QueueEngineUtil_AppState(
    mqbi::Queue*                  queue,
    bdlmt::EventScheduler*        scheduler,
    Routers::QueueRoutingContext& queueContext,
    unsigned int                  upstreamSubQueueId,
    const bsl::string&            appId,
    const mqbu::StorageKey&       appKey,
    bslma::Allocator*             allocator)
: d_routing_sp(new(*allocator) Routers::AppContext(queueContext, allocator),
               allocator)
, d_redeliveryList(allocator)
, d_putAsideList(allocator)
, d_priorityCount(0)
, d_queue_p(queue)
, d_isAuthorized(false)
, d_scheduler_p(scheduler)
, d_appKey(appKey)
, d_appId(appId)
, d_upstreamSubQueueId(upstreamSubQueueId)
, d_isScheduled(false)
, d_appOrdinal(mqbi::Storage::k_INVALID_ORDINAL)
{
    // Above, we retrieve domain config from 'queue' only if self node is a
    // cluster member, and pass a dummy config if self is proxy, because proxy
    // nodes don't load the domain config.
    BSLS_ASSERT_SAFE(d_scheduler_p);

    d_autoSubscription.d_evaluationContext_p =
        &d_routing_sp->d_queue.d_evaluationContext;

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    const int maxActionsPerInterval  = (brkrCfg.brokerVersion() ==
                                       bmqp::Protocol::k_DEV_VERSION)
                                           ? 32
                                           : 1;

    d_throttledEarlyExits.initialize(maxActionsPerInterval,
                                     5 * bdlt::TimeUnitRatio::k_NS_PER_S);
}

QueueEngineUtil_AppState::~QueueEngineUtil_AppState()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_throttleEventHandle);

    // In the case of `convertToLocal`, the new `RootQueueEngine` can reuse the
    // existing `RelayQueueEngine` routing contexts.
}

size_t
QueueEngineUtil_AppState::deliverMessages(bsls::TimeInterval*          delay,
                                          mqbi::StorageIterator*       reader,
                                          mqbi::StorageIterator*       start,
                                          const mqbi::StorageIterator* end)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(delay);
    BSLS_ASSERT_SAFE(reader);
    BSLS_ASSERT_SAFE(start);
    BSLS_ASSERT_SAFE(end);

    // deliver everything up to the 'end'

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasConsumers())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    size_t numMessages = processDeliveryLists(delay, reader);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_redeliveryList.size())) {
        // We only attempt to deliver new messages if we successfully
        // redelivered all messages in the redelivery list.
        return numMessages;  // RETURN
    }

    // Deliver messages until either:
    //   1. End of storage; or
    //   2. subStream's capacity is saturated
    //   3. 'storageIter == end'
    //
    // 'end' is never CONFIRMed, so the 'VirtualStorageIterator' cannot skip it

    d_resumePoint = bmqt::MessageGUID();
    while (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(start->hasReceipt())) {
        if (!end->atEnd()) {
            if (start->guid() == end->guid()) {
                // Deliver the rest by 'QueueEngineUtil_AppsDeliveryContext'
                break;
            }
        }

        Routers::Result result = Routers::e_SUCCESS;

        if (QueueEngineUtil::isBroadcastMode(d_queue_p)) {
            // No checking the state for broadcast
            broadcastOneMessage(start);
        }
        else {
            result = tryDeliverOneMessage(delay, start, false);

            if (result == Routers::e_SUCCESS) {
                reportStats(start);

                ++numMessages;
            }
            else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                         result == Routers::e_NO_CAPACITY ||
                         result == Routers::e_NO_SUBSCRIPTION)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                d_putAsideList.add(start->guid());
                // Do not block other Subscriptions. Continue.
            }
            else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                         result == Routers::e_NO_CAPACITY_ALL)) {
                d_resumePoint = start->guid();
                break;
            }
            else {
                BSLS_ASSERT_SAFE(result == Routers::e_INVALID);
                // The {GUID, App} is not valid anymore
            }
        }

        start->advance();
    }
    return numMessages;
}

Routers::Result QueueEngineUtil_AppState::tryDeliverOneMessage(
    bsls::TimeInterval*          delay,
    const mqbi::StorageIterator* message,
    bool                         isOutOfOrder)
{
    BSLS_ASSERT_SAFE(message);

    const mqbi::AppMessage& appView = message->appMessageView(ordinal());
    if (!appView.isPending()) {
        return Routers::e_INVALID;  // RETURN
    }

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
    bsls::TimeInterval now = bmqsys::Time::nowMonotonicClock();
    Visitor            visitor;

    Routers::Result result = Routers::e_SUCCESS;

    if (!QueueEngineUtil::loadMessageDelay(appView.d_rdaInfo,
                                           d_queue_p->messageThrottleConfig(),
                                           &messageDelay)) {
        result = selectConsumer(bdlf::BindUtil::bind(&Visitor::oneConsumer,
                                                     &visitor,
                                                     bdlf::PlaceHolders::_1),
                                message,
                                ordinal());
        // RelayQueueEngine_VirtualPushStorageIterator ignores ordinal
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
                           message->appMessageView(ordinal()).d_rdaInfo));
    visitor.d_handle->deliverMessage(message->appData(),
                                     message->guid(),
                                     message->attributes(),
                                     "",  // msgGroupId
                                     subQueueInfos,
                                     isOutOfOrder);

    visitor.d_consumer->d_timeLastMessageSent = now;
    visitor.d_consumer->d_lastSentMessage     = message->guid();

    return result;
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
QueueEngineUtil_AppState::processDeliveryLists(bsls::TimeInterval*    delay,
                                               mqbi::StorageIterator* reader)
{
    BSLS_ASSERT_SAFE(delay);

    size_t numMessages = processDeliveryList(delay, reader, d_redeliveryList);
    if (*delay == bsls::TimeInterval()) {
        // The only excuse for stopping the iteration is poisonous message
        numMessages += processDeliveryList(delay, reader, d_putAsideList);
    }
    return numMessages;
}

size_t
QueueEngineUtil_AppState::processDeliveryList(bsls::TimeInterval*    delay,
                                              mqbi::StorageIterator* reader,
                                              RedeliveryList&        list)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(list.empty())) {
        return 0;  // RETURN
    }

    // For each reader in the pending redelivery list
    RedeliveryList::iterator it          = list.begin();
    bmqt::MessageGUID        firstGuid   = *it;
    size_t                   numMessages = 0;

    while (!list.isEnd(it)) {
        Routers::Result result = Routers::e_INVALID;

        // Retrieve message from the storage
        reader->reset(*it);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(reader->atEnd())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // The message got gc'ed or purged
            BMQ_LOGTHROTTLE_INFO()
                << "#STORAGE_UNKNOWN_MESSAGE " << "Queue: '"
                << d_queue_p->description() << "', app: '" << appId()
                << "' could not redeliver GUID: '" << *it
                << "' (not in the storage)";
        }
        else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                     !reader->appMessageView(ordinal()).isPending())) {
            BMQ_LOGTHROTTLE_INFO()
                << "#STORAGE_UNKNOWN_MESSAGE " << "Queue: '"
                << d_queue_p->description() << "', app: '" << appId()
                << "' could not redeliver GUID: '" << *it << "' (wrong state "
                << reader->appMessageView(ordinal()).d_state << ")";
        }
        else {
            result = tryDeliverOneMessage(delay, reader, true);
        }
        // TEMPORARILY handling unknown 'Group's in RelayQE by reevaluating.
        // Instead, should communicate them upstream either in CloseQueue or in
        // Rejects.

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
            reportStats(reader);
        }
        else if (result == Routers::e_INVALID) {
            // Couldn't send it, but will never be able to do so; just consider
            // it as sent.
            // Remove from the redeliveryList
            it = list.erase(it);
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
                      << "', appId = '" << appId() << "' (re)delivered "
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

void QueueEngineUtil_AppState::undoRouting()
{
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
    undoRouting();

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
    // Redistribute messages: append all pending messages to
    // the redelivery list.

    int numMsgs = handle->transferUnconfirmedMessageGUID(
        bdlf::BindUtil::bind(&RedeliveryList::add,
                             &d_redeliveryList,
                             bdlf::PlaceHolders::_1),
        subQueue.subId());

    // We lost a reader, try to redeliver any potential messages
    // that need redelivery.
    if (numMsgs) {
        BALL_LOG_INFO << "Lost a reader for queue '"
                      << d_queue_p->description() << "' " << appId()
                      << ", redelivering " << numMsgs << " message(s) to "
                      << consumers().size()
                      << " remaining readers starting from "
                      << *d_redeliveryList.begin();
    }
    else {
        BALL_LOG_INFO << "Lost a reader for queue '"
                      << d_queue_p->description() << "' " << appId()
                      << ", nothing to redeliver.";
    }
    return hasConsumers();
}

Routers::Result QueueEngineUtil_AppState::selectConsumer(
    const Routers::Visitor&      visitor,
    const mqbi::StorageIterator* currentMessage,
    unsigned int                 ordinal)
{
    unsigned int sId =
        currentMessage->appMessageView(ordinal).d_subscriptionId;

    Routers::Result result = d_routing_sp->selectConsumer(visitor,
                                                          currentMessage,
                                                          sId);
    if (result == Routers::e_NO_CAPACITY_ALL) {
        if (d_throttledEarlyExits.requestPermission()) {
            BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queue_p->description()
                          << "', appId = '" << appId()
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

void QueueEngineUtil_AppState::authorize(const mqbu::StorageKey& appKey,
                                         unsigned int            appOrdinal)
{
    BSLS_ASSERT_SAFE(d_queue_p->storage());

    d_appKey       = appKey;
    d_appOrdinal   = appOrdinal;
    d_isAuthorized = true;
}

bool QueueEngineUtil_AppState::authorize()
{
    BSLS_ASSERT_SAFE(d_queue_p->storage());

    unsigned int     ordinal = 0;
    mqbu::StorageKey appKey;

    const bool hasVirtualStorage =
        d_queue_p->storage()->hasVirtualStorage(appId(), &appKey, &ordinal);
    if (hasVirtualStorage) {
        d_appKey       = appKey;
        d_appOrdinal   = ordinal;
        d_isAuthorized = true;

        return true;  // RETURN
    }
    return false;
}

void QueueEngineUtil_AppState::unauthorize()
{
    BSLS_ASSERT_SAFE(d_queue_p->storage());

    // Keep the d_appKey
    d_appOrdinal   = mqbi::Storage::k_INVALID_ORDINAL;
    d_isAuthorized = false;

    clear();
}

void QueueEngineUtil_AppState::clear()
{
    d_resumePoint = bmqt::MessageGUID();
    d_redeliveryList.clear();
    d_putAsideList.clear();
}

void QueueEngineUtil_AppState::loadInternals(mqbcmd::AppState* out) const
{
    out->appId()                = appId();
    out->numConsumers()         = d_routing_sp->d_consumers.size();
    out->redeliveryListLength() = d_redeliveryList.size();
    d_routing_sp->loadInternals(&out->roundRobinRouter());
}

void QueueEngineUtil_AppState::reportStats(
    const mqbi::StorageIterator* message) const
{
    if (bmqp::QueueId::k_PRIMARY_QUEUE_ID == d_queue_p->id()) {
        const bsls::Types::Int64 timeDelta = getMessageQueueTime(
            message->attributes());

        // First report 'queue time' metric for the entire queue
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_QUEUE_TIME,
            timeDelta);

        // Then report 'queue time' metric for appId
        d_queue_p->stats()->onEvent(
            mqbstat::QueueStatsDomain::EventType::e_QUEUE_TIME,
            timeDelta,
            appId());
    }
}

}  // close namespace mqbblp
}  // close namespace BloombergLP
