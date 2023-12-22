// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbblp_relayqueueengine.cpp                                        -*-C++-*-
#include <mqbblp_relayqueueengine.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandle.h>
#include <mqbblp_queuestate.h>
#include <mqbblp_storagemanager.h>
#include <mqbcmd_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>
#include <mqbs_inmemorystorage.h>
#include <mqbs_virtualstoragecatalog.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_weakmemfn.h>

// BDE
#include <ball_logthrottle.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;
// Time interval between messages logged with throttling.

#define BMQ_LOGTHROTTLE_INFO()                                                \
    BALL_LOGTHROTTLE_INFO(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)           \
        << "[THROTTLED] "

#define BMQ_LOGTHROTTLE_ERROR()                                               \
    BALL_LOGTHROTTLE_ERROR(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)          \
        << "[THROTTLED] "

// ====================
// class LimitedPrinter
// ====================

/// An utility class for limiting printing of large objects to a stream.
class LimitedPrinter {
  private:
    // DATA
    const bsl::size_t  d_maxPrintBytes;
    bmqu::MemOutStream d_out;

    // FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const LimitedPrinter& rhs)
    {
        if (rhs.d_out.length() > rhs.d_maxPrintBytes) {
            return stream << rhs.d_out.str().substr(0, rhs.d_maxPrintBytes)
                          << " ... skipped "
                          << rhs.d_out.length() - rhs.d_maxPrintBytes
                          << " bytes";
        }
        else {
            return stream << rhs.d_out.str();
        }
    }

  public:
    // CREATORS
    template <class PRINTABLE>
    LimitedPrinter(const PRINTABLE&  printable,
                   bsl::size_t       maxPrintBytes = 2048,
                   bslma::Allocator* allocator     = 0)
    : d_maxPrintBytes(maxPrintBytes)
    , d_out(allocator)
    {
        d_out << printable;
    }

  private:
    // NOT IMPLEMENTED
    LimitedPrinter(const LimitedPrinter&) BSLS_KEYWORD_DELETED;

    /// Copy constructor and assignment operator removed.
    LimitedPrinter& operator=(const LimitedPrinter&) BSLS_KEYWORD_DELETED;
};

}  // close unnamed namespace

// ==================================
// class RelayQueueEngine::AutoPurger
// ==================================

/// A guard class to remember to clean-up after `deliverMessages()` if we're
/// in broadcast mode.
class RelayQueueEngine::AutoPurger {
  private:
    // DATE
    RelayQueueEngine& d_relayQueueEngine;

  public:
    // CREATORS

    /// Create a `AutoPurger` using the specified `relayQueueEngine`.
    AutoPurger(RelayQueueEngine& relayQueueEngine)
    : d_relayQueueEngine(relayQueueEngine)
    {
    }

    ~AutoPurger()
    {
        // executed by the *DISPATCHER* thread
        // PRECONDITIONS
        QueueState* qs = d_relayQueueEngine.d_queueState_p;
        BSLS_ASSERT_SAFE(
            qs->queue()->dispatcher()->inDispatcherThread(qs->queue()));

        if (qs->isAtMostOnce()) {
            // We don't want to wait for confirmations in broadcast mode.
            // Clear the storage.
            qs->storage()->removeAll(mqbu::StorageKey::k_NULL_KEY);
            d_relayQueueEngine.afterQueuePurged(
                bmqp::ProtocolUtil::k_NULL_APP_ID,
                mqbu::StorageKey::k_NULL_KEY);
            d_relayQueueEngine.d_storageIter_mp->reset();
        }
    }
};

// ----------------------
// class RelayQueueEngine
// ----------------------

// PRIVATE CLASS METHODS
void RelayQueueEngine::onHandleCreation(void* ptr, void* cookie)
{
    // executed by the *DISPATCHER* thread

    RelayQueueEngine* engine        = static_cast<RelayQueueEngine*>(ptr);
    const bool        handleCreated = *static_cast<bool*>(cookie);
    QueueState*       qs            = engine->d_queueState_p;
    mqbi::Queue*      queue         = qs->queue();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue->dispatcher()->inDispatcherThread(queue));

    queue->domain()->cluster()->onQueueHandleCreated(queue,
                                                     queue->uri(),
                                                     handleCreated);
}

// PRIVATE MANIPULATORS
void RelayQueueEngine::onHandleConfigured(
    const bsl::weak_ptr<RelayQueueEngine>&   self,
    const bmqp_ctrlmsg::Status&              status,
    const bmqp_ctrlmsg::StreamParameters&    upStreamParameters,
    mqbi::QueueHandle*                       handle,
    const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
    const bsl::shared_ptr<ConfigureContext>& context)
{
    // executed by *ANY* thread

    bsl::shared_ptr<RelayQueueEngine> strongSelf = self.lock();
    if (!strongSelf) {
        // The engine was destroyed.
        return;  // RETURN
    }

    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&RelayQueueEngine::onHandleConfiguredDispatched,
                             this,
                             d_self.acquireWeak(),
                             status,
                             upStreamParameters,
                             handle,
                             downStreamParameters,
                             context),
        d_queueState_p->queue());

    // 'onHandleConfiguredDispatched' is now responsible for calling the
    // callback: either 'ClusterQueueHelper::onHandleConfigured' or
    // 'ClientSession::onHandleConfigured'.  Neither one requires queue thread.
}

void RelayQueueEngine::onHandleConfiguredDispatched(
    const bsl::weak_ptr<RelayQueueEngine>&   self,
    const bmqp_ctrlmsg::Status&              status,
    BSLS_ANNOTATION_UNUSED const             bmqp_ctrlmsg::StreamParameters&
                                             upStreamParameters,
    mqbi::QueueHandle*                       handle,
    const bmqp_ctrlmsg::StreamParameters&    downStreamParameters,
    const bsl::shared_ptr<ConfigureContext>& context)
{
    // executed by the *QUEUE DISPATCHER* thread

    bsl::shared_ptr<RelayQueueEngine> strongSelf = self.lock();
    if (!strongSelf) {
        // The engine was destroyed.
        return;  // RETURN
    }

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(context);

    // Force re-delivery
    deliverMessages();

    // RelayQueueEngine now assumes that configureQueue request cannot fail.
    // Even if request fails due to some reason (timeout, upstream crashing or
    // rejecting request, etc), ClusterQueueHelper at self node intercepts
    // *most* of these error responses, and simply forwards success status to
    // relay queue engine.  See 'ClusterQueueHelper::onConfigureQueueResponse'
    // for some reasoning behind this.  We assume success, and simply ignore
    // 'status'.

    const bool handleExists = d_queueState_p->handleCatalog().hasHandle(
        handle);

    if (!handleExists) {
        // TBD: handle does not exist anymore.. is this possible?  Send a new
        // (updated) configure request upstream, to effectively rollback this
        // request? With correct open/close queue sequence this scenario should
        // not be possible, but otherwise not sure how exactly to handle this
        // case.
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Received 'configure-stream' response for a handle which does "
            << "not exist in handle catalog for queue '"
            << d_queueState_p->uri() << "'. Sending success downstream.";

        // This happens when handle double closed.  For example, StopRequest
        // processing is in progress (de-configure response is pending) and
        // then the client disconnects.  Some requests could get short-cicuited
        // and handle deleted before de-confgiure response.
        // Need to erase routing associated with this handle.  The handle may
        // not have SubStreamInfo anymore, so search for affected apps.
        for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
            App_State& app(*it->second);
            app.invalidate(handle);
        }
        return;  // RETURN
    }

    BALL_LOGTHROTTLE_INFO_BLOCK(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
    {
        if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS == status.category()) {
            BALL_LOG_INFO
                << "[THROTTLED] Received success 'configure-stream' response"
                << " for handle [" << handle << "] for queue ["
                << d_queueState_p->uri() << "], for parameters "
                << downStreamParameters;
        }
        else {
            BALL_LOG_WARN
                << "[THROTTLED] #QUEUE_CONFIGURE_FAILURE "
                << "Received failed 'configure-stream' response for handle '"
                << handle->client() << ":" << handle->id() << "' for queue '"
                << d_queueState_p->uri() << "', for parameters "
                << downStreamParameters << ", but assuming success.";
        }

        mqbcmd::RoundRobinRouter outrr(d_allocator_p);
        context->d_routing_sp->loadInternals(&outrr);

        BALL_LOG_OUTPUT_STREAM << "[THROTTLED] For queue ["
                               << d_queueState_p->uri()
                               << "] new routing will be "
                               << LimitedPrinter(outrr, 2048, d_allocator_p);
    }

    const bsl::string& appId = downStreamParameters.appId();

    mqbi::QueueHandle::SubStreams::const_iterator it =
        handle->subStreamInfos().find(appId);
    BSLS_ASSERT_SAFE(it != handle->subStreamInfos().end());

    unsigned int upstreamSubQueueId = it->second.d_upstreamSubQueueId;

    // Update handle's stream parameters now.
    handle->setStreamParameters(downStreamParameters);

    // Rebuild consumers for routing
    const mqbi::QueueCounts& counts = it->second.d_counts;
    App_State*               app    = 0;
    if (counts.d_readCount > 0) {
        app = findApp(upstreamSubQueueId);
        BSLS_ASSERT_SAFE(app);

        applyConfiguration(*app, *context);
    }

    BALL_LOGTHROTTLE_INFO_BLOCK(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
    {
        mqbcmd::QueueEngine outqe(d_allocator_p);
        loadInternals(&outqe);

        BALL_LOG_OUTPUT_STREAM << "[THROTTLED] For queue ["
                               << d_queueState_p->uri()
                               << "], the engine config is "
                               << LimitedPrinter(outqe, 2048, d_allocator_p);
    }

    // Invoke callback sending ConfigureQueue response before PUSHing.
    context->invokeCallback();

    if (app) {
        processAppRedelivery(upstreamSubQueueId, app);
    }
}

