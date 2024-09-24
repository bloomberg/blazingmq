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

// mqbblp_rootqueueengine.cpp                                         -*-C++-*-
#include <mqbblp_rootqueueengine.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queueengineutil.h>
#include <mqbblp_queuehandle.h>
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbblp_storagemanager.h>
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>
#include <mqbi_appkeygenerator.h>
#include <mqbi_dispatcher.h>
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_replicatedstorage.h>
#include <mqbs_storageprintutil.h>
#include <mqbs_voidstorageiterator.h>
#include <mqbu_capacitymeter.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqp_routingconfigurationutils.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_outstreamformatsaver.h>

// BDE
#include <ball_logthrottle.h>
#include <bdlb_print.h>
#include <bdlb_scopeexit.h>
#include <bdlb_string.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bdlt_timeunitratio.h>
#include <bsl_algorithm.h>
#include <bsl_iostream.h>
#include <bsl_iterator.h>
#include <bsl_limits.h>
#include <bslma_managedptr.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const int k_MAX_INSTANT_MESSAGES = 10;
// Maximum messages logged with throttling in a short period of time.

const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;
// Time interval between messages logged with throttling.

}  // close unnamed namespace

// ---------------------
// class RootQueueEngine
// ---------------------

// PRIVATE MANIPULATORS
size_t RootQueueEngine::deliverMessages(AppState*               app,
                                        const bsl::string&      appId,
                                        const mqbu::StorageKey& key)
{
    // executed by the *QUEUE DISPATCHER* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    BSLS_ASSERT_SAFE(d_queueState_p->storage());

    bsls::TimeInterval delay;
    size_t             numMessages =
        app->deliverMessages(&delay, key, *d_queueState_p->storage(), appId);

    if (delay != bsls::TimeInterval()) {
        app->scheduleThrottle(
            mwcsys::Time::nowMonotonicClock() + delay,
            bdlf::BindUtil::bind(&RootQueueEngine::deliverMessages,
                                 this,
                                 app,
                                 appId,
                                 key));
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(numMessages > 0)) {
        d_consumptionMonitor.onMessageSent(key);
    }

    return numMessages;
}

RootQueueEngine::Apps::iterator
RootQueueEngine::makeSubStream(const bsl::string& appId,
                               const AppKeyCount& appKey,
                               bool               isAuthorized,
                               bool               hasStorage,
                               unsigned int       upstreamSubQueueId)
{
    bslma::ManagedPtr<mqbi::StorageIterator> storageIterMp;

    if (hasStorage) {
        storageIterMp = d_queueState_p->storage()->getIterator(appKey.first);
    }
    else {
        storageIterMp = bslma::ManagedPtr<mqbi::StorageIterator>(
            new (*d_allocator_p) mqbs::VoidStorageIterator(),
            d_allocator_p);
    }
    AppStateSp app(new (*d_allocator_p)
                       AppState(storageIterMp,
                                d_queueState_p->queue(),
                                d_scheduler_p,
                                isAuthorized,
                                d_queueState_p->routingContext(),
                                upstreamSubQueueId,
                                appId,
                                appKey.first,
                                d_allocator_p),
                   d_allocator_p);

    if (hasStorage) {
        d_consumptionMonitor.registerSubStream(appKey.first);
    }

    bsl::pair<Apps::iterator, Apps::InsertResult> rc = d_apps.insert(appId,
                                                                     appKey,
                                                                     app);
    BSLS_ASSERT_SAFE(rc.second == Apps::e_INSERTED);

    return rc.first;
}

bool RootQueueEngine::validate(unsigned int upstreamSubQueueId) const
{
    if (upstreamSubQueueId < d_queueState_p->subQueues().size()) {
        return d_queueState_p->subQueues()[upstreamSubQueueId];
    }
    else {
        return false;
    }
}

const RootQueueEngine::AppStateSp&
RootQueueEngine::subQueue(unsigned int upstreamSubQueueId) const
{
    BSLS_ASSERT_SAFE(validate(upstreamSubQueueId));

    return d_queueState_p->subQueues()[upstreamSubQueueId];
}

// CLASS METHODS
void RootQueueEngine::onHandleCreation(void* ptr, void* cookie)
{
    // executed by the *DISPATCHER* thread
    RootQueueEngine* engine      = static_cast<RootQueueEngine*>(ptr);
    const bool       hndlCreated = *static_cast<bool*>(cookie);
    QueueState*      qs          = engine->d_queueState_p;
    mqbi::Queue*     queue       = qs->queue();

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queue->dispatcher()->inDispatcherThread(queue));

    queue->domain()->cluster()->onQueueHandleCreated(queue,
                                                     queue->uri(),
                                                     hndlCreated);
}

void RootQueueEngine::create(bslma::ManagedPtr<mqbi::QueueEngine>* queueEngine,
                             QueueState*                           queueState,
                             const mqbconfm::Domain& domainConfig,
                             bslma::Allocator*       allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueEngine);

    queueEngine->load(new (*allocator)
                          RootQueueEngine(queueState, domainConfig, allocator),
                      allocator);
}

void RootQueueEngine::FanoutConfiguration::loadRoutingConfiguration(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);
    BSLS_ASSERT_SAFE(bmqp::RoutingConfigurationUtils::isClear(*config));

    bmqp::RoutingConfigurationUtils::setHasMultipleSubStreams(config);
    bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(config);
}

void RootQueueEngine::PriorityConfiguration::loadRoutingConfiguration(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);
    BSLS_ASSERT_SAFE(bmqp::RoutingConfigurationUtils::isClear(*config));

    bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(config);
}

void RootQueueEngine::BroadcastConfiguration::loadRoutingConfiguration(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);
    BSLS_ASSERT_SAFE(bmqp::RoutingConfigurationUtils::isClear(*config));

    bmqp::RoutingConfigurationUtils::setDeliverAll(config);
    bmqp::RoutingConfigurationUtils::setAtMostOnce(config);
    bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(config);
}

// CREATORS
RootQueueEngine::RootQueueEngine(QueueState*             queueState,
                                 const mqbconfm::Domain& domainConfig,
                                 bslma::Allocator*       allocator)
: d_queueState_p(queueState)
, d_consumptionMonitor(
      queueState,
      bdlf::BindUtil::bind(&RootQueueEngine::logAlarmCb,
                           this,
                           bdlf::PlaceHolders::_1,   // appKey
                           bdlf::PlaceHolders::_2),  // enableLog
      allocator)
, d_apps(allocator)
, d_nullKeyCount(0)
, d_hasAutoSubscriptions(false)
, d_isFanout(domainConfig.mode().isFanoutValue())
, d_scheduler_p(queueState->scheduler())
, d_miscWorkThreadPool_p(queueState->miscWorkThreadPool())
, d_appsDeliveryContext(d_queueState_p->queue(), allocator)
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p);
    BSLS_ASSERT_SAFE(d_queueState_p->queue());
    BSLS_ASSERT_SAFE(d_queueState_p->storage());
    BSLS_ASSERT_SAFE(
        d_queueState_p->queue()->domain()->cluster()->isClusterMember());

    d_throttledRejectedMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 5 seconds

    d_throttledRejectMessageDump.initialize(
        1,
        15 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // One maximum log per 15 seconds

    resetState();  // Just to ensure 'resetState' is doing what is expected,
                   // similarly to the constructor.
}

