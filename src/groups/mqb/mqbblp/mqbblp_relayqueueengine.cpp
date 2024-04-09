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

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_weakmemfn.h>

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

// ====================
// class LimitedPrinter
// ====================

/// An utility class for limiting printing of large objects to a stream.
class LimitedPrinter {
  private:
    // DATA
    const bsl::size_t  d_maxPrintBytes;
    mwcu::MemOutStream d_out;

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

    // Attempt to deliver all data in the storage.  Otherwise, broadcast
    // can get dropped if the incoming configure response removes consumers.

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

    if (bmqp_ctrlmsg::StatusCategory::E_SUCCESS == status.category()) {
        BALL_LOG_INFO << "Received success 'configure-stream' response for "
                      << "handle [" << handle << "] for queue ["
                      << d_queueState_p->uri() << "], for parameters "
                      << downStreamParameters;
    }
    else {
        BALL_LOG_WARN
            << "#QUEUE_CONFIGURE_FAILURE "
            << "Received failed 'configure-stream' response for handle '"
            << handle->client() << ":" << handle->id() << "' for queue '"
            << d_queueState_p->uri() << "', for parameters "
            << downStreamParameters << ", but assuming success.";
    }

    BALL_LOGTHROTTLE_INFO_BLOCK(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
    {
        mqbcmd::RoundRobinRouter outrr(d_allocator_p);
        context->d_routing_sp->loadInternals(&outrr);

        BALL_LOG_OUTPUT_STREAM << "For queue [" << d_queueState_p->uri()
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

    if (counts.d_readCount > 0) {
        AppsMap::const_iterator itApp = d_apps.find(upstreamSubQueueId);
        App_State&              app   = *itApp->second;

        BSLS_ASSERT_SAFE(itApp != d_apps.end());

        applyConfiguration(app, *context);
    }

    BALL_LOGTHROTTLE_INFO_BLOCK(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
    {
        mqbcmd::QueueEngine outqe(d_allocator_p);
        loadInternals(&outqe);

        BALL_LOG_OUTPUT_STREAM << "For queue [" << d_queueState_p->uri()
                               << "], the engine config is "
                               << LimitedPrinter(outqe, 2048, d_allocator_p);
    }

    // Invoke callback sending ConfigureQueue response before PUSHing.
    context->invokeCallback();

    // 'downstreamSubQueueId' can represent a producer, or a consumer with no
    // capacity or with some capacity.  We want to invoke 'onHandleUsable' only
    // in the later case.

    deliverMessages();
}

void RelayQueueEngine::onHandleReleased(
    const bmqp_ctrlmsg::Status&                                  status,
    mqbi::QueueHandle*                                           handle,
    const bmqp_ctrlmsg::QueueHandleParameters&                   hp,
    const bsl::shared_ptr<QueueEngineUtil_ReleaseHandleProctor>& proctor)
{
    d_queueState_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(mwcu::WeakMemFnUtil::weakMemFn(
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

    App_State& app = *appStateIt->second;

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
            // streamParameters (i.e. invalid consumerPriority).  This is not
            // the expected flow since we expect an incoming configureQueue
            // with null streamParameters to precede a 'releaseHandle' that
            // releases the consumer portion of the handle.
            BALL_LOG_ERROR
                << "#QUEUE_IMPROPER_BEHAVIOR "
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

        app.invalidate(handle);

        // This is in continuation of the special-case handling above.  If the
        // client is attempting to release the consumer portion of an *active*
        // (highest priority) consumer handle without having first configured
        // it to have null streamParameters (i.e. invalid consumerPriority).
        // Then, we need to rebuild the highest priority state so as to "mimic"
        // the effects of a configureQueue with null streamParameters.  Note
        // that this may affect the 'd_queueState_p->streamParameters()'.

        if (app.transferUnconfirmedMessages(handle, info)) {
            processAppRedelivery(app, info.appId());
        }
        else {
            // We lost the last reader.
            //
            // Messages to be delivered downstream need to be cleared from
            // 'd_subStreamMessages_p' for all subIds (since we have lost all
            // readers), because those messages may be re-routed by the primary
            // to another client.  Also get rid of any pending and
            // to-be-redelivered messages.
            mqbu::StorageKey appKey = mqbu::StorageKey(upstreamSubQueueId);
            afterQueuePurged(info.appId(), appKey);
        }
        if (streamResult.hasNoHandleStreamProducers()) {
            // Remove the producer handle from cached producers/consumers
            app.d_cache.erase(handle);
        }
    }

    if (streamResult.isQueueStreamEmpty()) {
        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], removing App for appId: [" << info.appId()
                      << "] and virtual storage associated with"
                      << " upstreamSubQueueId: [" << upstreamSubQueueId << "]";

        BSLS_ASSERT_SAFE(app.d_cache.empty());
        const bool removed = d_subStreamMessages_p->removeVirtualStorage(
            mqbu::StorageKey(upstreamSubQueueId));

        BSLS_ASSERT_SAFE(removed);
        (void)removed;  // Compiler happiness

        // Remove and delete empty App_State
        d_apps.erase(appStateIt);
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

    // Auto-purge on exit.
    // TBD: We need to revisit this.  We would prefer to have
    //      'deliverMessages()' called once for a bunch of messages, since
    //      what we have now is inefficient but it's less of an overhead here
    //      as compared to the primary, so this is why we haven't implemented
    //      this optimization yet.
    AutoPurger onExit(*this);

    // The guard MUST be created before the no consumers check.  This is
    // because in broadcast mode we want to purge any unused messages even if
    // there are no consumers.  Failure to do so would lead to keeping messages
    // and delivering them when a new consumers arrive.

    // Deliver messages until either:
    //   1. End of storage; or
    //   2. subStream's capacity is saturated

    QueueEngineUtil_AppsDeliveryContext context(d_queueState_p->queue(),
                                                d_allocator_p);
    while (context.d_doRepeat) {
        context.reset();

        for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
            // First try to send any message in the redelivery list ...
            bsls::TimeInterval     delay;
            const AppStateSp&      appSp = it->second;
            const mqbu::StorageKey key(appSp->upstreamSubQueueId());
            mqbi::Storage* storage = d_subStreamMessages_p->virtualStorage(
                key);
            const bsl::string& appId = appSp->d_appId;
            BSLS_ASSERT_SAFE(storage);

            appSp->processDeliveryLists(&delay, key, *storage, appId);

            if (delay != bsls::TimeInterval()) {
                appSp->scheduleThrottle(
                    mwcsys::Time::nowMonotonicClock() + delay,
                    bdlf::BindUtil::bind(
                        &RelayQueueEngine::processAppRedelivery,
                        this,
                        bsl::ref(*appSp),
                        appSp->d_appId));
            }
            else if (appSp->redeliveryListSize() == 0) {
                context.processApp(*appSp);
            }
        }
        context.deliverMessage();
    }
}

void RelayQueueEngine::processAppRedelivery(App_State&         state,
                                            const bsl::string& appId)
{
    // executed by the *QUEUE DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    BSLS_ASSERT_SAFE(d_subStreamMessages_p);

    bsls::TimeInterval     delay;
    const mqbu::StorageKey key(state.upstreamSubQueueId());
    mqbi::Storage* virtualStorage = d_subStreamMessages_p->virtualStorage(key);

    BSLS_ASSERT_SAFE(virtualStorage);

    state.processDeliveryLists(&delay, key, *virtualStorage, appId);

    if (delay != bsls::TimeInterval()) {
        state.scheduleThrottle(
            mwcsys::Time::nowMonotonicClock() + delay,
            bdlf::BindUtil::bind(&RelayQueueEngine::processAppRedelivery,
                                 this,
                                 bsl::ref(state),
                                 appId));
    }
    else if (state.redeliveryListSize() == 0) {
        deliverMessages();
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

    BALL_LOG_INFO << "For queue '" << d_queueState_p->uri() << "', about to "
                  << "rebuild upstream state [current stream parameters: "
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
        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], last advertised stream parameter by the queue"
                      << " were same as newly advertised ones: "
                      << previousParameters
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
        mwcu::MemOutStream errorStream(d_allocator_p);
        context->load(iter->first,
                      &errorStream,
                      iter->second.d_downstreamSubQueueId,
                      upstreamSubQueueId,
                      streamParameters,
                      appState->d_routing_sp.get());

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

    BALL_LOG_INFO << "For queue '" << d_queueState_p->uri() << "', rebuilt "
                  << "upstream parameters [new upstream parameters: "
                  << upstreamParams << "]";
}

void RelayQueueEngine::applyConfiguration(App_State&        app,
                                          ConfigureContext& context)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    app.reset();

    app.d_routing_sp = context.d_routing_sp;

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

    app.d_routing_sp->apply();

    app.d_routing_sp->registerSubscriptions();

    BALL_LOG_INFO << "For queue '" << d_queueState_p->uri() << "', "
                  << "rebuilt highest priority consumers: [count: "
                  << app.consumers().size() << "]";
}

// CREATORS
RelayQueueEngine::RelayQueueEngine(
    QueueState*                  queueState,
    mqbs::VirtualStorageCatalog* subStreamMessages,
    const mqbconfm::Domain&      domainConfig,
    bslma::Allocator*            allocator)
: d_queueState_p(queueState)
, d_subStreamMessages_p(subStreamMessages)
, d_domainConfig(domainConfig, allocator)
, d_apps(allocator)
, d_self(this)  // use default allocator
, d_scheduler_p(queueState->scheduler())
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueState);
    BSLS_ASSERT_SAFE(queueState->queue());
    BSLS_ASSERT_SAFE(queueState->storage());
    BSLS_ASSERT_SAFE(subStreamMessages);

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
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    BSLS_ASSERT_SAFE(d_subStreamMessages_p);
    return 0;
}

void RelayQueueEngine::resetState()
{
    d_self.reset(this);

    for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
        it->second->reset();
        it->second->d_routing_sp.reset();
    }
    d_apps.clear();
}