void RelayQueueEngine::onHandleReleased(
    const bmqp_ctrlmsg::Status&                                  status,
    mqbi::QueueHandle*                                           handle,
    const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
    const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor)
{
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(bmqu::WeakMemFnUtil::weakMemFn(
                                 &RelayQueueEngine::onHandleReleasedDispatched,
                                 d_self.acquireWeak()),
                             status,
                             handle,
                             hp,
                             proctor),
        d_queueState_p->queue());
}

void RelayQueueEngine::onHandleReleasedDispatched(
    const bmqp_ctrlmsg::Status&                                  status,
    mqbi::QueueHandle*                                           handle,
    const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
    const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    bdlb::ScopeExitAny proctorGuard(
        bdlf::BindUtil::bind(&QueueEngineUtil_ReleaseHandleProctor::release,
                             proctor.get()));

    // RelayQueueEngine now assumes that closeQueue request cannot fail.  Even
    // if request fails due to some reason (timeout, upstream crashing or
    // rejecting request, etc), ClusterQueueHelper at self node intercepts
    // *most* these error responses, and simply forwards success status to
    // relay queue engine.  See 'ClusterQueueHelper::onReleaseQueueResponse'
    // for some reasoning behind this.  We assume success, and simply ignore
    // 'status'.
    const bool handleExists = d_queueState_p->handleCatalog().hasHandle(
        handle);
    if (!handleExists) {
        // TBD: handle does not exist anymore.. is this possible?  Send a new
        // (updated) configure request upstream, to effectively rollback this
        // request?

        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Received 'releaseHandle' response for a handle  which does not"
            << " exist in handle catalog for  queue '" << d_queueState_p->uri()
            << "'.";
        return;  // RETURN
    }

    if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS == status.category()) {
        BALL_LOG_INFO << "Received success 'releaseHandle' response for "
                      << "handle [" << handle->client() << ":" << handle->id()
                      << "] for queue [" << d_queueState_p->uri()
                      << "], for parameters " << hp;
    }
    else {
        BALL_LOG_WARN
            << "#QUEUE_CLOSE_FAILURE "
            << "Received failed 'releaseHandle' response for handle '"
            << handle->client() << ":" << handle->id() << "' for queue '"
            << d_queueState_p->uri() << "', for parameters " << hp
            << ", but assuming success.";
    }
    // Use default initializers for AppId and SubId unless this is fanout
    bmqp_ctrlmsg::SubQueueIdInfo info;

    if (!hp.subIdInfo().isNull()) {
        info = hp.subIdInfo().value();
    }
    mqbi::QueueHandle::SubStreams::const_iterator itStream =
        handle->subStreamInfos().find(info.appId());

    if (itStream == handle->subStreamInfos().end()) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Received 'releaseHandle' response for a subQueue which does"
            << " not exist in the handle catalog for '"
            << d_queueState_p->uri() << "'.";
        return;  // RETURN
    }

    // Update effective upstream parameters now that response from upstream has
    // been received.
    BSLS_ASSERT_SAFE(
        bmqp::QueueUtil::isValidSubset(hp,
                                       d_queueState_p->handleParameters()));
    // Release the handle.
    if (0 != proctor->releaseHandle(handle, hp)) {
        return;  // RETURN
    }
    mqbi::QueueHandleReleaseResult streamResult = proctor->releaseStream(hp);

    unsigned int upstreamSubQueueId = itStream->second.d_upstreamSubQueueId;

    // Look up App_State
    AppsMap::iterator appStateIt = d_apps.find(upstreamSubQueueId);
    BSLS_ASSERT_SAFE(appStateIt != d_apps.end());
    BSLS_ASSERT_SAFE(appStateIt->second);

    App_State* app = appStateIt->second.get();

    if (streamResult.hasNoHandleStreamConsumers()) {
        // There are no more consumers for this subStream.
        // The subStream and its context will be erased from the handle state.

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], unregistering subStream: [subStreamInfo: " << info
                      << "] from handle: [" << handle->client() << ":"
                      << handle->id() << "]";

        if (!bmqp::QueueUtil::isEmpty(itStream->second.d_streamParameters)) {
            // The handle has a valid consumer priority, meaning that a
            // downstream client is attempting to release the consumer portion
            // of the handle without having first configured it to have null
            // streamParameters (i.e. invalid consumerPriority).
            // This is expected in shutdown V2 where we minimize the
            // number of requests.
            BALL_LOG_INFO
                << "For queue [" << d_queueState_p->uri() << "],  received a "
                << "'releaseHandle' releasing the consumer portion of handle "
                << "[id: " << handle->id()
                << ", clientPtr: " << handle->client() << ", ptr: " << handle
                << "] without having first configured the handle to have null "
                << "streamParameters. Handle's parameters are "
                << "[handleParameters: " << handle->handleParameters()
                << ", streamParameters: "
                << itStream->second.d_streamParameters
                << "], and the parameters specified in this 'releaseHandle' "
                << "request are [handleParameters: " << hp << ", isFinal "
                << bsl::boolalpha << proctor->d_isFinal << "]";

            // We need to set null streamParameters on the handle's subStream
            // so as to mimic the effects of a configureQueue with null
            // streamParameters.
            bmqp_ctrlmsg::StreamParameters nullStreamParams;

            nullStreamParams.appId() = info.appId();

            handle->setStreamParameters(nullStreamParams);
        }

        app->invalidate(handle);

        // This is in continuation of the special-case handling above.  If the
        // client is attempting to release the consumer portion of an *active*
        // (highest priority) consumer handle without having first configured
        // it to have null streamParameters (i.e. invalid consumerPriority).
        // Then, we need to rebuild the highest priority state so as to "mimic"
        // the effects of a configureQueue with null streamParameters.  Note
        // that this may affect the 'd_queueState_p->streamParameters()'.

        if (app->transferUnconfirmedMessages(handle, info)) {
            processAppRedelivery(upstreamSubQueueId, app);
        }
        else {
            // We lost the last reader.
            //
            // Messages to be delivered downstream need to be cleared from
            // 'd_subStreamMessages_p' for all subIds (since we have lost all
            // readers), because those messages may be re-routed by the primary
            // to another client.  Also get rid of any pending and
            // to-be-redelivered messages.
            beforeOneAppRemoved(upstreamSubQueueId);
            d_pushStream.removeApp(upstreamSubQueueId);
            app->clear();
        }
        if (streamResult.hasNoHandleStreamProducers()) {
            // Remove the producer handle from cached producers/consumers
            app->d_cache.erase(handle);
        }
    }

    if (streamResult.isQueueStreamEmpty()) {
        unsigned int numMessages = d_pushStream.removeApp(upstreamSubQueueId);

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], removing App for appId: [" << info.appId()
                      << "] and virtual storage associated with"
                      << " upstreamSubQueueId: [" << upstreamSubQueueId << "]"
                      << " with " << numMessages << " messages";

        BSLS_ASSERT_SAFE(app->d_cache.empty());

        // Remove and delete empty App_State
        d_apps.erase(appStateIt);
        d_appIds.erase(app->appId());
        d_queueState_p->removeUpstreamParameters(upstreamSubQueueId);
        d_queueState_p->abandon(upstreamSubQueueId);
    }

    handle->unregisterSubStream(info,
                                mqbi::QueueCounts(hp.readCount(),
                                                  hp.writeCount()),
                                proctor->result().hasNoHandleClients());

    // All done, invoke the clientCb if one was provided
}