// MANIPULATORS
//   (virtual mqbi::QueueEngine)
int RootQueueEngine::configure(bsl::ostream& errorDescription)
{
    enum RcEnum {
        // Return values
        rc_SUCCESS = 0  // No error
        ,
        rc_APP_INITIALIZATION_ERROR = -1  // No Virtual Storage
        ,
        rc_AUTO_SUBSCRIPTION_ERROR = -2  // Wrong expression
        ,
        rc_AUTO_SUBSCRIPTIONS_ERROR = -3  // Wrong number of auto subscriptions
    };

    // Populate map of appId to appKey for statically registered consumers
    size_t numApps = 0;

    const bsl::vector<mqbconfm::Subscription>& subscriptions =
        d_queueState_p->domain()->config().subscriptions();
    d_hasAutoSubscriptions = !subscriptions.empty();

    if (d_isFanout) {
        const bsl::vector<bsl::string>& cfgAppIds =
            d_queueState_p->domain()->config().mode().fanout().appIDs();
        for (numApps = 0; numApps < cfgAppIds.size(); ++numApps) {
            if (initializeAppId(cfgAppIds[numApps],
                                errorDescription,
                                bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID)) {
                return rc_APP_INITIALIZATION_ERROR;  // RETURN
            }
        }
        for (unsigned int i = 0; i < subscriptions.size(); ++i) {
            Apps::iterator itApp = d_apps.findByKey1(subscriptions[i].appId());
            if (itApp != d_apps.end()) {
                int rc = itApp->value()->setSubscription(
                    subscriptions[i].expression());

                if (rc != 0) {
                    bmqeval::ErrorType::Enum errorType =
                        static_cast<bmqeval::ErrorType::Enum>(rc);

                    BALL_LOG_ERROR
                        << "#QUEUE_CONFIGURE_FAILURE Queue '"
                        << d_queueState_p->queue()->description()
                        << "' failed to compile auto subscription: '"
                        << subscriptions[i].expression().text()
                        << "' for the '" << itApp->key1()
                        << "' app, rc: " << rc << ", reason: '"
                        << bmqeval::ErrorType::toString(errorType) << "'";
                    return rc_AUTO_SUBSCRIPTION_ERROR;  // RETURN
                }
            }
            else {
                BALL_LOG_WARN << "Queue \""
                              << d_queueState_p->queue()->description()
                              << "' ignores auto subscription: '"
                              << subscriptions[i].appId() << "'";
            }
        }
    }
    else {
        numApps = 1;
        if (initializeAppId(bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                            errorDescription,
                            bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID)) {
            return rc_APP_INITIALIZATION_ERROR;  // RETURN
        }
        // TODO: what is auto subscription "appId" for priority/broadcast?
        if (subscriptions.size() > 1) {
            BALL_LOG_ERROR << "#QUEUE_CONFIGURE_FAILURE  Queue '"
                           << d_queueState_p->queue()->description()
                           << "' Cannot have more than 1 auto subscription";

            return rc_AUTO_SUBSCRIPTIONS_ERROR;  // RETURN
        }

        Apps::iterator itApp = d_apps.begin();
        BSLS_ASSERT_SAFE(itApp != d_apps.end());

        int rc = 0;
        if (subscriptions.size() == 1) {
            rc = itApp->value()->setSubscription(
                subscriptions[0].expression());
        }
        else {
            mqbconfm::Expression empty(d_allocator_p);
            rc = itApp->value()->setSubscription(empty);
        }

        if (rc != 0) {
            bmqeval::ErrorType::Enum errorType =
                static_cast<bmqeval::ErrorType::Enum>(rc);

            BALL_LOG_ERROR << "#QUEUE_CONFIGURE_FAILURE Queue '"
                           << d_queueState_p->queue()->description()
                           << "' Failed to compile auto subscription: '"
                           << subscriptions[0].expression().text()
                           << "', rc: " << rc << ", reason: '"
                           << bmqeval::ErrorType::toString(errorType) << "'";
            return rc_AUTO_SUBSCRIPTION_ERROR;  // RETURN
        }
    }

    if (!QueueEngineUtil::isBroadcastMode(d_queueState_p->queue())) {
        d_consumptionMonitor.setMaxIdleTime(
            d_queueState_p->queue()->domain()->config().maxIdleTime() *
            bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND);
    }

    return rc_SUCCESS;
}

int RootQueueEngine::initializeAppId(const bsl::string& appId,
                                     bsl::ostream&      errorDescription,
                                     unsigned int       upstreamSubQueueId)
{
    if (d_apps.findByKey1(appId) != d_apps.end()) {
        // Don't reconfigure an AppId that is already registered.
        return 0;  // RETURN
    }

    mqbu::StorageKey appKey;
    if (!d_queueState_p->storage()->hasVirtualStorage(appId, &appKey)) {
        BALL_LOG_ERROR << "#QUEUE_STORAGE_NOTFOUND "
                       << "Virtual storage does not exist for AppId '" << appId
                       << "', queue: '"
                       << d_queueState_p->queue()->description() << "'";

        errorDescription << "Virtual storage does not exist for AppId ["
                         << appId << "], queue: '"
                         << d_queueState_p->queue()->description() << "'";

        BSLS_ASSERT_SAFE(false && "Virtual storage does not exist for appId");
        return -1;  // RETURN
    }
    BSLS_ASSERT_SAFE(!appKey.isNull());

    makeSubStream(appId,
                  AppKeyCount(appKey, 0),
                  true,
                  true,
                  upstreamSubQueueId);

    BALL_LOG_INFO << "Found virtual storage for appId [" << appId
                  << "], queue [" << d_queueState_p->uri() << "], appKey ["
                  << appKey << "]";

    return 0;
}

void RootQueueEngine::resetState()
{
    d_consumptionMonitor.reset();
    d_apps.clear();
}

void RootQueueEngine::rebuildSelectedApp(
    mqbi::QueueHandle*                   handle,
    const mqbi::QueueHandle::StreamInfo& info,
    const Apps::iterator&                itApp,
    const Routers::AppContext*           previous)
{
    BSLS_ASSERT_SAFE(handle);

    const AppStateSp& app = itApp->value();

    BSLS_ASSERT_SAFE(app->d_routing_sp);

    mwcu::MemOutStream errorStream(d_allocator_p);

    app->d_routing_sp->loadApp(itApp->key1().c_str(),
                               handle,
                               &errorStream,
                               info,
                               previous);

    if (errorStream.length() > 0) {
        BALL_LOG_WARN << "#BMQ_SUBSCRIPTION_FAILURE for queue '"
                      << d_queueState_p->uri()
                      << "', error rebuilding routing [stream parameters: "
                      << info.d_streamParameters << "]: [ "
                      << errorStream.str() << " ]";
    }
}

int RootQueueEngine::rebuildInternalState(bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->id() ==
                     bmqp::QueueId::k_PRIMARY_QUEUE_ID);

    // This method is called when a node that previously was not the primary
    // for the queue becomes the primary node.  The node continues to use the
    // same queue handles for this queue, and it now needs to rebuild its
    // internal state to account for the transition from non-primary to
    // primary.

    if (d_apps.empty()) {
        BALL_LOG_ERROR
            << "#QUEUE_CONFIGURE_FAILURE "
            << "Engine must be configured before rebuilding internal state";
        errorDescription << "Engine must be configured before rebuilding"
                         << " internal state";
        return -1;  // RETURN
    }

    // Instead of rebuilding routing, adopt the existing state.
    // In any case, all previously assigned upstream subQueue ids and upstream
    // Subscription ids must stay the same.

    QueueState::SubQueues subQueues = d_queueState_p->subQueues();
    for (size_t i = 0; i < subQueues.size(); ++i) {
        AppStateSp& previous = subQueues[i];

        if (!previous) {
            continue;  // CONTINUE
        }
        Apps::iterator itApp = d_apps.findByKey1(previous->d_appId);
        unsigned int   upstreamSubQueueId = previous->upstreamSubQueueId();

        if (itApp == d_apps.end()) {
            // This means that this is an unregistered appId.
            // Reasoning: 'd_apps' is populated in 'configure', which retrieves
            // the list of registered appIds from the domain config.
            // An unregistered appId won't exist in that config, and thus,
            // won't be part of 'd_apps'.  So we explicitly update 'd_apps'
            // with this appId with the similar logic that we execute when
            // invoking 'getHandle' with an unregistered appId.

            AppKeyCount key2;

            if (d_queueState_p->isCSLModeEnabled()) {
                key2 = AppKeyCount(mqbu::StorageKey::k_NULL_KEY,
                                   d_nullKeyCount++);
            }
            else {
                key2 = AppKeyCount(
                    d_queueState_p->appKeyGenerator()->generateAppKey(
                        previous->d_appId,
                        d_queueState_p->partitionId()),
                    0);
            }

            itApp = makeSubStream(previous->d_appId,
                                  key2,
                                  false,
                                  false,
                                  bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
        }

        AppStateSp& app      = itApp->value();
        app->d_routing_sp    = previous->d_routing_sp;
        app->d_priorityCount = previous->d_priorityCount;

        app->setUpstreamSubQueueId(upstreamSubQueueId);
        d_queueState_p->abandon(upstreamSubQueueId);
        d_queueState_p->adopt(app);
    }

    BALL_LOG_INFO << "Rebuilt internal state of queue engine for queue ["
                  << d_queueState_p->queue()->description() << "] having "
                  << d_apps.size() << " consumers "
                  << "[handleParameters: "
                  << d_queueState_p->handleParameters()
                  << ", streamParameters: "
                  << d_queueState_p->subQueuesParameters() << "]";

    return 0;
}