int RelayQueueEngine::rebuildInternalState(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    BSLS_ASSERT_OPT(false && "should never be invoked");
    return 0;
}

mqbi::QueueHandle* RelayQueueEngine::getHandle(
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
                                         *clientContext) != 0) {
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
            &d_queueState_p->stats());
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

    AppsMap::iterator itApp = d_apps.find(upstreamSubQueueId);
    if (itApp == d_apps.end()) {
        // Create and insert new App_State
        // Correlate downstreamInfo with upstream
        mqbu::StorageKey   appKey = mqbu::StorageKey(upstreamSubQueueId);
        bsl::ostringstream errorDesc;
        d_subStreamMessages_p->addVirtualStorage(errorDesc,
                                                 downstreamInfo.appId(),
                                                 appKey);

        AppStateSp appStateSp(new (*d_allocator_p) App_State(
                                  upstreamSubQueueId,
                                  downstreamInfo.appId(),
                                  d_subStreamMessages_p->getIterator(appKey),
                                  d_queueState_p->queue(),
                                  d_scheduler_p,
                                  appKey,
                                  d_queueState_p->routingContext(),
                                  d_allocator_p),
                              d_allocator_p);
        BSLS_ASSERT_SAFE(appStateSp->d_storageIter_mp);

        itApp = d_apps.insert(bsl::make_pair(upstreamSubQueueId, appStateSp))
                    .first;

        d_queueState_p->adopt(appStateSp);

        BALL_LOG_INFO << "For queue [" << d_queueState_p->uri()
                      << "], created App for appId: ["
                      << downstreamInfo.appId()
                      << "] and virtual storage associated with"
                      << " upstreamSubQueueId: [" << upstreamSubQueueId << "]";
    }

    BSLS_ASSERT_SAFE(itApp != d_apps.end());
    App_State& app(*itApp->second);

    queueHandle->registerSubStream(
        downstreamInfo,
        upstreamSubQueueId,
        mqbi::QueueCounts(handleParameters.readCount(),
                          handleParameters.writeCount()));

    // If a new reader/write, insert its (default-valued) stream parameters
    // into our map of consumer stream parameters advertised upstream.
    bsl::pair<App_State::CachedParametersMap::iterator, bool> insertResult =
        app.d_cache.insert(bsl::make_pair(
            queueHandle,
            RelayQueueEngine_AppState::CachedParameters(handleParameters,
                                                        downstreamInfo.subId(),
                                                        d_allocator_p)));
    if (!insertResult.second) {
        bmqp::QueueUtil::mergeHandleParameters(
            &insertResult.first->second.d_handleParameters,
            handleParameters);
    }
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

    unsigned int      upstreamSubQueueId = it->second.d_upstreamSubQueueId;
    AppsMap::iterator itApp(d_apps.find(upstreamSubQueueId));
    BSLS_ASSERT_SAFE(itApp != d_apps.end());

    context->initializeRouting(d_queueState_p->routingContext());

    configureApp(*itApp->second, handle, streamParameters, context);
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

    unsigned int      upstreamSubQueueId = it->second.d_upstreamSubQueueId;
    AppsMap::iterator itApp              = d_apps.find(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(itApp != d_apps.end());
    App_State::CachedParametersMap::iterator itHandle =
        itApp->second->d_cache.find(handle);
    BSLS_ASSERT_SAFE(itHandle != itApp->second->d_cache.end());
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
            mwcu::WeakMemFnUtil::weakMemFn(&RelayQueueEngine::onHandleReleased,
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
        // TEMPORARILY
        // Call the same 'deliverMessages' as in the case of 'afterNewMessage'
        // which attempts to delivery every message for every application
        // (which is '__defaut' in non-fanout case.  While the logic is
        // simplified, it may be suboptimal if the case of large number of
        // applications and small capacity limits.

        (void)upstreamSubQueueId;
        deliverMessages();
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
    AppsMap::iterator itApp = d_apps.find(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(itApp != d_apps.end());

    // Inform the 'app' that 'msgGUID' is about to be removed from its virtual
    // storage, so that app can advance its iterator etc if required.

    QueueEngineUtil_AppState& app = *itApp->second;

    app.beforeMessageRemoved(msgGUID, false);

    // Create appKey and remove message.
    const mqbu::StorageKey appKey(upstreamSubQueueId);
    d_subStreamMessages_p->remove(msgGUID, appKey);

    app.tryCancelThrottle(handle, msgGUID);

    // If proxy, also inform the physical storage.
    if (d_queueState_p->domain()->cluster()->isRemote()) {
        mqbi::Storage*            storage = d_queueState_p->storage();
        mqbi::StorageResult::Enum rc      = storage->releaseRef(msgGUID,
                                                           appKey,
                                                           0ULL);
        if (mqbi::StorageResult::e_ZERO_REFERENCES == rc) {
            // Since there are no references, there should be no app holding
            // msgGUID and no need to call `beforeMessageRemoved`
            BSLS_ASSERT_SAFE(!d_subStreamMessages_p->hasMessage(msgGUID));

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

    AppsMap::iterator itApp = d_apps.find(upstreamSubQueueId);

    BSLS_ASSERT_SAFE(itApp != d_apps.end());

    // Inform the 'app' that 'msgGUID' is about to be removed from its virtual
    // storage, so that app can advance its iterator etc if required.

    QueueEngineUtil_AppState& app = *itApp->second;
    const mqbu::StorageKey    appKey(upstreamSubQueueId);
    int                       result = 0;

    bslma::ManagedPtr<mqbi::StorageIterator> message;

    mqbi::StorageResult::Enum rc = d_subStreamMessages_p->getIterator(&message,
                                                                      appKey,
                                                                      msgGUID);

    if (rc == mqbi::StorageResult::e_SUCCESS) {
        result = message->rdaInfo().counter();

        if (d_throttledRejectedMessages.requestPermission()) {
            BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queueState_p->uri()
                          << "' rejecting PUSH [GUID: '" << msgGUID
                          << "', subQueueId: " << app.upstreamSubQueueId()
                          << "] with the counter: [" << message->rdaInfo()
                          << "]";
        }

        if (!message->rdaInfo().isUnlimited()) {
            BSLS_ASSERT_SAFE(result);
            message->rdaInfo().setPotentiallyPoisonous(true);
            message->rdaInfo().setCounter(--result);

            if (result == 0) {
                // Inform the 'app' that 'msgGUID' is about to be removed from
                // its virtual storage, so that app can advance its iterator
                // etc if required.

                app.beforeMessageRemoved(msgGUID, false);

                d_subStreamMessages_p->remove(msgGUID, appKey);

                if (d_queueState_p->domain()->cluster()->isRemote()) {
                    rc = d_queueState_p->storage()->releaseRef(msgGUID,
                                                               appKey,
                                                               0ULL,
                                                               true);
                    if (mqbi::StorageResult::e_ZERO_REFERENCES == rc) {
                        // Since there are no references, there should be no
                        // app holding msgGUID and no need to call
                        // `beforeMessageRemoved`.
                        BSLS_ASSERT_SAFE(
                            !d_subStreamMessages_p->hasMessage(msgGUID));
                        d_queueState_p->storage()->remove(msgGUID, 0, true);
                    }
                }
            }
        }
    }
    else if (d_throttledRejectedMessages.requestPermission()) {
        BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queueState_p->uri()
                      << "' got reject for an unknown message [GUID: '"
                      << msgGUID
                      << "', subQueueId: " << app.upstreamSubQueueId() << "]";
    }

    return result;
}

void RelayQueueEngine::beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    for (AppsMap::iterator iter = d_apps.begin(); iter != d_apps.end();
         ++iter) {
        App_State& app(*iter->second);

        app.beforeMessageRemoved(msgGUID, true);

        mqbu::StorageKey appKey = mqbi::QueueEngine::k_DEFAULT_APP_KEY;

        if (d_queueState_p->hasMultipleSubStreams()) {
            appKey = app.d_appKey;
        }
        d_subStreamMessages_p->remove(msgGUID, appKey);
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
        d_subStreamMessages_p->removeAll(mqbu::StorageKey::k_NULL_KEY);

        for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
            it->second->d_storageIter_mp->reset();
            it->second->d_redeliveryList.clear();
        }
    }
    else {
        // Purge virtual storage and reset iterator corresponding to
        // the specified 'appKey', which can be constructed by using
        // either upstream subId or storage-level appKey.

        // Lookup in 'd_subStreamMessages_p' first.
        bsl::string      appIdFound;
        mqbu::StorageKey subStreamAppKey;
        bool             foundAppKey = true;

        if (d_subStreamMessages_p->hasVirtualStorage(appKey, &appIdFound)) {
            BSLS_ASSERT_SAFE(appIdFound == appId);

            subStreamAppKey = appKey;
        }
        else if (d_queueState_p->storage()->hasVirtualStorage(appKey,
                                                              &appIdFound)) {
            // Note that 'appId' may not exist in
            // 'd_subStreamMessages_p' if that consumer is not
            // connected (directly or indirectly) to this relay node.

            BSLS_ASSERT_SAFE(appIdFound == appId);

            foundAppKey = d_subStreamMessages_p->hasVirtualStorage(
                appIdFound,
                &subStreamAppKey);
        }
        else {
            foundAppKey = false;

            BALL_LOG_ERROR << "#QUEUE_STORAGE_NOTFOUND "
                           << "For queue '" << d_queueState_p->uri()
                           << "', queueKey '" << d_queueState_p->key()
                           << "', failed to find virtual storage iterator "
                           << "corresponding to appKey '" << appKey
                           << "' while attempting to purge its virtual "
                           << "storage.";
        }

        if (foundAppKey) {
            // Clear out virtual storage corresponding to the retrieved
            // 'subStreamAppKey'.
            d_subStreamMessages_p->removeAll(subStreamAppKey);

            for (AppsMap::iterator it = d_apps.begin(); it != d_apps.end();
                 ++it) {
                if (it->second->d_appId == appIdFound) {
                    it->second->d_storageIter_mp->reset();
                    it->second->d_redeliveryList.clear();
                }
            }
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

mqbi::StorageResult::Enum RelayQueueEngine::evaluateAutoSubscriptions(
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
    mqbi::Storage::AppIdKeyPairs appIdKeyPairs;
    if (d_subStreamMessages_p) {
        numSubStreams = d_subStreamMessages_p->numVirtualStorages();
        d_subStreamMessages_p->loadVirtualStorageDetails(&appIdKeyPairs);
    }
    else {
        numSubStreams = d_queueState_p->storage()->numVirtualStorages();
        d_queueState_p->storage()->loadVirtualStorageDetails(&appIdKeyPairs);
    }
    BSLS_ASSERT_SAFE(static_cast<int>(appIdKeyPairs.size()) == numSubStreams);

    relayQueueEngine.numSubstreams() = numSubStreams;
    if (numSubStreams) {
        bsl::vector<mqbcmd::RelayQueueEngineSubStream>& subStreams =
            relayQueueEngine.subStreams();
        subStreams.reserve(appIdKeyPairs.size());
        for (size_t i = 0; i < appIdKeyPairs.size(); ++i) {
            const mqbi::Storage::AppIdKeyPair& p = appIdKeyPairs[i];

            subStreams.resize(subStreams.size() + 1);
            mqbcmd::RelayQueueEngineSubStream& subStream = subStreams.back();
            subStream.appId()                            = p.first;
            mwcu::MemOutStream appKey;
            appKey << p.second;
            subStream.appKey() = appKey.str();
            subStream.numMessages() =
                (d_subStreamMessages_p
                     ? d_subStreamMessages_p->numMessages(p.second)
                     : d_queueState_p->storage()->numMessages(p.second));
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

bool RelayQueueEngine::subscriptionId2upstreamSubQueueId(
    unsigned int* subQueueId,
    unsigned int  subscriptionId) const
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
            return false;  // RETURN
        }
        if (itId->value().d_priorityGroup == 0) {
            // The 'd_queueContext.d_groupIds' may contain new ids for which
            // configure response is not received yet.
            return false;
        }
        *subQueueId = itId->value().upstreamSubQueueId();
    }
    return true;
}

}  // close package namespace
}  // close enterprise namespace