void RelayQueueEngine::deliverMessages()
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // Auto-purge broadcast storage on exit.
    AutoPurger onExit(*this);

    // The guard MUST be created before the no consumers check.  This is
    // because in broadcast mode we want to purge any unused messages even if
    // there are no consumers.  Failure to do so would lead to keeping messages
    // and delivering them when a new consumers arrive.

    // Deliver messages until either:
    //   1. End of storage; or
    //   2. All subStreams return 'e_NO_CAPACITY_ALL'

    while (d_appsDeliveryContext.reset(d_storageIter_mp.get())) {
        // Assume, all Apps need to deliver (some may be at capacity)
        unsigned int numApps = d_storageIter_mp->numApps();

        for (unsigned int i = 0; i < numApps; ++i) {
            const PushStream::Element* element = d_storageIter_mp->element(i);

            App_State* app = element->app().d_app.get();

            BSLS_ASSERT_SAFE(app);

            if (!app->isAuthorized()) {
                // This App got the PUSH (recorded in the PushStream)
                BMQ_LOGTHROTTLE_ERROR()
                    << "#NOT AUTHORIZED "
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id()
                    << ") discarding a PUSH message for guid "
                    << d_storageIter_mp->guid() << ", with NOT AUTHORIZED App "
                    << app->appId();

                d_storageIter_mp->removeCurrentElement();
            }
            else if (element->app().isLastPush(
                         d_storageIter_mp->guid(),
                         d_appsDeliveryContext.revCounter())) {
                // This `app` has already seen this message.

                BMQ_LOGTHROTTLE_INFO()
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id() << ", App '"
                    << app->appId()
                    << "') discarding a duplicate PUSH for guid "
                    << d_storageIter_mp->guid();

                d_storageIter_mp->removeCurrentElement();
            }
            else {
                element->app().setLastPush(d_storageIter_mp->guid(),
                                           d_appsDeliveryContext.revCounter());

                if (d_appsDeliveryContext.processApp(*app, i, true)) {
                    // The current element has made it either to delivery or
                    // putAside and it can be removed
                    d_storageIter_mp->removeCurrentElement();
                }
            }
        }
        d_appsDeliveryContext.deliverMessage();
    }
}

void RelayQueueEngine::processAppRedelivery(unsigned int upstreamSubQueueId,
                                            App_State*   app)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // Position to the last 'Routers::e_NO_CAPACITY_ALL' point
    bslma::ManagedPtr<PushStreamIterator> storageIter_mp;
    PushStreamIterator*                   start = 0;

    if (app->resumePoint().isUnset()) {
        start = d_storageIter_mp.get();
    }
    else {
        PushStream::iterator it = d_pushStream.d_stream.find(
            app->resumePoint());

        if (it == d_pushStream.d_stream.end()) {
            // The message is gone because of purge
            // Start at the beginning
            it = d_pushStream.d_stream.begin();
        }

        storageIter_mp.load(new (*d_allocator_p)
                                VirtualPushStreamIterator(upstreamSubQueueId,
                                                          storage(),
                                                          &d_pushStream,
                                                          it),
                            d_allocator_p);

        start = storageIter_mp.get();
    }

    bsls::TimeInterval delay;
    app->deliverMessages(&delay,
                         d_realStorageIter_mp.get(),
                         start,
                         d_storageIter_mp.get());

    if (delay != bsls::TimeInterval()) {
        app->scheduleThrottle(
            bmqsys::Time::nowMonotonicClock() + delay,
            bdlf::BindUtil::bind(&RelayQueueEngine::processAppRedelivery,
                                 this,
                                 upstreamSubQueueId,
                                 app));
    }

    if (app->isReadyForDelivery()) {
        // can continue delivering
        const bmqt::MessageGUID dummy;
        afterNewMessage(dummy, 0);
    }
}

void RelayQueueEngine::configureApp(
    App_State&                               appState,
    mqbi::QueueHandle*                       handle,
    const bmqp_ctrlmsg::StreamParameters&    streamParameters,
    const bsl::shared_ptr<ConfigureContext>& context)
{
    // Update handle's 'upstream/outstanding' stream parameters.  Note that
    // self node has to send a configure request upstream.  RelayQueueEngine
    // needs to determine if need to send a 'configure' request upstream. It is
    // assumed (and in fact, part of configure-request workflow) that this
    // request will not fail.  So, we update the 'effective' stream parameters
    // before sending the request but update the stream parameters of the
    // specified 'handle' only in the response callback.

    App_State::CachedParametersMap::iterator itHandle(
        appState.d_cache.find(handle));

    if (itHandle->second.d_streamParameters == streamParameters) {
        // Last advertised stream parameters for this handle are same as
        // the newly advertised ones.  No need to send any notification
        // upstream.
        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], last advertised stream parameter by handle ["
                      << handle->id() << "] were same as newly advertised "
                      << "ones: " << streamParameters
                      << ". Not sending configure-queue request upstream, but "
                      << "returning success to downstream client.";

        return;  // RETURN
    }
    else {
        // Update handle's last advertised stream parameters
        itHandle->second.d_streamParameters = streamParameters;
    }

    // Rebuild effective stream parameters advertised to upstream, based on the
    // aggregate stream parameters across all consumers in the
    // 'upstreamSubQueueId' having the highest priority advertised so far.
    bmqp_ctrlmsg::StreamParameters previousParameters;
    unsigned int upstreamSubQueueId = appState.upstreamSubQueueId();
    bool         hadParameters      = d_queueState_p->getUpstreamParameters(
        &previousParameters,
        upstreamSubQueueId);

    BMQ_LOGTHROTTLE_INFO()
        << "For queue '" << d_queueState_p->uri()
        << "', about to rebuild upstream state [current stream parameters: "
        << previousParameters << "]";

    rebuildUpstreamState(context->d_routing_sp.get(),
                         &appState,
                         upstreamSubQueueId,
                         streamParameters.appId());

    // Update effective upstream parameters if specified 'handle' is a reader
    bmqp_ctrlmsg::StreamParameters streamParamsToSend;
    bool hasStreamParamsToSend = d_queueState_p->getUpstreamParameters(
        &streamParamsToSend,
        upstreamSubQueueId);

    BSLS_ASSERT_SAFE(hasStreamParamsToSend);
    (void)hasStreamParamsToSend;

    if (hadParameters && previousParameters == streamParamsToSend) {
        // Last advertised stream parameters for this queue are same as the
        // newly advertised ones.  No need to send any notification upstream.

        BMQ_LOGTHROTTLE_INFO()
            << "For queue [" << d_queueState_p->uri()
            << "], last advertised stream parameter by the queue"
            << " were same as newly advertised ones: " << previousParameters
            << ". Not sending configure-queue request upstream, but"
            << " returning success to downstream client.";

        // Set the parameters and inform downstream client of success
        handle->setStreamParameters(streamParameters);

        applyConfiguration(appState, *context);
        return;  // RETURN
    }

    // Send a configure stream request upstream.
    d_queueState_p->domain()->cluster()->configureQueue(
        d_queueState_p->queue(),
        streamParamsToSend,
        appState.upstreamSubQueueId(),  // upstream subQueueId
        bdlf::BindUtil::bind(&RelayQueueEngine::onHandleConfigured,
                             this,
                             d_self.acquireWeak(),
                             bdlf::PlaceHolders::_1,  // status
                             bdlf::PlaceHolders::_2,  // upStreamParameters
                             handle,
                             streamParameters,  // downStreamParameters
                             context));

    // 'onHandleConfigured' is now responsible for calling the callback
}