mqbi::QueueHandle* RootQueueEngine::getHandle(
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

    // Create a proctor which will notify the cluster in its destructor
    // whether a new queue handle was created or not, so that cluster can keep
    // track of total handles created for a given queue.
    bslma::ManagedPtr<RootQueueEngine> proctor(this,                // ptr
                                               &handleCreated,      // cookie
                                               &onHandleCreation);  // deleter

    if (!bmqp::QueueUtil::isValid(handleParameters)) {
        CALLBACK(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                 -1,
                 "Invalid handle parameters specified",
                 0);

        return 0;  // RETURN
    }

    // Check num producer/consumer limits.  Note that this check is prone to
    // race during failover scenarios, these max producer/consumer config
    // fields should be used with caution (perhaps as a hint or soft-limit
    // instead of a hard limit).
    mwcu::MemOutStream errorDescription(d_allocator_p);
    if (!QueueEngineUtil::consumerAndProducerLimitsAreValid(
            d_queueState_p,
            errorDescription,
            handleParameters)) {
        CALLBACK(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                 -1,
                 errorDescription.str(),
                 0);

        return 0;  // RETURN
    }
    const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo =
        bmqp::QueueUtil::extractSubQueueInfo(handleParameters);

    if (d_isFanout) {
        if (bmqt::QueueFlagsUtil::isReader(handleParameters.flags()) &&
            bmqp::QueueUtil::isDefaultSubstream(subStreamInfo)) {
            // Fanout readers must have subQueueId info
            BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                           << "Reader of a fanout queue requires an appId to"
                           << " be specified. Queue '" << d_queueState_p->uri()
                           << "' , Specified handle params: "
                           << handleParameters << ", client '"
                           << clientContext->description() << "'"
                           << ", requesterId '" << clientContext->requesterId()
                           << "'.";

            CALLBACK(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                     -1,
                     "Reader of a fanout queue requires an appId to be"
                     " specified.",
                     0);

            return 0;  // RETURN
        }
    }
    else if (!bmqp::QueueUtil::isDefaultSubstream(subStreamInfo)) {
        // Priority should not use non-default id
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "AppId should not be specified when opening a"
                       << " non-fanout queue '" << d_queueState_p->uri()
                       << "', specified handle params: " << handleParameters
                       << ", client '" << clientContext->description() << "'"
                       << ", requesterId '" << clientContext->requesterId()
                       << "'.";

        CALLBACK(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                 -1,
                 "AppId should not be specified when opening a non-fanout"
                 " queue",
                 0);

        return 0;  // RETURN
    }

    const bmqp::QueueId queueId =
        bmqp::QueueUtil::createQueueIdFromHandleParameters(handleParameters);
    mqbi::QueueHandle* queueHandle =
        d_queueState_p->handleCatalog().getHandleByRequester(*clientContext,
                                                             queueId.id());
    if (queueHandle) {
        // Already aware of this queueId from this client.

        bmqt::Uri   uri;
        bsl::string error;
        int rc = bmqt::UriParser::parse(&uri, &error, handleParameters.uri());
        BSLS_ASSERT_SAFE(rc == 0);
        (void)rc;  // compiler happiness
        BSLS_ASSERT_SAFE(queueHandle->queue()->uri().asString() ==
                         queueHandle->queue()->uri().canonical());
        // Queue's 'uri' should always be the canonical uri
        if (queueHandle->queue()->uri().asString() != uri.canonical()) {
            BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                           << "Mismatched queue URIs for same queueId for a "
                           << "client. Rejecting open-queue request. client '"
                           << clientContext->description()
                           << "', requesterId '"
                           << clientContext->requesterId()
                           << "', queue handle ptr '" << queueHandle
                           << "'. Queue handle's URI '"
                           << queueHandle->queue()->uri()
                           << "', specified handle parameters: "
                           << handleParameters << ".";

            CALLBACK(bmqp_ctrlmsg::StatusCategory::E_INVALID_ARGUMENT,
                     -1,
                     "Queue URI mismatch for same queueId.",
                     0);

            return 0;  // RETURN
        }

        // Done with all validations; can change the state now.
        // Update the current handle parameters.
        bmqp_ctrlmsg::QueueHandleParameters currentHandleParameters =
            queueHandle->handleParameters();

        // Update current handle parameters
        bmqp::QueueUtil::mergeHandleParameters(&currentHandleParameters,
                                               handleParameters);

        // Update handle's queue parameters
        queueHandle->setHandleParameters(currentHandleParameters);

        BALL_LOG_INFO << "Reconfigured existing handle "
                      << "[client: " << clientContext->description()
                      << ", requesterId: " << clientContext->requesterId()
                      << ", queueId:" << queueId
                      << ", new handle parameters: " << currentHandleParameters
                      << "]";
    }
    else {
        // This is a new client, we need to create a new handle for it
        queueHandle = d_queueState_p->handleCatalog().createHandle(
            clientContext,
            handleParameters,
            &d_queueState_p->stats());
        BSLS_ASSERT_SAFE(queueHandle && "handle creation failed");

        handleCreated = true;

        BALL_LOG_INFO << "Created new handle " << queueHandle
                      << " [client: " << clientContext->description()
                      << ", requesterId: " << clientContext->requesterId()
                      << ", queueId: " << queueId
                      << ", handle parameters: " << handleParameters << "].";
    }
    BSLS_ASSERT_SAFE(queueHandle != 0);

    // Done with all validations; can change the state now.
    if (handleParameters.readCount()) {
        // Ensure appId is authorized
        const bsl::string& appId = subStreamInfo.appId();
        Apps::iterator     iter  = d_apps.findByKey1(appId);

        if (iter == d_apps.end()) {
            MWCTSK_ALARMLOG_ALARM("FANOUT_UNREGISTERED_APPID")
                << "AppId '" << appId << "' is not authorized for queue '"
                << d_queueState_p->uri()
                << "' - please contact BlazingMQ team to request configuration"
                   " of this AppId"
                << MWCTSK_ALARMLOG_END;

            AppKeyCount key2;
            if (d_queueState_p->isCSLModeEnabled()) {
                key2 = AppKeyCount(mqbu::StorageKey::k_NULL_KEY,
                                   d_nullKeyCount++);
            }
            else {
                key2 = AppKeyCount(
                    d_queueState_p->appKeyGenerator()->generateAppKey(
                        appId,
                        d_queueState_p->partitionId()),
                    0);
            }
            iter = makeSubStream(appId,
                                 key2,
                                 false,
                                 false,
                                 bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
        }
        BSLS_ASSERT_SAFE(iter != d_apps.end());
        BSLS_ASSERT_SAFE(iter->key1() == subStreamInfo.appId());

        // Do not insert the handle to the AppState::d_consumers until
        // configureHandle (which specifies priority)

        d_queueState_p->adopt(iter->value());
        upstreamSubQueueId = iter->value()->upstreamSubQueueId();
    }
    else {
        upstreamSubQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    }
    // else: do not create AppState for producer (but do register handle)

    // Update queue's aggregated parameters.
    d_queueState_p->add(handleParameters);

    // Handle's parameters must always be a subset of queue's aggregated handle
    // parameters
    BSLS_ASSERT_SAFE(
        bmqp::QueueUtil::isValidSubset(queueHandle->handleParameters(),
                                       d_queueState_p->handleParameters()));

    // Register substream
    queueHandle->registerSubStream(
        subStreamInfo,
        upstreamSubQueueId,
        mqbi::QueueCounts(handleParameters.readCount(),
                          handleParameters.writeCount()));

    // Inform the requester of the success
    CALLBACK(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, 0, "", queueHandle);

    return queueHandle;

#undef CALLBACK
}

void RootQueueEngine::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(handle);

#define CONFIGURE_CB(CAT, RC, MSG, SPARAMS)                                   \
    if (configuredCb) {                                                       \
        bmqp_ctrlmsg::Status status;                                          \
        status.category() = CAT;                                              \
        status.code()     = RC;                                               \
        status.message()  = MSG;                                              \
        configuredCb(status, SPARAMS);                                        \
    }

    // Verify handle exists
    if (!d_queueState_p->handleCatalog().hasHandle(handle)) {
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "Attempting to configure unknown handle. Queue '"
                       << d_queueState_p->uri()
                       << "', stream params: " << streamParameters
                       << ", handlePtr '" << handle << "'.";

        CONFIGURE_CB(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                     -1,
                     "Attempting to configure unknown handle.",
                     bmqp_ctrlmsg::StreamParameters());
        return;  // RETURN
    }

    const bsl::string& appId = streamParameters.appId();
    mqbi::QueueHandle::SubStreams::const_iterator it =
        handle->subStreamInfos().find(appId);
    if (it == handle->subStreamInfos().end()) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Attempting to configure unknown App for the handle. "
            << "Queue '" << d_queueState_p->uri()
            << "', stream params: " << streamParameters << ", handlePtr '"
            << handle << "'.";

        CONFIGURE_CB(
            bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
            -1,
            "Attempting to configure unknown substream for the handle.",
            bmqp_ctrlmsg::StreamParameters());
        return;  // RETURN
    }

    const bmqp::QueueId queueId(handle->id(),
                                it->second.d_downstreamSubQueueId);

    BALL_LOG_INFO << "For queue [" << d_queueState_p->queue()->uri()
                  << "], appId: '" << appId << "'"
                  << ", configured handle " << handle
                  << " with queueId: " << queueId
                  << " with stream parameters: " << streamParameters;

    // Skip producer.  This should be optimized out.
    if (it->second.d_counts.d_readCount == 0) {
        CONFIGURE_CB(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                     0,
                     "",
                     streamParameters);
        return;  // RETURN
    }

    handle->setStreamParameters(streamParameters);
    // Verify consumer priority validity
    // Note: 'consumerPriorityCount() == 0' means that handle is being
    // deconfigured, in this case priority expected to be invalid
    if (!bmqp::ProtocolUtil::verify(streamParameters)) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Attempting to configure stream with invalid priority. "
            << "Queue '" << d_queueState_p->uri()
            << "', stream params: " << streamParameters << ", handlePtr '"
            << handle << "'.";

        CONFIGURE_CB(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                     -1,
                     "Attempting to configure stream with invalid priority.",
                     bmqp_ctrlmsg::StreamParameters());
        return;  // RETURN
    }

    Apps::iterator iter = d_apps.findByKey1(appId);
    BSLS_ASSERT_SAFE(iter != d_apps.end());

    const AppStateSp& affectedApp = iter->value();
    // prepare the App for rebuilding consumers
    affectedApp->reset();

    // Rebuild the highest priority state for all affected apps.

    BSLS_ASSERT_SAFE(
        bmqt::QueueFlagsUtil::isReader(handle->handleParameters().flags()));

    bsl::shared_ptr<Routers::AppContext> previous = affectedApp->d_routing_sp;

    affectedApp->d_routing_sp.reset(new (*d_allocator_p) Routers::AppContext(
                                        d_queueState_p->routingContext(),
                                        d_allocator_p),
                                    d_allocator_p);

    d_queueState_p->handleCatalog().iterateConsumers(
        bdlf::BindUtil::bind(&RootQueueEngine::rebuildSelectedApp,
                             this,
                             bdlf::PlaceHolders::_1,  // handle
                             bdlf::PlaceHolders::_2,  // info
                             iter,
                             previous.get()));

    affectedApp->d_priorityCount = affectedApp->d_routing_sp->finalize();
    affectedApp->d_routing_sp->apply();
    affectedApp->d_routing_sp->registerSubscriptions();

    BALL_LOG_INFO << "Rebuilt active consumers of the highest "
                  << "priority for queue '"
                  << d_queueState_p->queue()->description() << "', appId = '"
                  << iter->key1() << "'. Now there are "
                  << affectedApp->d_priorityCount << " consumers.";

    // Inform the requester of the success before attempting to deliver new
    // messages.
    CONFIGURE_CB(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                 0,
                 "",
                 streamParameters);

    // Now triggering message delivery for affected apps
    deliverMessages(affectedApp.get(), iter->key1(), iter->key2().first);
}

void RootQueueEngine::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    BALL_LOG_INFO << "RootQueueEngine::releaseHandle "
                  << "HandlePtr '" << handle << "', queue handle's URI '"
                  << handle->queue()->uri()
                  << "', specified handle params: " << handleParameters
                  << ", isFinal: " << bsl::boolalpha << isFinal << ".";

    QueueEngineUtil_ReleaseHandleProctor proctor(d_queueState_p,
                                                 isFinal,
                                                 releasedCb);

    if (!d_queueState_p->handleCatalog().hasHandle(handle)) {
        BALL_LOG_ERROR << "#CLIENT_IMPROPER_BEHAVIOR "
                       << "Attempting to release unknown handle. HandlePtr '"
                       << handle << "', queue '" << d_queueState_p->uri()
                       << "'.";

        return;  // RETURN
    }

    bmqt::Uri   uri;
    bsl::string error;
    int rc = bmqt::UriParser::parse(&uri, &error, handleParameters.uri());
    BSLS_ASSERT_OPT(rc == 0);
    if (handle->queue()->uri().asString() != uri.canonical()) {
        // This should not occur, because we explicitly check for queue URI
        // mismatch in the open-queue request, but adding an explicit 'if'
        // check anyways.

        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "Attempting to release handle with queue URI mismatch. "
            << "HandlePtr '" << handle << "', queue handle's URI '"
            << handle->queue()->uri()
            << "', specified handle params: " << handleParameters << ".";

        return;  // RETURN
    }

    // Extract queueId (Id, subId)
    const bmqp_ctrlmsg::SubQueueIdInfo& subStreamInfo =
        bmqp::QueueUtil::extractSubQueueInfo(handleParameters);
    const bmqp::QueueId queueId(handleParameters.qId(), subStreamInfo.subId());

    // Validate 'handleParameters'.  If they are invalid, but 'isFinal' flag is
    // true, we still want to honor the 'isFinal' flag, and remove handle from
    // our catalog of handles.  But in order to do so, we need to "fix" the
    // specified invalid 'handleParameters'.  We do so by making a copy of
    // them, and initializing with with our view of the handle parameters, so
    // that any logic related to those parameters executed by us remains valid.
    bmqp_ctrlmsg::QueueHandleParameters effectiveHandleParam(handleParameters);

    if (!bmqp::QueueUtil::isValid(effectiveHandleParam) ||
        !bmqp::QueueUtil::isValidSubset(effectiveHandleParam,
                                        handle->handleParameters()) ||
        !bmqp::QueueUtil::isValidSubset(effectiveHandleParam,
                                        d_queueState_p->handleParameters())) {
        BALL_LOG_ERROR
            << "#CLIENT_IMPROPER_BEHAVIOR "
            << "For queue '" << d_queueState_p->uri() << "', invalid handle "
            << "parameters specified when  attempting to release a handle: "
            << effectiveHandleParam
            << ". Handle's current params: " << handle->handleParameters()
            << ", queue's aggregated params: "
            << d_queueState_p->handleParameters() << ". HandlePtr '" << handle
            << "', id: " << queueId.id()
            << ". 'isFinal' flag: " << bsl::boolalpha << isFinal << ".";
        if (!isFinal) {
            return;  // RETURN
        }

        // Update effective handle parameters with our view of the handle in
        // order to make their value sane.
        effectiveHandleParam = handle->handleParameters();
    }

    // After all of the above 'isValid*' checks, 'effectiveHandleParam' should
    // be a valid subset of the queue's aggregated handle parameters.  We are
    // basically trusting that self's view of handle parameters is correct.
    BSLS_ASSERT_SAFE(
        bmqp::QueueUtil::isValidSubset(effectiveHandleParam,
                                       d_queueState_p->handleParameters()));

    if (0 != proctor.releaseHandle(handle, effectiveHandleParam)) {
        // releaseHandleHelper logs the error.
        return;  // RETURN
    }

    // Determine and copy appropriate subStreams to release (either one or all)
    mqbi::QueueHandle::SubStreams subStreamInfosToRelease(d_allocator_p);

    if (proctor.result().hasNoHandleClients()) {
        // Handle is being fully released, meaning all subStreams should be
        // released

        // NOTE: In the case where 'isFinal' is true we rely on the subStream
        //       counts from the previous handle registrations to adjust the
        //       queue's cumulative values per appId.

        bsl::copy(handle->subStreamInfos().begin(),
                  handle->subStreamInfos().end(),
                  bsl::inserter(subStreamInfosToRelease,
                                subStreamInfosToRelease.begin()));
    }
    else {
        mqbi::QueueHandle::SubStreams::const_iterator itSubStream =
            handle->subStreamInfos().find(subStreamInfo.appId());
        BSLS_ASSERT_SAFE(itSubStream != handle->subStreamInfos().end());

        subStreamInfosToRelease.insert(*itSubStream).first->second.d_counts =
            mqbi::QueueCounts(effectiveHandleParam.readCount(),
                              effectiveHandleParam.writeCount());
    }
    // Release state of subStreams selected above
    for (mqbi::QueueHandle::SubStreams::const_iterator citer =
             subStreamInfosToRelease.begin();
         citer != subStreamInfosToRelease.end();
         ++citer) {
        bmqp_ctrlmsg::SubQueueIdInfo currSubStreamInfo;

        currSubStreamInfo.appId() = citer->first;
        currSubStreamInfo.subId() = citer->second.d_downstreamSubQueueId;

        bmqp_ctrlmsg::QueueHandleParameters copy(effectiveHandleParam);
        copy.subIdInfo().makeValue(currSubStreamInfo);

        copy.readCount()  = citer->second.d_counts.d_readCount;
        copy.writeCount() = citer->second.d_counts.d_writeCount;

        mqbi::QueueHandleReleaseResult result = proctor.releaseStream(copy);

        if (copy.readCount()) {
            Apps::iterator itApp = d_apps.findByKey1(citer->first);

            BSLS_ASSERT_SAFE(itApp != d_apps.end());

            AppState* app(itApp->value().get());

            if (result.hasNoHandleStreamConsumers()) {
                // No re-delivery attempts until entire handle stops consuming
                // (read count drops to zero).

                if (app->find(handle)) {
                    // The handle has a valid consumer priority, meaning that a
                    // downstream client is attempting to release the handle
                    // without having first configured it to have null
                    // streamParameters (i.e. invalid consumerPriority).  This
                    // is not the expected flow since we expect an incoming
                    // configureQueue with null streamParameters to precede a
                    // 'releaseHandle' that releases the handle.
                    BALL_LOG_ERROR
                        << "#QUEUE_IMPROPER_BEHAVIOR "
                        << "For queue [" << d_queueState_p->uri() << "],  "
                        << "received a 'releaseHandle' for the handle [id: "
                        << handle->id() << ", clientPtr: " << handle->client()
                        << ", ptr: " << handle << "] without having first "
                        << "configured the handle to have null "
                        << "streamParameters. Handle's parameters are "
                        << "[handleParameters: " << handle->handleParameters()
                        << ", streamParameters: "
                        << citer->second.d_streamParameters
                        << "], and the parameters specified in this "
                        << "'releaseHandle' request are [handleParameters: "
                        << handleParameters << ", isFinal " << bsl::boolalpha
                        << isFinal << "]";

                    // We need to set null streamParameters on the handle so as
                    // to mimic the effects of a configureQueue with null
                    // streamParameters.
                    bmqp_ctrlmsg::StreamParameters nullStreamParams;
                    nullStreamParams.appId() = currSubStreamInfo.appId();

                    handle->setStreamParameters(nullStreamParams);
                    // Create new Routing from scratch using Subscription Ids
                    // from the old routing.
                    bsl::shared_ptr<Routers::AppContext> replacement(
                        new (*d_allocator_p) Routers::AppContext(
                            d_queueState_p->routingContext(),
                            d_allocator_p),
                        d_allocator_p);

                    mwcu::MemOutStream errorStream(d_allocator_p);
                    app->rebuildConsumers(currSubStreamInfo.appId().c_str(),
                                          &errorStream,
                                          d_queueState_p,
                                          replacement);
                    if (errorStream.length() > 0) {
                        BALL_LOG_WARN
                            << "#BMQ_SUBSCRIPTION_FAILURE for queue '"
                            << d_queueState_p->uri()
                            << "', error rebuilding routing: [ "
                            << errorStream.str() << " ]";
                    }

                    BALL_LOG_INFO
                        << "Rebuilt active consumers of the "
                        << " highest priority for the queue '"
                        << d_queueState_p->queue()->description()
                        << "', appId = '" << currSubStreamInfo.appId()
                        << "'. Now there are " << app->consumers().size()
                        << " consumers.";
                }
                // else configureHandle has not been called or the handle is
                // of too low priority
                if (app->transferUnconfirmedMessages(handle,
                                                     currSubStreamInfo)) {
                    // There are potential consumers to redeliver to
                    deliverMessages(app,
                                    currSubStreamInfo.appId(),
                                    itApp->key2().first);
                }

                if (result.isQueueStreamEmpty()) {
                    // There are no clients for this app in this queue (across
                    // all handles).  If we have unauthorized app, we remove it
                    // from 'configured' apps thus returning to the original
                    // state.  On the surface it results in alarm being
                    // (re)generated if the unauthorized app is used again
                    // after all previous clients are gone.
                    if (!itApp->value()->d_isAuthorized) {
                        BALL_LOG_INFO
                            << "There are no more clients for the unauthorized"
                            << " appId [" << itApp->key1()
                            << "] and the appId was not registered with BMQ."
                            << "Removing this appId from queue engine since "
                            << "all clients have gone away.  The 'unregistered"
                            << "appId' warning will be raised again if a "
                            << "client for this appId opens the queue again.";

                        d_apps.erase(itApp);

                        d_queueState_p->abandon(app->upstreamSubQueueId());
                    }
                }
                // If lost read capacity, validate that handle is removed from
                // the set of consumers for the given appId
                BSLS_ASSERT_SAFE(!hasHandle(subStreamInfo.appId(), handle));
            }  // else there are app consumers on this handle
        }      // else producer

        // Register/unregister both consumers and producers
        handle->unregisterSubStream(
            currSubStreamInfo,
            mqbi::QueueCounts(citer->second.d_counts.d_readCount,
                              citer->second.d_counts.d_writeCount),
            proctor.result().hasNoHandleClients());
    }

    // POSTCONDITION
    BSLS_ASSERT_SAFE(!proctor.result().hasNoHandleClients() ||
                     handle->subStreamInfos().size() == 0);
}