void RelayQueueEngine::rebuildUpstreamState(Routers::AppContext* context,
                                            App_State*           appState,
                                            unsigned int upstreamSubQueueId,
                                            const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    BSLS_ASSERT_SAFE(!appState->d_cache.empty());

    bmqp_ctrlmsg::StreamParameters upstreamParams;

    context->reset();

    for (App_State::CachedParametersMap::iterator iter =
             appState->d_cache.begin();
         iter != appState->d_cache.end();
         ++iter) {
        const bmqp_ctrlmsg::StreamParameters& streamParameters =
            iter->second.d_streamParameters;
        bmqu::MemOutStream errorStream(d_allocator_p);
        context->load(iter->first,
                      &errorStream,
                      iter->second.d_downstreamSubQueueId,
                      upstreamSubQueueId,
                      streamParameters,
                      appState->routing().get());

        if (errorStream.length() > 0) {
            BALL_LOG_WARN << "#BMQ_SUBSCRIPTION_FAILURE for queue '"
                          << d_queueState_p->uri()
                          << "', error rebuilding routing [stream parameters: "
                          << streamParameters << "]: [" << errorStream.str()
                          << " ]";
        }
    }
    context->finalize();

    context->generate(&upstreamParams);

    upstreamParams.appId() = appId;
    // Items in 'd_cache' contain downstream 'subId', replace it with the
    // upstream one for 'setUpstreamParameters'.

    d_queueState_p->setUpstreamParameters(upstreamParams, upstreamSubQueueId);

    BMQ_LOGTHROTTLE_INFO()
        << "For queue '" << d_queueState_p->uri()
        << "', rebuilt upstream parameters [new upstream parameters: "
        << upstreamParams << "]";
}

void RelayQueueEngine::applyConfiguration(App_State&        app,
                                          ConfigureContext& context)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    app.undoRouting();

    app.routing() = context.d_routing_sp;

    if (!d_queueState_p->isDeliverConsumerPriority()) {
        if (d_queueState_p->hasMultipleSubStreams()) {
            BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                           << "Fanout queue '" << d_queueState_p->uri()
                           << "' does not have priority configuration.";

            // One handle can be represented as priority routing group in
            // backward compatible manner.
            BSLS_ASSERT_SAFE(1 == app.d_cache.size() &&
                             "This version expects priority support for"
                             " fanout queues");
        }
        else {
            BSLS_ASSERT_SAFE(
                false && "This version expects either priority or broadcast"
                         " queues");
        }
    }

    app.routing()->apply();

    Routers::Consumers& consumers = app.routing()->d_consumers;

    for (Routers::Consumers::const_iterator itConsumer = consumers.begin();
         itConsumer != consumers.end();
         ++itConsumer) {
        Routers::Consumer& consumer = consumers.value(itConsumer);
        mqbi::QueueHandle* handle   = itConsumer->first;

        if (!d_queueState_p->handleCatalog().hasHandle(handle)) {
            // This can happen with out-of-order Configure responses as in the
            // case when network disconnects with two concurrent requests and
            // the second gets (error) response before the first one gets
            // cancelled.
            itConsumer->second.lock()->invalidate();

            app.routing()->finalize();

            continue;  // CONTINUE
        }

        consumer.registerSubscriptions(handle);
    }

    BALL_LOG_INFO << "For queue '" << d_queueState_p->uri() << "', "
                  << "rebuilt highest priority consumers: [count: "
                  << app.consumers().size() << "]";
}

// CREATORS
RelayQueueEngine::RelayQueueEngine(QueueState*             queueState,
                                   const mqbconfm::Domain& domainConfig,
                                   bslma::Allocator*       allocator)
: d_queueState_p(queueState)
, d_pushStream(queueState->pushElementsPool(), allocator)
, d_domainConfig(domainConfig, allocator)
, d_apps(allocator)
, d_appIds(allocator)
, d_self(this)  // use default allocator
, d_appsDeliveryContext(d_queueState_p->queue(), allocator)
, d_storageIter_mp()
, d_realStorageIter_mp()
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueState);
    BSLS_ASSERT_SAFE(queueState->queue());
    BSLS_ASSERT_SAFE(queueState->storage());

    d_throttledRejectedMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 5 seconds

    resetState();  // Ensure 'resetState' is doing what is expected, similarly
                   // to the constructor.
}

RelayQueueEngine::~RelayQueueEngine()
{
    // PRECONDITIONS
    BSLS_REVIEW_SAFE(d_apps.empty());

    d_self.invalidate();
}

// MANIPULATORS
int RelayQueueEngine::configure(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    BSLS_ANNOTATION_UNUSED bool          isReconfigure)
{
    return 0;
}

void RelayQueueEngine::resetState(bool isShuttingDown)
{
    for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
        it->second->undoRouting();
        if (isShuttingDown) {
            it->second->routing()->reset();
        }
        // else, keep the routing which new engine can reuse
    }
    if (!isShuttingDown) {
        d_apps.clear();
    }

    d_realStorageIter_mp = storage()->getIterator(
        mqbu::StorageKey::k_NULL_KEY);

    d_storageIter_mp.load(
        new (*d_allocator_p) PushStreamIterator(storage(),
                                                &d_pushStream,
                                                d_pushStream.d_stream.begin()),
        d_allocator_p);
}

int RelayQueueEngine::rebuildInternalState(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    BSLS_ASSERT_OPT(false && "should never be invoked");
    return 0;
}

mqbi::QueueHandle* RelayQueueEngine::getHandle(
    const mqbi::OpenQueueConfirmationCookie&                  context,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

#define CALLBACK(CAT, RC, MSG, HAN)                                           \
    if (callback) {                                                           \
        bmqp_ctrlmsg::Status status(d_allocator_p);                           \
        status.category() = CAT;                                              \
        status.code()     = RC;                                               \
        status.message()  = MSG;                                              \
        callback(status, HAN);                                                \
    }

    bool handleCreated = false;

    // Create a proctor which will notify the cluster in its destructor whether
    // a new queue handle was created or not, so that cluster can keep track of
    // total handles created for a given queue.
    bslma::ManagedPtr<RelayQueueEngine> proctor(this,                // ptr
                                                &handleCreated,      // cookie
                                                &onHandleCreation);  // deleter
    if (!bmqp::QueueUtil::isValid(handleParameters)) {
        CALLBACK(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                 -1,
                 "Invalid handle parameters specified",
                 0);
        return 0;  // RETURN
    }

    mqbi::QueueHandle* queueHandle =
        d_queueState_p->handleCatalog().getHandleByRequester(
            *clientContext,
            handleParameters.qId());
    if (queueHandle) {
        // Already aware of this queueId from this client.
        if (QueueEngineUtil::validateUri(handleParameters,
                                         queueHandle,
                                         clientContext.get()) != 0) {
            CALLBACK(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                     -1,
                     "Queue URI mismatch for same queueId.",
                     0);
            return 0;  // RETURN
        }

        // Update the current handle parameters.
        bmqp_ctrlmsg::QueueHandleParameters currentHandleParameters =
            queueHandle->handleParameters();

        bmqp::QueueUtil::mergeHandleParameters(&currentHandleParameters,
                                               handleParameters);
        queueHandle->setHandleParameters(currentHandleParameters);

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], reconfigured existing handle [client: "
                      << clientContext->description()
                      << ", requesterId: " << clientContext->requesterId()
                      << ", queueId:" << handleParameters.qId()
                      << ", new handle parameters: " << currentHandleParameters
                      << "]";
    }
    else {
        // This is a new client, we need to create a new handle for it
        queueHandle = d_queueState_p->handleCatalog().createHandle(
            clientContext,
            handleParameters,
            d_queueState_p->stats().get());
        handleCreated = true;

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], created new handle " << queueHandle
                      << " [client: " << clientContext->description()
                      << ", requesterId: " << clientContext->requesterId()
                      << ", queueId:" << handleParameters.qId()
                      << ", handle parameters: " << handleParameters << "].";
    }

    // Update queue's aggregated parameters.
    d_queueState_p->add(handleParameters);

    // Handle parameters must remain a subset of queue's aggregated params.
    BSLS_ASSERT_SAFE(
        bmqp::QueueUtil::isValidSubset(queueHandle->handleParameters(),
                                       d_queueState_p->handleParameters()));
    // The handle may not have 'd_clientContext_sp' in the case when in between
    // outgoing OpenQueue request and incoming OpenQueue response, the client
    // disconnects and calls 'QueueHandle::clearClient'/'clearClientDispatched'
    // Should not be a problem because 'QueueHandle::canDeliver' does check for
    // the client presence.

    const bmqp_ctrlmsg::SubQueueIdInfo& downstreamInfo =
        bmqp::QueueUtil::extractSubQueueInfo(handleParameters);

    App_State* app = findApp(upstreamSubQueueId);
    if (app == 0) {
        // Create and insert new App_State
        // Correlate downstreamInfo with upstream

        AppStateSp appStateSp(new (*d_allocator_p)
                                  App_State(upstreamSubQueueId,
                                            downstreamInfo.appId(),
                                            d_queueState_p->queue(),
                                            d_queueState_p->scheduler(),
                                            d_queueState_p->routingContext(),
                                            d_allocator_p),
                              d_allocator_p);

        app = appStateSp.get();

        d_apps.insert(bsl::make_pair(upstreamSubQueueId, appStateSp));

        d_appIds.insert(bsl::make_pair(app->appId(), appStateSp));

        d_queueState_p->adopt(appStateSp);

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], created App for appId: ["
                      << downstreamInfo.appId()
                      << "] and virtual storage associated with"
                      << " upstreamSubQueueId: [" << upstreamSubQueueId << "]";
    }

    BSLS_ASSERT_SAFE(app);

    if (!app->isAuthorized()) {
        if (app->authorize()) {
            BALL_LOG_INFO << "Queue '" << d_queueState_p->uri()
                          << "' authorized App '" << downstreamInfo.appId()
                          << "' with ordinal " << app->ordinal() << ".";
        }
    }

    mqbi::QueueHandle::SubStreams::const_iterator citSubStream =
        queueHandle->registerSubStream(
            downstreamInfo,
            upstreamSubQueueId,
            mqbi::QueueCounts(handleParameters.readCount(),
                              handleParameters.writeCount()));

    // If a new reader/write, insert its (default-valued) stream parameters
    // into our map of consumer stream parameters advertised upstream.
    bsl::pair<App_State::CachedParametersMap::iterator, bool> insertResult =
        app->d_cache.insert(bsl::make_pair(
            queueHandle,
            RelayQueueEngine_AppState::CachedParameters(handleParameters,
                                                        downstreamInfo.subId(),
                                                        d_allocator_p)));
    if (!insertResult.second) {
        bmqp::QueueUtil::mergeHandleParameters(
            &insertResult.first->second.d_handleParameters,
            handleParameters);
    }

    context->d_stats = citSubStream->second.d_clientStats;

    // Inform the requester of the success
    CALLBACK(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, 0, "", queueHandle);

    return queueHandle;

#undef CALLBACK
}

void RelayQueueEngine::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(handle);

    // The 'context' will mirror streamParameters when calling 'configuredCb'
    bsl::shared_ptr<ConfigureContext> context(
        new (*d_allocator_p)
            ConfigureContext(configuredCb, streamParameters, d_allocator_p),
        d_allocator_p);

    // Verify handle exists
    if (!d_queueState_p->handleCatalog().hasHandle(handle)) {
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "Attempting to configure unknown handle. Queue '"
                       << d_queueState_p->uri()
                       << "', stream params: " << streamParameters
                       << ", handlePtr '" << handle << "'.";

        context->setStatus(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                           -1,
                           "Attempting to configure unknown handle.");
        return;  // RETURN
    }

    // Verify substream exists
    mqbi::QueueHandle::SubStreams::const_iterator it =
        handle->subStreamInfos().find(streamParameters.appId());

    if (it == handle->subStreamInfos().end()) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Attempting to configure unknown substream for the handle. "
            << "Queue '" << d_queueState_p->uri()
            << "', stream params: " << streamParameters << ", handlePtr '"
            << handle << "'.";

        context->setStatus(
            bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
            -1,
            "Attempting to configure unknown substream for the handle.");
        return;  // RETURN
    }

    unsigned int upstreamSubQueueId = it->second.d_upstreamSubQueueId;
    App_State*   app                = findApp(upstreamSubQueueId);
    BSLS_ASSERT_SAFE(app);

    context->initializeRouting(d_queueState_p->routingContext());

    configureApp(*app, handle, streamParameters, context);
}

void RelayQueueEngine::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor> proctor(
        new (*d_allocator_p)
            QueueEngineUtil_ReleaseHandleProctor(d_queueState_p,
                                                 isFinal,
                                                 releasedCb),
        d_allocator_p);

    if (!d_queueState_p->handleCatalog().hasHandle(handle)) {
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "Attempting to release unknown handle. HandlePtr '"
                       << handle << "', queue '" << d_queueState_p->uri()
                       << "'.";

        return;  // RETURN
    }

    if (QueueEngineUtil::validateUri(handleParameters, handle) != 0) {
        return;  // RETURN
    }

    releaseHandleImpl(handle, handleParameters, proctor);
}