void RootQueueEngine::onHandleUsable(mqbi::QueueHandle* handle,
                                     unsigned int       upstreamSubscriptionId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(handle);

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

    // Before attempting to deliver any messages, flush the storage.
    d_queueState_p->queue()->storage()->dispatcherFlush(true, false);

    unsigned int upstreamSubQueueId = 0;
    if (d_queueState_p->routingContext().onUsable(&upstreamSubQueueId,
                                                  upstreamSubscriptionId)) {
        const AppStateSp app = subQueue(upstreamSubQueueId);
        BSLS_ASSERT_SAFE(app);

        deliverMessages(app.get(), app->d_appId, app->d_appKey);
    }
}

void RootQueueEngine::afterNewMessage(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* source)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // Deliver new messages to active (alive and capable to deliver) consumers
    do {
        d_appsDeliveryContext.reset();

        for (Apps::iterator iter = d_apps.begin(); iter != d_apps.end();
             ++iter) {
            AppStateSp& app = iter->value();
            if (app->redeliveryListSize() == 0) {
                if (d_appsDeliveryContext.processApp(*app)) {
                    d_consumptionMonitor.onMessageSent(iter->key2().first);
                }
            }
        }
        d_appsDeliveryContext.deliverMessage();
    } while (d_appsDeliveryContext.d_doRepeat);

    if (QueueEngineUtil::isBroadcastMode(d_queueState_p->queue())) {
        // Clear storage status
        mqbi::StorageResult::Enum rc =
            d_queueState_p->queue()->storage()->removeAll(
                mqbu::StorageKey::k_NULL_KEY);
        // Intended to be used with 'InMemoryStorage'.  Since 'appKey' isn't
        //  used while calling 'removeAll()', it should always succeed.
        BSLS_ASSERT_SAFE(mqbi::StorageResult::e_SUCCESS == rc);
        (void)rc;  // Compiler happiness
    }
}

int RootQueueEngine::onConfirmMessage(mqbi::QueueHandle*       handle,
                                      const bmqt::MessageGUID& msgGUID,
                                      unsigned int             subQueueId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(
        !QueueEngineUtil::isBroadcastMode(d_queueState_p->queue()) &&
        "confirm isn't expected for this queue");
    BSLS_ASSERT_SAFE(handle);

    enum RcEnum {
        // Value for the various RC error categories
        rc_ERROR               = -1,
        rc_NO_MORE_REFERENCES  = 0,
        rc_NON_ZERO_REFERENCES = 1
    };

    // Inform the 'app' that 'msgGUID' is about to be removed from its virtual
    // storage, so that app can advance its iterator etc if required.

    // TODO: handle missing SubQueue?
    QueueEngineUtil_AppState& app = *subQueue(subQueueId);

    // Inform app that a message from its virtual storage is getting removed,
    // so that it can advance its iterator etc if required.
    app.beforeMessageRemoved(msgGUID, false);

    const mqbu::StorageKey& appKey = app.d_appKey;
    BSLS_ASSERT_SAFE(!appKey.isNull());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_queueState_p->storage()->hasVirtualStorage(appKey))) {
        // If an appId was dynamically unregistered, it is possible that the
        // client may still attempt at confirming outstanding messages, which
        // we need to guard against.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_ERROR;  // RETURN
    }

    // Release from storage
    mqbi::StorageResult::Enum rc = d_queueState_p->storage()->releaseRef(
        msgGUID,
        appKey,
        bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::utc()));

    app.tryCancelThrottle(handle, msgGUID);

    if (rc == mqbi::StorageResult::e_NON_ZERO_REFERENCES) {
        return rc_NON_ZERO_REFERENCES;  // RETURN
    }

    if (rc == mqbi::StorageResult::e_ZERO_REFERENCES) {
        return rc_NO_MORE_REFERENCES;  // RETURN
    }

    BALL_LOG_INFO << "'" << d_queueState_p->queue()->description()
                  << "', appId = '" << app.d_appId
                  << "' failed to release references upon CONFIRM " << msgGUID
                  << "' [reason: " << mqbi::StorageResult::toAscii(rc) << "]";

    // TBD: Handle return code for 'e_GUID_NOT_FOUND', 'e_APPKEY_NOT_FOUND',
    //      and (probably dramatically) 'e_WRITE_FAILURE'.

    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    return rc_ERROR;
}