void RelayQueueEngine::releaseHandleImpl(
    mqbi::QueueHandle*                                           handle,
    const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
    const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor)
{
    const bsl::string& appId = bmqp::QueueUtil::extractAppId(hp);

    mqbi::QueueHandle::SubStreams::const_iterator it =
        handle->subStreamInfos().find(appId);
    BSLS_ASSERT_SAFE(it != handle->subStreamInfos().end());

    unsigned int upstreamSubQueueId = it->second.d_upstreamSubQueueId;
    App_State*   app                = findApp(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(app);
    App_State::CachedParametersMap::iterator itHandle = app->d_cache.find(
        handle);
    BSLS_ASSERT_SAFE(itHandle != app->d_cache.end());
    bmqp_ctrlmsg::QueueHandleParameters& cachedHandleParameters =
        itHandle->second.d_handleParameters;

    // Validate 'handleParameters'.  If they are invalid, but 'isFinal' flag is
    // true, we still want to honor the 'isFinal' flag, and remove handle from
    // our catalog of handles.  But in order to do so, we need to "fix" the
    // specified invalid 'handleParameters'.  We do so by making a copy of
    // them, and initializing them with our view of the handle parameters, so
    // that any logic related to those parameters executed by us remains valid.
    bmqp_ctrlmsg::QueueHandleParameters effectiveHandleParam(hp);
    if (!bmqp::QueueUtil::isValid(effectiveHandleParam) ||
        !bmqp::QueueUtil::isValidSubset(effectiveHandleParam,
                                        cachedHandleParameters) ||
        !bmqp::QueueUtil::isValidSubset(effectiveHandleParam,
                                        d_queueState_p->handleParameters())) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "For queue '" << d_queueState_p->uri()
            << "', invalid handle parameters specified when attempting to "
            << "release a handle: " << effectiveHandleParam
            << ". Handle's current params: " << cachedHandleParameters
            << ", queue's aggregated params: "
            << d_queueState_p->handleParameters() << ". HandlePtr '" << handle
            << "', id: " << handle->id()
            << ". 'isFinal' flag: " << bsl::boolalpha << proctor->d_isFinal
            << ".";

        if (!proctor->d_isFinal) {
            return;  // RETURN
        }

        // Update effective handle parameters with our view of the handle in
        // order to make their value sane.
        effectiveHandleParam             = cachedHandleParameters;
        effectiveHandleParam.subIdInfo() = hp.subIdInfo();

        if (!bmqp::QueueUtil::isValid(effectiveHandleParam)) {
            // A Close request (for example, StopRequest related) is out, and
            // then downstream disconnects before the response.
            // 'tearDown' requests 'drop' which attempts to send another
            // (excessive) Close request.
            // The excessive request can delete the queue before the response
            // to the first request.
            BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                           << "Ignoring excessive Close request For queue '"
                           << d_queueState_p->uri()
                           << "' because the handle parameters are invalid: "
                           << effectiveHandleParam;
            return;  // RETURN
        }
    }

    // update our view of QueueHandleParameters
    bmqp::QueueUtil::subtractHandleParameters(&cachedHandleParameters,
                                              effectiveHandleParam);

    proctor->addRef();

    // Send a close queue request upstream.
    d_queueState_p->domain()->cluster()->configureQueue(
        d_queueState_p->queue(),
        effectiveHandleParam,
        upstreamSubQueueId,
        bdlf::BindUtil::bind(
            bmqu::WeakMemFnUtil::weakMemFn(&RelayQueueEngine::onHandleReleased,
                                           d_self.acquireWeak()),
            bdlf::PlaceHolders::_1,  // Status
            handle,
            effectiveHandleParam,
            proctor));
}

void RelayQueueEngine::onHandleUsable(mqbi::QueueHandle* handle,
                                      unsigned int upstreamSubscriptionId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(handle);

    // Note that specified 'subQueueId' is the downstream subId.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_queueState_p->handleCatalog().hasHandle(handle))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "Making unknown handle available.";
        BSLS_ASSERT_SAFE(false && "Making unknown handle available.");
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!bmqt::QueueFlagsUtil::isReader(
            handle->handleParameters().flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }
    unsigned int upstreamSubQueueId = 0;

    if (d_queueState_p->routingContext().onUsable(&upstreamSubQueueId,
                                                  upstreamSubscriptionId)) {
        App_State* app = findApp(upstreamSubQueueId);

        BSLS_ASSERT_SAFE(app);
        processAppRedelivery(upstreamSubQueueId, app);
    }
}

void RelayQueueEngine::afterNewMessage(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* source)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    deliverMessages();
}

int RelayQueueEngine::onConfirmMessage(mqbi::QueueHandle*       handle,
                                       const bmqt::MessageGUID& msgGUID,
                                       unsigned int upstreamSubQueueId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_ERROR               = -1,
        rc_NO_MORE_REFERENCES  = 0,
        rc_NON_ZERO_REFERENCES = 1
    };

    // Specified 'subscriptionId' is the downstream one.  Need to calculate
    // corresponding appKey.  This is done by retrieving the upstream subId
    // and using it to create the appKey.
    App_State* app = findApp(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(app);

    app->tryCancelThrottle(handle, msgGUID);

    // If proxy, also inform the physical storage.
    if (d_queueState_p->domain()->cluster()->isRemote()) {
        mqbi::Storage*            storage = d_queueState_p->storage();
        mqbi::StorageResult::Enum rc      = storage->confirm(msgGUID,
                                                        app->appKey(),
                                                        0ULL);
        if (mqbi::StorageResult::e_ZERO_REFERENCES == rc) {
            // Since there are no references, there should be no app holding
            // msgGUID and no need to call `beforeMessageRemoved`

            return rc_NO_MORE_REFERENCES;  // RETURN
        }

        if (mqbi::StorageResult::e_NON_ZERO_REFERENCES == rc) {
            return rc_NON_ZERO_REFERENCES;  // RETURN
        }

        beforeMessageRemoved(msgGUID);
        return rc_ERROR;  // RETURN
    }

    return rc_NON_ZERO_REFERENCES;
}

int RelayQueueEngine::onRejectMessage(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    const bmqt::MessageGUID&                  msgGUID,
    unsigned int                              upstreamSubQueueId)
{
    // Specified 'subQueueId' is the downstream one.  Need to convert it into
    // corresponding appKey.

    // Retrieve upstream subId and use it to create an appKey to reject the
    // message.

    App_State* app = findApp(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(app);

    // Inform the 'app' that 'msgGUID' is about to be removed from its virtual
    // storage, so that app can advance its iterator etc if required.

    int result = 0;

    bslma::ManagedPtr<mqbi::StorageIterator> message;
    mqbi::StorageResult::Enum rc = storage()->getIterator(&message,
                                                          app->appKey(),
                                                          msgGUID);

    if (rc == mqbi::StorageResult::e_SUCCESS) {
        bmqp::RdaInfo& rda =
            message->appMessageState(app->ordinal()).d_rdaInfo;

        result = rda.counter();

        if (d_throttledRejectedMessages.requestPermission()) {
            BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queueState_p->uri()
                          << "' rejecting PUSH [GUID: '" << msgGUID
                          << "', subQueueId: " << app->upstreamSubQueueId()
                          << "] with the counter: [" << rda << "]";
        }

        if (!rda.isUnlimited()) {
            BSLS_ASSERT_SAFE(result);
            rda.setPotentiallyPoisonous(true);
            rda.setCounter(--result);

            if (result == 0) {
                // TODO d_subStreamMessages_p->remove(msgGUID, appKey);

                if (d_queueState_p->domain()->cluster()->isRemote()) {
                    rc =
                        storage()->confirm(msgGUID, app->appKey(), 0ULL, true);
                    if (mqbi::StorageResult::e_ZERO_REFERENCES == rc) {
                        // Since there are no references, there should be no
                        // app holding msgGUID and no need to call
                        // `beforeMessageRemoved`.

                        storage()->remove(msgGUID, 0);
                    }
                }
            }
        }
    }
    else if (d_throttledRejectedMessages.requestPermission()) {
        BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queueState_p->uri()
                      << "' got reject for an unknown message [GUID: '"
                      << msgGUID
                      << "', subQueueId: " << app->upstreamSubQueueId() << "]";
    }

    return result;
}

void RelayQueueEngine::beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (!d_storageIter_mp->atEnd() && (d_storageIter_mp->guid() == msgGUID)) {
        d_storageIter_mp->removeAllElements();

        d_storageIter_mp->advance();
    }
    else {
        PushStreamIterator del(storage(),
                               &d_pushStream,
                               d_pushStream.d_stream.find(msgGUID));

        if (!del.atEnd()) {
            del.removeAllElements();

            del.advance();
        }
    }
}

void RelayQueueEngine::afterQueuePurged(const bsl::string&      appId,
                                        const mqbu::StorageKey& appKey)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // FIXME: This component can be invoked by mqbblp or mqbs components.  When
    // invoked by mqbs components, 'appKey' will be the one used at storage
    // level (in replication etc), and when invoked by mqbblp components,
    // 'appKey' will by constructed by using the upstream subQueueId.
    // Currently, this ambiguity is resolved in a hideous way, as evident from
    // the code.  One way to fix this is to add additional flavor of
    // 'afterQueuePurged' in 'mqbi::QueueEngine' interface, which takes
    // 'unsigned int upstreamSubId'.  Another way is for the relay node to
    // always use storage-level appKeys.  Note that in this approach, it would
    // need to perform an additional lookup upon receiving a PUSH message
    // (upstreamSubId -> appKey).  Also note that in this case, proxy will
    // still continue to construct the appKey using upstreamSubId.

    if (appKey.isNull()) {
        BSLS_ASSERT_SAFE(appId == bmqp::ProtocolUtil::k_NULL_APP_ID);

        // Purge all virtual storages, and reset all iterators.

        d_pushStream.removeAll();

        for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
            it->second->clear();
        }
        d_storageIter_mp->reset();
    }
    else {
        // Purge virtual storage corresponding to the specified 'appKey'.

        AppIds::const_iterator itApp = d_appIds.find(appId);
        if (itApp == d_appIds.end()) {
            BALL_LOG_ERROR << "#QUEUE_STORAGE_NOTFOUND "
                           << "For queue '" << d_queueState_p->uri()
                           << "', queueKey '" << d_queueState_p->key()
                           << "', failed to find virtual storage iterator "
                           << "corresponding to appKey '" << appKey
                           << "' while attempting to purge its virtual "
                           << "storage.";
        }
        else {
            // Clear out virtual storage corresponding to the App.
            beforeOneAppRemoved(itApp->second->upstreamSubQueueId());
            d_pushStream.removeApp(itApp->second->upstreamSubQueueId());

            itApp->second->clear();
        }
    }
}

void RelayQueueEngine::onTimer(
    BSLS_ANNOTATION_UNUSED bsls::Types::Int64 currentTimer)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // NOTHING
}

mqbi::StorageResult::Enum RelayQueueEngine::evaluateAppSubscriptions(
    BSLS_ANNOTATION_UNUSED const bmqp::PutHeader& putHeader,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bmqp::MessagePropertiesInfo& mpi,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 timestamp)
{
    // executed by the *QUEUE DISPATCHER* thread

    BSLS_ASSERT_OPT(false && "should never be invoked");

    // NOTHING
    return mqbi::StorageResult::e_INVALID_OPERATION;
}

void RelayQueueEngine::loadInternals(mqbcmd::QueueEngine* out) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    mqbcmd::RelayQueueEngine& relayQueueEngine = out->makeRelay();

    int                          numSubStreams = 0;
    mqbi::Storage::AppInfos      appIdKeyPairs;

    numSubStreams = storage()->numVirtualStorages();
    storage()->loadVirtualStorageDetails(&appIdKeyPairs);

    BSLS_ASSERT_SAFE(static_cast<int>(appIdKeyPairs.size()) == numSubStreams);

    relayQueueEngine.numSubstreams() = numSubStreams;
    if (numSubStreams) {
        bsl::vector<mqbcmd::RelayQueueEngineSubStream>& subStreams =
            relayQueueEngine.subStreams();
        subStreams.reserve(appIdKeyPairs.size());

        for (mqbi::Storage::AppInfos::const_iterator cit =
                 appIdKeyPairs.cbegin();
             cit != appIdKeyPairs.cend();
             ++cit) {
            subStreams.resize(subStreams.size() + 1);
            mqbcmd::RelayQueueEngineSubStream& subStream = subStreams.back();
            subStream.appId()                            = cit->first;
            bmqu::MemOutStream appKey;
            appKey << cit->second;
            subStream.appKey()      = appKey.str();
            subStream.numMessages() = d_queueState_p->storage()->numMessages(
                cit->second);
        }
    }

    relayQueueEngine.appStates().reserve(d_apps.size());
    for (AppsMap::const_iterator it = d_apps.begin(); it != d_apps.end();
         ++it) {
        relayQueueEngine.appStates().resize(
            relayQueueEngine.appStates().size() + 1);
        mqbcmd::AppState& appState = relayQueueEngine.appStates().back();

        it->second->loadInternals(&appState);
    }

    d_queueState_p->routingContext().loadInternals(
        &relayQueueEngine.routing());
}

void RelayQueueEngine::registerStorage(const bsl::string&      appId,
                                       const mqbu::StorageKey& appKey,
                                       unsigned int            appOrdinal)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    AppIds::iterator iter = d_appIds.find(appId);

    if (iter == d_appIds.end()) {
        // No consumer has opened the queue with 'appId'.

        return;  // RETURN
    }

    // A consumer has already opened the queue with 'appId'.

    BALL_LOG_INFO << "Remote queue: " << d_queueState_p->uri()
                  << " (id: " << d_queueState_p->id()
                  << ") now has storage: [App Id: " << appId
                  << ", key: " << appKey << ", ordinal: " << appOrdinal << "]";

    iter->second->authorize(appKey, appOrdinal);
}

void RelayQueueEngine::unregisterStorage(const bsl::string&      appId,
                                         const mqbu::StorageKey& appKey,
                                         unsigned int            appOrdinal)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    AppIds::iterator iter = d_appIds.find(appId);
    mqbu::StorageKey key;

    if (iter == d_appIds.end()) {
        // No consumer has opened the queue with 'appId'.
    }
    else {
        // A consumer has already opened the queue with 'appId'.
        BSLS_ASSERT_SAFE(iter->second->appKey() == appKey);

        iter->second->unauthorize();
    }

    (void)appOrdinal;
}

bool RelayQueueEngine::subscriptionId2upstreamSubQueueId(
    const bmqt::MessageGUID& msgGUID,
    unsigned int*            subQueueId,
    unsigned int             subscriptionId) const
{
    if (subscriptionId == bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID) {
        BSLS_ASSERT_SAFE(d_apps.find(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) !=
                         d_apps.end());
        *subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    }
    else {
        const Routers::SubscriptionIds::SharedItem itId =
            d_queueState_p->routingContext().d_groupIds.find(subscriptionId);
        if (!itId) {
            BMQ_LOGTHROTTLE_ERROR()
                << "#QUEUE_UNKNOWN_SUBSCRIPTION_ID "
                << "Remote queue: " << d_queueState_p->uri()
                << " (id: " << d_queueState_p->id()
                << ") received a PUSH message for guid " << msgGUID
                << ", with unknown Subscription Id " << subscriptionId;

            return false;  // RETURN
        }
        if (itId->value().d_priorityGroup == 0) {
            // The 'd_queueContext.d_groupIds' may contain new ids for which
            // configure response is not received yet.

            BMQ_LOGTHROTTLE_ERROR()
                << "#QUEUE_UNKNOWN_SUBSCRIPTION_ID "
                << "Remote queue: " << d_queueState_p->uri()
                << " (id: " << d_queueState_p->id()
                << ") received a PUSH message for guid " << msgGUID
                << ", with unconfigured Subscription Id " << subscriptionId;

            return false;
        }
        *subQueueId = itId->value().upstreamSubQueueId();
    }
    return true;
}