int RootQueueEngine::onRejectMessage(mqbi::QueueHandle*       handle,
                                     const bmqt::MessageGUID& msgGUID,
                                     unsigned int             subQueueId)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(
        !QueueEngineUtil::isBroadcastMode(d_queueState_p->queue()) &&
        "reject isn't expected for this queue");
    BSLS_ASSERT_SAFE(handle);

    // TODO: handle missing SubQueue?
    QueueEngineUtil_AppState& app = *subQueue(subQueueId);

    const mqbu::StorageKey& appKey = app.d_appKey;

    BSLS_ASSERT_SAFE(!appKey.isNull());

    bslma::ManagedPtr<mqbi::StorageIterator> message;
    int                                      counter = 0;
    mqbi::StorageResult::Enum                storageRc =
        d_queueState_p->storage()->getIterator(&message, appKey, msgGUID);

    if (storageRc == mqbi::StorageResult::e_SUCCESS) {
        // Handle 'maxDeliveryAttempts' parameter reconfigure.
        // To do this, we need to take care of the following:
        //   1. Change the default 'maxDeliveryAttempts' in message storage
        // classes, so it is set as a default for any new message iterator.
        //   2. If possible, update the existing messages with the new value
        // of 'maxDeliveryAttempts'.
        // This code does the 2nd part.  We need this fix to be able to get rid
        // of poisonous messages already stored in a working cluster, without
        // bouncing off.
        //
        // We use the domain's 'maxDeliveryAttempts' as a baseline to compare
        // with each specific message's 'rdaInfo', and there might be a few
        // cases:
        // +=====================+===========+===============================+
        // | maxDeliveryAttempts | rdaInfo   | Action:                       |
        // +=====================+===========+===============================+
        // | Unlimited           | Unlimited | Do nothing (same value)       |
        // +---------------------+-----------+-------------------------------+
        // | Unlimited           | Limited   | Set 'rdaInfo' to unlimited    |
        // +---------------------+-----------+-------------------------------+
        // | Limited             | Unlimited | Set 'rdaInfo' to limited      |
        // |                     |           | 'maxDeliveryAttempts'         |
        // +---------------------+-----------+-------------------------------+
        // | Limited             | Limited   | Do nothing (not possible to   |
        // |                     |           | deduce what to do in general) |
        // +---------------------+-----------+-------------------------------+
        // So this code handles only the situation when we want to switch
        // 'rdaInfo' between limited and unlimited.
        //
        // See also: mqbblp_relayqueueengine
        // Note that RelayQueueEngine doesn't contain a similar code to fix
        // 'rdaInfo'.  This is because we work with an assumption that if we
        // have a poisonous message, all the consumers will crash anyway, so a
        // replica/proxy will free the corresponding handles, and all message
        // iterators will be recreated with the correct 'rdaInfo' received from
        // primary, if a new consumer connects to the replica/proxy.
        const int maxDeliveryAttempts =
            d_queueState_p->domain()->config().maxDeliveryAttempts();
        const bool domainIsUnlimited = (maxDeliveryAttempts == 0);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                domainIsUnlimited != message->rdaInfo().isUnlimited())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOGTHROTTLE_WARN(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
                << "[THROTTLED] Mismatch between the message's RdaInfo "
                << message->rdaInfo() << " and the domain's "
                << "'maxDeliveryAttempts' setting [" << maxDeliveryAttempts
                << "], updating message's RdaInfo";
            if (maxDeliveryAttempts > 0) {
                message->rdaInfo().setCounter(maxDeliveryAttempts);
            }
            else {
                message->rdaInfo().setUnlimited();
            }
        }

        counter = message->rdaInfo().counter();

        if (d_throttledRejectedMessages.requestPermission()) {
            BALL_LOG_INFO << "[THROTTLED] Queue '" << d_queueState_p->uri()
                          << "' rejecting PUSH [GUID: '" << msgGUID
                          << "', appId: " << app.d_appId
                          << ", subQueueId: " << app.upstreamSubQueueId()
                          << "] with the counter: [" << message->rdaInfo()
                          << "]";
        }

        if (!message->rdaInfo().isUnlimited()) {
            BSLS_ASSERT_SAFE(counter);
            message->rdaInfo().setPotentiallyPoisonous(true);
            message->rdaInfo().setCounter(--counter);

            if (counter == 0) {
                // Purge the message from corresponding app.
                app.beforeMessageRemoved(msgGUID, false);
                storageRc = d_queueState_p->storage()->releaseRef(msgGUID,
                                                                  appKey,
                                                                  0ULL,
                                                                  true);

                // Log the rejected message and raise an alarm, in a throttled
                // manner.
                if (d_throttledRejectMessageDump.requestPermission()) {
                    bsl::shared_ptr<bdlbb::Blob>   appData;
                    bsl::shared_ptr<bdlbb::Blob>   options;
                    mqbi::StorageMessageAttributes attributes;
                    int retrievalRc = d_queueState_p->storage()->get(
                        &appData,
                        &options,
                        &attributes,
                        msgGUID);
                    BSLS_ASSERT_SAFE(retrievalRc == 0);
                    static_cast<void>(retrievalRc);
                    d_miscWorkThreadPool_p->enqueueJob(bdlf::BindUtil::bind(
                        &QueueEngineUtil::logRejectMessage,
                        msgGUID,
                        app.d_appId,
                        app.upstreamSubQueueId(),
                        appData,
                        attributes,
                        d_queueState_p,
                        d_allocator_p));
                }
                d_queueState_p->stats().onEvent(
                    mqbstat::QueueStatsDomain::EventType::e_REJECT,
                    1);

                // Lastly, if message reached a ref count of zero in the
                // storage (i.e., all appIds have confirmed the message),
                // delete it from the storage.

                if (mqbi::StorageResult::e_ZERO_REFERENCES == storageRc) {
                    // Since there are no references, there should be no app
                    // holding msgGUID and no need to call
                    // `beforeMessageRemoved`.
                    beforeMessageRemoved(msgGUID);
                    d_queueState_p->storage()->remove(msgGUID, 0, true);
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

    return counter;
}

void RootQueueEngine::beforeMessageRemoved(const bmqt::MessageGUID& msgGUID)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    for (Apps::iterator iter = d_apps.begin(); iter != d_apps.end(); ++iter) {
        iter->value()->beforeMessageRemoved(msgGUID, true);
    }
}

void RootQueueEngine::afterQueuePurged(const bsl::string&      appId,
                                       const mqbu::StorageKey& appKey)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (appKey.isNull()) {
        // NOTE: Since in CSL mode when a consumer opens the queue with an
        // unauthorized appId, we insert an item having a pair of (appId,
        // nullKey) as its key to d_apps.  Thus, to avoid accidentally treating
        // a nullKey resulting from unauthorized appId as wildcard matching, we
        // add an additional assert that the appId must be empty.
        BSLS_ASSERT_SAFE(appId == bmqp::ProtocolUtil::k_NULL_APP_ID);

        for (Apps::iterator iter = d_apps.begin(Apps::e_SECOND_KEY);
             iter != d_apps.end();
             ++iter) {
            BSLS_ASSERT_SAFE(iter->value()->d_storageIter_mp);
            iter->value()->d_storageIter_mp->reset();
        }

        return;  // RETURN
    }

    Apps::iterator iter = d_apps.findByKey2(AppKeyCount(appKey, 0));
    BSLS_ASSERT_SAFE(iter != d_apps.end());
    BSLS_ASSERT_SAFE(iter->key1() == appId);
    iter->value()->d_storageIter_mp->reset();
}

void RootQueueEngine::onTimer(bsls::Types::Int64 currentTimer)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    d_consumptionMonitor.onTimer(currentTimer);
}