unsigned int
RelayQueueEngine::push(mqbi::StorageMessageAttributes*     attributes,
                       const bmqt::MessageGUID&            msgGUID,
                       const bsl::shared_ptr<bdlbb::Blob>& appData,
                       bmqp::Protocol::SubQueueInfosArray& subscriptions,
                       bool                                isOutOfOrder)
{
    if (isOutOfOrder) {
        BSLS_ASSERT_SAFE(subscriptions.size() == 1);

        // No guarantee of uniqueness.  Cannot use PushStream.
        unsigned int subQueueId;

        unsigned int subscriptionId = subscriptions.begin()->id();
        unsigned int ordinalPlusOne = 0;  // Invalid value

        // Reusing 'subscriptions' to 'setPushState()' below.
        subscriptions.begin()->setId(ordinalPlusOne);

        if (subscriptionId2upstreamSubQueueId(msgGUID,
                                              &subQueueId,
                                              subscriptionId)) {
            App_State* app = findApp(subQueueId);

            if (app == 0) {
                BMQ_LOGTHROTTLE_ERROR()
                    << "#QUEUE_UNKNOWN_SUBSCRIPTION_ID "
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id()
                    << ") discarding a PUSH message for guid " << msgGUID
                    << ", with unknown App Id " << subQueueId;

                return 0;  // RETURN
            }

            if (!checkForDuplicate(app, msgGUID)) {
                return 0;  // RETURN
            }

            app->putForRedelivery(msgGUID);

            attributes->setRefCount(1);

            // Reusing 'subscriptions' to 'setPushState()' below.
            ordinalPlusOne = 1 + app->ordinal();
            subscriptions.begin()->setId(ordinalPlusOne);

            storePush(attributes, msgGUID, appData, subscriptions, true);

            // Attempt to deliver
            processAppRedelivery(subQueueId, app);

            return 1;  // RETURN
        }

        return 0;  // RETURN
    }

    // Count only those subQueueIds which 'storage' is aware of.

    PushStream::iterator itGuid = d_pushStream.findOrAppendMessage(msgGUID);
    unsigned int         count  = 0;

    for (bmqp::Protocol::SubQueueInfosArray::iterator it =
             subscriptions.begin();
         it != subscriptions.end();
         ++it) {
        unsigned int subQueueId;

        unsigned int subscriptionId = it->id();
        unsigned int ordinalPlusOne = 0;  // Invalid value

        // Reusing 'subscriptions' to 'setPushState()' below.
        it->setId(ordinalPlusOne);

        if (!subscriptionId2upstreamSubQueueId(msgGUID,
                                               &subQueueId,
                                               subscriptionId)) {
            continue;  // CONTINUE
        }

        PushStream::Apps::iterator itApp = d_pushStream.d_apps.find(
            subQueueId);
        if (itApp == d_pushStream.d_apps.end()) {
            AppsMap::const_iterator app_cit = d_apps.find(subQueueId);

            if (app_cit == d_apps.end()) {
                BMQ_LOGTHROTTLE_ERROR()
                    << "#QUEUE_UNKNOWN_SUBSCRIPTION_ID "
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id()
                    << ") discarding a PUSH message for guid " << msgGUID
                    << ", with unknown App Id " << subscriptionId;
                continue;  // CONTINUE
            }

            itApp =
                d_pushStream.d_apps.emplace(subQueueId, app_cit->second).first;
        }
        else {
            const PushStream::App& app = itApp->second;

            if (app.last() && app.last()->equal(itGuid)) {
                BMQ_LOGTHROTTLE_INFO()
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id() << ", App '"
                    << itApp->second.d_app->appId()
                    << "') discarding a duplicate PUSH for guid " << msgGUID;

                continue;  // CONTINUE
            }

            if (!checkForDuplicate(app.d_app.get(), msgGUID)) {
                continue;  // CONTINUE
            }
        }

        PushStream::Element* element =
            d_pushStream.create(it->rdaInfo(), subscriptionId, itGuid, itApp);

        // Reusing 'subscriptions' to 'setPushState()' below.
        ordinalPlusOne = 1 + itApp->second.d_app->ordinal();
        it->setId(ordinalPlusOne);

        d_pushStream.add(element);
        ++count;
    }

    if (count) {
        // Pass correct ref count
        attributes->setRefCount(count);
        storePush(attributes, msgGUID, appData, subscriptions, false);
    }
    return count;
}

bool RelayQueueEngine::checkForDuplicate(const App_State*         app,
                                         const bmqt::MessageGUID& msgGUID)
{
    // Check the storage if this is duplicate PUSH
    // Currently, only Proxies can do this because they clear their storage
    // once all readers are gone.  Therefore, a Proxy can detect duplicate PUSH
    // when there is a client (potentially) processing the same PUSH.
    // Replicas do not clear their (replicated) storage.  They do not check if
    // there is a client (potentially) processing the same PUSH.
    // Also, Proxies use InMemoryStorage which relies on PUSH uniqueness for
    // calculating the refCount.
    // (Replicas receive the refCount by replication).

    if (d_queueState_p->domain()->cluster()->isRemote()) {
        d_realStorageIter_mp->reset(msgGUID);
        if (!d_realStorageIter_mp->atEnd()) {
            const mqbi::AppMessage& appView =
                d_realStorageIter_mp->appMessageView(app->ordinal());

            if (appView.isPushing()) {
                BMQ_LOGTHROTTLE_INFO()
                    << "Remote queue: " << d_queueState_p->uri()
                    << " (id: " << d_queueState_p->id() << ", App '"
                    << app->appId()
                    << "') discarding a duplicate PUSH for guid " << msgGUID;
                return false;  // RETURN
            }
        }
    }
    return true;
}

void RelayQueueEngine::storePush(
    mqbi::StorageMessageAttributes*           attributes,
    const bmqt::MessageGUID&                  msgGUID,
    const bsl::shared_ptr<bdlbb::Blob>&       appData,
    const bmqp::Protocol::SubQueueInfosArray& subscriptions,
    bool                                      isOutOfOrder)
{
    if (d_queueState_p->domain()->cluster()->isRemote()) {
        // Save the message along with the subIds in the storage.  Note that
        // for now, we will assume that in fanout mode, the only option present
        // in 'options' is subQueueInfos, and we won't store the specified
        // 'options' in the storage.

        mqbi::DataStreamMessage* dataStreamMessage = 0;

        mqbi::StorageResult::Enum result = storage()->put(
            attributes,
            msgGUID,
            appData,
            bsl::shared_ptr<bdlbb::Blob>(),
            &dataStreamMessage);  // No options

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                result != mqbi::StorageResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQ_LOGTHROTTLE_INFO()
                << d_queueState_p->uri() << " failed to store GUID ["
                << msgGUID << "], result = " << result;
        }
        else {
            BSLS_ASSERT_SAFE(dataStreamMessage);

            // Reusing previously cached ordinals.
            for (bmqp::Protocol::SubQueueInfosArray::const_iterator cit =
                     subscriptions.begin();
                 cit != subscriptions.end();
                 ++cit) {
                if (cit->id() > 0) {
                    dataStreamMessage->app(cit->id() - 1).setPushState();
                }
            }
        }
    }
}

void RelayQueueEngine::beforeOneAppRemoved(unsigned int upstreamSubQueueId)
{
    while (!d_storageIter_mp->atEnd()) {
        const int numApps = d_storageIter_mp->numApps();
        if (numApps > 1) {
            // Removal of App's elements will not invalidate 'd_storageIter_mp'
            break;
        }
        if (numApps == 1) {
            const PushStream::Element* element = d_storageIter_mp->element(0);
            if (element->app().d_app->upstreamSubQueueId() !=
                upstreamSubQueueId) {
                break;
            }
        }
        else {
            BSLS_ASSERT_SAFE(numApps == 0);

            // The case when 'advance' does not follow 'removeCurrentElement'
        }

        d_storageIter_mp->advance();
    }
}

mqbi::Storage* RelayQueueEngine::storage() const
{
    return d_queueState_p->storage();
}

}  // close package namespace
}  // close enterprise namespace