bool RootQueueEngine::logAlarmCb(const mqbu::StorageKey& appKey,
                                 const bool              enableLog) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    // Construct AppId from appKey
    bmqt::UriBuilder uriBuilder(d_queueState_p->uri(), d_allocator_p);
    bsl::string      appId(d_allocator_p);

    if (appKey.isNull()) {
        appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    }
    else if (d_queueState_p->storage()->hasVirtualStorage(appKey, &appId)) {
        uriBuilder.setId(appId);
    }

    // Get AppState by appId.
    Apps::const_iterator cItApp = d_apps.findByKey1(appId);
    if (cItApp == d_apps.end()) {
        BALL_LOG_WARN << "No app found for appId: " << appId;
        return false;  // RETURN
    }
    const AppStateSp& app = cItApp->value();

    // Check if there are un-delivered messages
    bslma::ManagedPtr<mqbi::StorageIterator> head = app->head();
    if (!head) {
        // No un-delivered messages, do nothing.
        return false;  // RETURN
    }
    else if (!enableLog) {
        // There are un-delivered messages, but log is disabled.
        return true;  // RETURN
    }

    // Logging alarm info
    bdlma::LocalSequentialAllocator<4096> localAllocator(d_allocator_p);

    bmqt::Uri uri(&localAllocator);
    uriBuilder.uri(&uri);

    mwcu::MemOutStream ss(&localAllocator);

    // Log app consumers queue handles info
    int idx          = 1;
    int numConsumers = 0;

    QueueEngineUtil_AppState::Consumers& consumers = app->consumers();
    for (QueueEngineUtil_AppState::Consumers::const_iterator citConsumer =
             consumers.begin();
         citConsumer != consumers.end();
         ++citConsumer) {
        mqbi::QueueHandle* const queueHandle_p = citConsumer->first;

        const mqbi::QueueHandle::SubStreams& subStreamInfos =
            queueHandle_p->subStreamInfos();

        for (mqbi::QueueHandle::SubStreams::const_iterator citSubStreams =
                 subStreamInfos.begin();
             citSubStreams != subStreamInfos.end();
             ++citSubStreams) {
            numConsumers += citSubStreams->second.d_counts.d_readCount;

            const int level = 2, spacesPerLevel = 2;

            ss << "\n  " << idx++ << ". "
               << queueHandle_p->client()->description()
               << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
               << "Handle Parameters .....: "
               << queueHandle_p->handleParameters()
               << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
               << "Number of unconfirmed messages .....: "
               << queueHandle_p->countUnconfirmed()
               << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
               << "UnconfirmedMonitors ....:";

            const bsl::vector<const mqbu::ResourceUsageMonitor*> monitors =
                queueHandle_p->unconfirmedMonitors(appId);
            for (size_t i = 0; i < monitors.size(); ++i) {
                ss << "\n  " << *monitors[i];
            }
        }
    }

    mwcu::MemOutStream out(&localAllocator);
    out << "Queue '" << uri << "' ";
    d_queueState_p->storage()->capacityMeter()->printShortSummary(out);
    out << ", max idle time "
        << mwcu::PrintUtil::prettyTimeInterval(
               d_queueState_p->queue()->domain()->config().maxIdleTime() *
               bdlt::TimeUnitRatio::k_NANOSECONDS_PER_SECOND)
        << " appears to be stuck. It currently has " << numConsumers
        << " consumers." << ss.str() << '\n';

    // Log un-delivered messages info
    mqbi::Storage* const storage = d_queueState_p->storage();
    out << "\nFor appId: " << appId << '\n';
    out << "Put aside list size: " << app->putAsideListSize() << '\n';
    out << "Redelivery list size: " << app->redeliveryListSize() << '\n';
    out << "Number of messages: "
        << d_queueState_p->storage()->numMessages(app->d_appKey) << '\n';
    out << "Number of bytes: "
        << d_queueState_p->storage()->numBytes(app->d_appKey) << "\n\n";

    // Log consumer subscriptions
    mqbblp::Routers::QueueRoutingContext& routingContext =
        app->d_routing_sp->d_queue;
    mqbcmd::Routing routing;
    routingContext.loadInternals(&routing);
    const bsl::vector<mqbcmd::SubscriptionGroup>& subscrGroups =
        routing.subscriptionGroups();

    // Limit to log only k_EXPR_NUM_LIMIT expressions
    static const size_t k_EXPR_NUM_LIMIT = 50;
    ss.reset();
    size_t exprNum = 0;
    for (bsl::vector<mqbcmd::SubscriptionGroup>::const_iterator cIt =
             subscrGroups.begin();
         cIt != subscrGroups.end() && exprNum < k_EXPR_NUM_LIMIT;
         ++cIt) {
        if (!cIt->expression().empty()) {
            ss << cIt->expression() << '\n';
            ++exprNum;
        }
    }
    if (exprNum) {
        if (exprNum == k_EXPR_NUM_LIMIT) {
            out << k_EXPR_NUM_LIMIT << " of "
                << " consumer subscription expressions: ";
        }
        else {
            out << "Consumer subscription expressions: ";
        }
        out << '\n' << ss.str() << '\n';
    }

    // Log the first (oldest) message in a put aside list and its properties
    if (!app->d_putAsideList.empty()) {
        bslma::ManagedPtr<mqbi::StorageIterator> storageIt_mp;
        mqbi::StorageResult::Enum                rc = storage->getIterator(
            &storageIt_mp,
            appKey,
            app->d_putAsideList.first());
        if (rc == mqbi::StorageResult::e_SUCCESS) {
            // Log timestamp
            out << "Oldest message in a 'Put aside' list:\n";
            mqbcmd::Result result;
            mqbs::StoragePrintUtil::listMessage(&result.makeMessage(),
                                                storage,
                                                *storageIt_mp);
            mqbcmd::HumanPrinter::print(out, result);
            out << '\n';
            // Log message properties
            const bsl::shared_ptr<bdlbb::Blob>& appData =
                storageIt_mp->appData();
            const bmqp::MessagePropertiesInfo& logic =
                storageIt_mp->attributes().messagePropertiesInfo();
            bmqp::MessageProperties properties;
            int ret = properties.streamIn(*appData, logic.isExtended());
            if (!ret) {
                out << "Message Properties: " << properties << '\n';
            }
            else {
                BALL_LOG_WARN << "Failed to streamIn MessageProperties, rc = "
                              << rc;
            }
        }
        else {
            BALL_LOG_WARN << "Failed to get storage iterator for GUID: "
                          << app->d_putAsideList.first() << ", rc = " << rc;
        }
    }

    // Print the 10 oldest messages in the queue
    static const int k_NUM_MSGS = 10;
    const int        level = 0, spacesPerLevel = 2;

    out << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
        << k_NUM_MSGS << " oldest messages in the queue:\n";

    mqbcmd::Result result;
    mqbs::StoragePrintUtil::listMessages(&result.makeQueueContents(),
                                         appId,
                                         0,
                                         k_NUM_MSGS,
                                         storage);
    mqbcmd::HumanPrinter::print(out, result);

    // Print the current head of the queue
    out << mwcu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
        << "Current head of the queue:\n";

    mqbs::StoragePrintUtil::listMessage(&result.makeMessage(), storage, *head);

    mqbcmd::HumanPrinter::print(out, result);
    out << "\n";

    MWCTSK_ALARMLOG_ALARM("QUEUE_STUCK") << out.str() << MWCTSK_ALARMLOG_END;

    return true;
}

void RootQueueEngine::afterAppIdRegistered(
    const mqbi::Storage::AppIdKeyPair& appIdKeyPair)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (!d_isFanout) {
        BALL_LOG_ERROR << "RootQueueEngine::afterAppIdRegistered() should "
                       << "never be called for a non-Fanout queue.  Received "
                       << "call to register appId '" << appIdKeyPair.first
                       << "', appKey '" << appIdKeyPair.second << "'.";

        return;  // RETURN;
    }

    // We need to handle 2 scenarios here: a consumer with the specified
    // 'appId' may have already opened the queue, or otherwise.

    const bsl::string&      appId  = appIdKeyPair.first;
    const mqbu::StorageKey& appKey = appIdKeyPair.second;
    if (d_queueState_p->isCSLModeEnabled()) {
        BSLS_ASSERT_SAFE(!appKey.isNull());
    }
    else {
        BSLS_ASSERT_SAFE(appKey.isNull());
    }

    Apps::iterator   iter = d_apps.findByKey1(appId);
    mqbu::StorageKey key;

    if (iter == d_apps.end()) {
        // No consumer has opened the queue with 'appId'.
        if (d_queueState_p->isCSLModeEnabled()) {
            key = appKey;
        }
        else {
            // TODO_CSL Remove this snippet when pre-CSL workflow has been
            // retired from all clusters.

            key = d_queueState_p->appKeyGenerator()->generateAppKey(
                appId,
                d_queueState_p->partitionId());
        }

        iter = makeSubStream(appId,
                             AppKeyCount(key, 0),
                             true,
                             false,
                             bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID);
    }
    else {
        // A consumer has already opened the queue with 'appId'.
        if (d_queueState_p->isCSLModeEnabled()) {
            // Two possible scenarios:
            //
            // 1. When a consumer opened the queue with an unauthorized appId,
            //    we inserted an item having a pair of (appId, nullKey) as its
            //    key to d_apps.  Now, we replace the nullKey with the actual
            //    queue key from the appId registration.
            //
            // 2. The appId was previously registered and then unregistered.
            //    However, the old appId/appKey pair is still lingering because
            //    during RootQueueEngine::afterAppIdUnregistered() we still
            //    keep the app but invalidate the iterator.  If not all
            //    consumers using the old appId have closed the queue, then
            //    there will still be an "ghost" entry in d_apps.  For this
            //    very reason, we *cannot* assert that the appKey we have is a
            //    nullKey.

            // Cannot assert due to Scenario (2.) above
            // BSLS_ASSERT_SAFE(iter->key2().first.isNull());

            key = appKey;

            AppStateSp consumerStateSp = iter->value();
            d_apps.erase(iter);

            iter = d_apps.insert(appId, AppKeyCount(key, 0), consumerStateSp)
                       .first;
        }
        else {
            // TODO_CSL Remove this snippet when pre-CSL workflow has been
            // retired from all clusters.

            key = iter->key2().first;
        }

        // Update the 'isAuthorized' boolean in app's state to true, now that
        // the appId has been registered.  This is critical to ensure that the
        // now-registered appId continues to be available across restarts of
        // its consumers.
        iter->value()->d_isAuthorized = true;
    }

    BSLS_ASSERT_SAFE(!key.isNull());

    d_queueState_p->storageManager()->updateQueue(
        d_queueState_p->uri(),
        d_queueState_p->key(),
        d_queueState_p->partitionId(),
        mqbi::Storage::AppIdKeyPairs(1,
                                     mqbi::Storage::AppIdKeyPair(appId, key)),
        mqbi::Storage::AppIdKeyPairs());

    bslma::ManagedPtr<mqbi::StorageIterator> storageIterMp;
    storageIterMp = d_queueState_p->storage()->getIterator(iter->key2().first);

    d_consumptionMonitor.registerSubStream(key);

    iter->value()->d_storageIter_mp = storageIterMp;
}

void RootQueueEngine::afterAppIdUnregistered(
    const mqbi::Storage::AppIdKeyPair& appIdKeyPair)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (!d_isFanout) {
        BALL_LOG_ERROR << "It should be not possible to unregister appId '"
                       << appIdKeyPair.first << "', appKey '"
                       << appIdKeyPair.second << "' for a non-Fanout queue.";

        return;  // RETURN
    }

    const bsl::string& appId = appIdKeyPair.first;
    Apps::iterator     iter  = d_apps.findByKey1(appId);
    BSLS_ASSERT_SAFE(iter != d_apps.end());

    const mqbu::StorageKey& appKey = d_queueState_p->isCSLModeEnabled()
                                         ? appIdKeyPair.second
                                         : iter->key2().first;
    BSLS_ASSERT_SAFE(iter->key2().first == appKey);

    // we still keep the app but invalidate the iterator
    iter->value()->d_storageIter_mp.load(new (*d_allocator_p)
                                             mqbs::VoidStorageIterator(),
                                         d_allocator_p);

    // Do a best effort to confirm the messages and remove the storage.  If
    // either fails, just log the condition.
    {
        const mqbi::StorageResult::Enum rc =
            d_queueState_p->storage()->removeAll(appKey);
        if (rc != mqbi::StorageResult::e_SUCCESS) {
            BALL_LOG_WARN << "#QUEUE_APPID_UNREGISTER_FAILURE "
                          << "Failed to unregister appId '" << appId
                          << "', appKey '" << appKey << "' of queue '"
                          << d_queueState_p->queue()->description()
                          << "' [reason: " << mqbi::StorageResult::toAscii(rc)
                          << "]";
        }
    }

    d_queueState_p->storageManager()->updateQueue(
        d_queueState_p->uri(),
        d_queueState_p->key(),
        d_queueState_p->partitionId(),
        mqbi::Storage::AppIdKeyPairs(),
        mqbi::Storage::AppIdKeyPairs(1,
                                     mqbi::Storage::AppIdKeyPair(appId,
                                                                 appKey)));
    // No need to log in case of failure because 'updateQueue' does it (even in
    // case of success FTM).

    d_consumptionMonitor.unregisterSubStream(appKey);
}

mqbi::StorageResult::Enum RootQueueEngine::evaluateAutoSubscriptions(
    const bmqp::PutHeader&              putHeader,
    const bsl::shared_ptr<bdlbb::Blob>& appData,
    const bmqp::MessagePropertiesInfo&  mpi,
    bsls::Types::Uint64                 timestamp)
{
    if (!d_hasAutoSubscriptions) {
        // No-op if no auto subscriptions configured
        return mqbi::StorageResult::e_SUCCESS;
    }

    mqbi::StorageResult::Enum result = mqbi::StorageResult::e_SUCCESS;

    Routers::QueueRoutingContext& queue = d_queueState_p->routingContext();

    // 'setPropertiesReader' is done in 'QueueRoutingContext' ctor

    queue.d_preader->next(appData, mpi);

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(&Routers::MessagePropertiesReader::clear,
                             queue.d_preader));

    d_queueState_p->storage()->selectForAutoConfirming(
        putHeader.messageGUID());

    for (Apps::iterator it = d_apps.begin(); it != d_apps.end(); ++it) {
        AppStateSp& app = it->value();
        if (!app->evaluateAutoSubcription()) {
            result = d_queueState_p->storage()->autoConfirm(it->key2().first,
                                                            timestamp);

            if (result != mqbi::StorageResult::e_SUCCESS) {
                return result;
            }
        }
    }

    return result;
}

// ACCESSORS
//   (virtual mqbi::QueueEngine)
unsigned int RootQueueEngine::messageReferenceCount() const
{
    unsigned refCount = d_isFanout ? d_queueState_p->domain()
                                         ->config()

                                         .mode()
                                         .fanout()
                                         .appIDs()
                                         .size()
                                   : 1;

    unsigned int numNegative = d_queueState_p->storage()->numAutoConfirms();

    BSLS_ASSERT_SAFE(numNegative <= refCount);

    return refCount - numNegative;
}

void RootQueueEngine::loadInternals(mqbcmd::QueueEngine* out) const
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    mqbcmd::FanoutQueueEngine& fanoutQueueEngine = out->makeFanout();
    // TODO: Implement in a way that makes sense
    const mqbconfm::Domain& domainConfig =
        d_queueState_p->queue()->domain()->config();

    fanoutQueueEngine.mode() = d_queueState_p->queue()
                                   ->domain()
                                   ->config()

                                   .mode()
                                   .selectionName();
    fanoutQueueEngine.maxConsumers() = domainConfig.maxConsumers();
    Apps& consumerStatesRef          = const_cast<Apps&>(d_apps);

    bsl::vector<mqbcmd::ConsumerState>& consumerStates =
        fanoutQueueEngine.consumerStates();
    consumerStates.reserve(consumerStatesRef.size());
    for (Apps::iterator iter = consumerStatesRef.begin(); iter != d_apps.end();
         ++iter) {
        consumerStates.resize(consumerStates.size() + 1);
        mqbcmd::ConsumerState& consumerState = consumerStates.back();
        consumerState.appId()                = iter->key1();

        if (d_queueState_p->storage()->hasVirtualStorage(iter->key1())) {
            consumerState.isAtEndOfStorage().makeValue(
                iter->value()->d_storageIter_mp->atEnd());
            consumerState.status() = (!iter->value()->hasConsumers()
                                          ? mqbcmd::ConsumerStatus::REGISTERED
                                          : mqbcmd::ConsumerStatus::ALIVE);
        }
        else {
            consumerState.status() = mqbcmd::ConsumerStatus::UNAUTHORIZED;
        }

        iter->value()->loadInternals(&consumerState.appState());
    }

    d_queueState_p->routingContext().loadInternals(
        &fanoutQueueEngine.routing());
}

bool RootQueueEngine::hasHandle(const bsl::string& appId,
                                mqbi::QueueHandle* handle) const
{
    Apps::iterator iter =
        const_cast<RootQueueEngine*>(this)->d_apps.findByKey1(appId);

    return (iter != d_apps.end() && iter->value()->find(handle));
}

}  // close package namespace
}  // close enterprise namespace
