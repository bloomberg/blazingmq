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

// mqbblp_queueenginetester.cpp                                       -*-C++-*-
#include <mqbblp_queueenginetester.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_queue.h>
#include <mqbi_storage.h>
#include <mqbs_inmemorystorage.h>
#include <mqbs_storageutil.h>
#include <mqbu_messageguidutil.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqimp_queuemanager.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>
#include <bmqp_queueid.h>
#include <bmqp_queueutil.h>
#include <bmqp_routingconfigurationutils.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_string.h>
#include <bdlb_tokenizer.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_utility.h>
#include <bslmt_once.h>
#include <bsls_types.h>
#include <bslstl_stringref.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const char k_LOG_CATEGORY[] = "MQBBLP.QUEUEENGINETESTER";

/// Constant representing the numeric value of an invalid, or unassigned
/// queue id.  This value should be used for the id of a queue that is used
/// for testing purposes (specificially, as the `id` of `mqbblp::QueueState`
/// and `bmqp_ctrlmsg::OpenQueueOld`).
const unsigned int k_UNASSIGNED_QUEUE_ID =
    bmqp::QueueId::k_UNASSIGNED_QUEUE_ID;

/// Constant representing a null QueueKey for a queue. This value should be
/// used for the key of a queue that is used for testing purposes
/// (specificially, as the `key` of `mqbblp::QueueState` and `queueKey` of
/// `mqbs::InMemoryStorage`).
const mqbu::StorageKey k_NULL_QUEUE_KEY = mqbu::StorageKey::k_NULL_KEY;

/// Constant representing a valid PartitionId for a queue.  This value
/// should be used for the key of a queue that is used for testing purposes
/// (specifically, as the `partitionId` of a `mqbblp::QueueState`).
const int k_PARTITION_ID = 0;

// CONSTANTS

/// Constant representing the default initializer value for
/// maxUnconfirmedMessages of a queue's stream parameters.
const int k_MAX_UNCONFIRMED_MESSAGES_DEFAULT_INITIAILIZER = 1000;

/// Constant representing the default initializer value for
/// maxUnconfirmedBytes of a queue's stream parameters.
const int k_MAX_UNCONFIRMED_BYTES_DEFAULT_INITIAILIZER = 32 * 1024 * 1024;

/// When the tester posts messages, it assigns incremental value as the
/// message timestamp.  When the tester needs to garbage collect first N
/// messages, it specifies (TTL + N + 1) as the elapsed seconds argument.
/// The TTL must be greater than total number of messages the tester can
/// post.
const size_t k_MAX_MESSAGES = 64000;

// STATIC HELPER FUNCTIONS

/// Populate the specified `handleOut` with the specified `handleIn` and log
/// the specified `status` and `requestedHandleParameters`.
static void dummyGetHandleCallback(
    mqbi::QueueHandle**                        handleOut,
    const bmqp_ctrlmsg::QueueHandleParameters& requestedHandleParameters,
    const bmqp_ctrlmsg::Status&                status,
    mqbi::QueueHandle*                         handleIn)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(handleOut && "'handleOut' must not be null");

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
    BALL_LOG_TRACE << "'getHandle' callback invoked "
                   << "[requestedHandleParameters: "
                   << requestedHandleParameters << ", status: " << status
                   << "]";

    *handleOut = handleIn;
}

/// A dummy callback provided to `QueueEngine::configureHandle()` populating
/// the specified `rc` with the result status code of the configureHandle
/// operation.  Note that since `QueueEngine::configureHandle()` has void
/// return type, this method is the only place where the QueueEngineTester
/// can do post-processing on the `status` of the configureHandle operation
/// in a single thread environment.
static void dummyHandleConfiguredCallback(
    int*                                  rc,
    const bmqp_ctrlmsg::Status&           status,
    const bmqp_ctrlmsg::StreamParameters& streamParameters,
    mqbi::QueueHandle*                    handle)
{
    BSLS_ASSERT_OPT(rc);

    if (handle) {
        BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);
        BALL_LOG_TRACE << "handle with client " << handle->client()
                       << ": configured handle "
                       << "[ handle parameters: " << handle->handleParameters()
                       << ", stream parameters: " << streamParameters << "]";
    }

    *rc = status.code();
}

/// Populate the specified `messages` with messages parsed from the
/// specified `messagesStr`.  The format of `messagesStr` must be:
///   `<msg1>,<msg2>,...,<msgN>`
///
/// The behavior is undefined unless `messages` is formatted as above.  Note
/// that `messages` will be cleared.
static void parseMessages(bsl::vector<bsl::string>* messages,
                          bsl::string               messagesStr)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(messages);

    messages->clear();

    bdlb::Tokenizer tokenizer(messagesStr, " ", ",");

    BSLS_ASSERT_OPT(tokenizer.isValid() && "Format error in 'messages'");

    while (tokenizer.isValid()) {
        messages->push_back(tokenizer.token());
        ++tokenizer;
    }
}

/// Populate the specified `handleParams` with attributes (e.g. `readCount`)
/// parsed from the specified `attributesStr`, if any, and return the number
/// of attributes parsed.  Additionally, populate the optionally specified
/// `isFinal` flag with the value of the `isFinal` attribute if provided in
/// `attributesStr`.  The format of `attributesStr` must be such that each
/// attribute key (e.g. `readCount`) is followed by one hard delimiter
/// (e.g. `=`) and then the attribute value (e.g. `2`), and key-value pairs
/// are seperated by at least one soft delimiter (e.g. ` `) and no hard
/// delimiters: `[readCount=<N>] [writeCount=<M>]` Note that if there are no
/// attributes to be parsed from `attributesStr`, then `handleParams`
/// attributes are populated with default values.  The format of attributes
/// must be as follows:
///
///   "[readCount=<N>] [writeCount=<M>] [isFinal=(true|false)]"
///
/// E.g.:
///
///   "readCount=2 writeCount=1 isFinal=true"
static int
parseHandleParameters(bmqp_ctrlmsg::QueueHandleParameters* handleParams,
                      bool*                                isFinal,
                      const bsl::string&                   attributesStr)
{
    BSLS_ASSERT_OPT(handleParams && "Must provide 'handleParams'");

    bdlb::Tokenizer tokenizer(attributesStr, " ", "=");

    handleParams->qId()        = k_UNASSIGNED_QUEUE_ID;
    handleParams->flags()      = bmqt::QueueFlagsUtil::empty();
    handleParams->readCount()  = 0;
    handleParams->writeCount() = 0;
    handleParams->adminCount() = 0;
    if (isFinal) {
        *isFinal = false;
    }

    int numAttributes = 0;
    while (tokenizer.isValid()) {
        BSLS_ASSERT_OPT(!tokenizer.isPreviousHard() &&
                        tokenizer.isTrailingHard() &&
                        "Format error in 'clientText'");

        const bslstl::StringRef& attribute = tokenizer.token();

        if (bdlb::String::areEqualCaseless("readCount", attribute)) {
            BSLS_ASSERT_OPT(handleParams->readCount() == 0 &&
                            "Duplicate readCount in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            handleParams->readCount() = bsl::stoi(tokenizer.token());
            bmqt::QueueFlagsUtil::setReader(&handleParams->flags());

            ++tokenizer;
        }
        else if (bdlb::String::areEqualCaseless("writeCount", attribute)) {
            BSLS_ASSERT_OPT(handleParams->writeCount() == 0 &&
                            "Duplicate writeCount in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            handleParams->writeCount() = bsl::stoi(tokenizer.token());
            bmqt::QueueFlagsUtil::setWriter(&handleParams->flags());

            ++tokenizer;
        }
        else if (bdlb::String::areEqualCaseless("isFinal", attribute)) {
            BSLS_ASSERT_OPT(isFinal &&
                            "'isFinal' must be provided if the attribute "
                            "is specified");
            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            *isFinal = bdlb::String::areEqualCaseless(tokenizer.token(),
                                                      "true");

            ++tokenizer;
        }
        else {
            BSLS_ASSERT_OPT(false && "Format error in 'clientText'");
        }

        ++numAttributes;
    }

    return numAttributes;
}

/// Populate the specified `streamParams` with attributes parsed from the
/// specified `attributesStr`, if any, and return the number of attributes
/// parsed.  The format of `attributesStr` must be such that each attribute
/// key (e.g. `consumerPriority`) is followed by one hard delimiter
/// (e.g. `=`) and then the attribute value (e.g. `2`), and key-value pairs
/// are seperated by at least one soft delimiter (e.g. ` `) and no hard
/// delimiters:
///   '[consumerPriority=<P>] [consumerPriorityCount=<C>]
///    [maxUnconfirmedMessages=<M>] [maxUnconfirmedBytes=<B>]'
/// The behavior is undefined unless `attributesStr` is formatted as above.
static int parseStreamParameters(bmqp_ctrlmsg::StreamParameters* streamParams,
                                 const bsl::string&              attributesStr)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(streamParams && "Must provide 'streamParameters'");

    streamParams->subscriptions().resize(1);
    streamParams->subscriptions()[0].consumers().resize(1);

    bdlb::Tokenizer             tokenizer(attributesStr, " ", "=");
    bmqp_ctrlmsg::ConsumerInfo& info =
        streamParams->subscriptions()[0].consumers()[0];
    if (attributesStr.empty()) {
        info.maxUnconfirmedMessages() = 1024;
        info.maxUnconfirmedBytes()    = 32 * 1024 * 1024;
        return 0;  // RETURN
    }

    info.maxUnconfirmedMessages() =
        k_MAX_UNCONFIRMED_MESSAGES_DEFAULT_INITIAILIZER;
    info.maxUnconfirmedBytes() = k_MAX_UNCONFIRMED_BYTES_DEFAULT_INITIAILIZER;
    info.consumerPriority()    = bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;
    info.consumerPriorityCount() = -1;

    int numAttributes = 0;
    while (tokenizer.isValid()) {
        BSLS_ASSERT_OPT(!tokenizer.isPreviousHard() &&
                        tokenizer.isTrailingHard() &&
                        "Format error in 'clientText'");

        const bslstl::StringRef& attribute = tokenizer.token();

        if (bdlb::String::areEqualCaseless("consumerPriority", attribute)) {
            BSLS_ASSERT_OPT(info.consumerPriority() ==
                                bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID &&
                            "Duplicate consumerPriority in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            info.consumerPriority() = bsl::stoi(tokenizer.token());

            ++tokenizer;
        }
        else if (bdlb::String::areEqualCaseless("consumerPriorityCount",
                                                attribute)) {
            BSLS_ASSERT_OPT(
                info.consumerPriorityCount() == -1 &&
                "Duplicate consumerPriorityCount in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            info.consumerPriorityCount() = bsl::stoi(tokenizer.token());

            ++tokenizer;
        }
        else if (bdlb::String::areEqualCaseless("maxUnconfirmedMessages",
                                                attribute)) {
            BSLS_ASSERT_OPT(
                info.maxUnconfirmedMessages() ==
                    k_MAX_UNCONFIRMED_MESSAGES_DEFAULT_INITIAILIZER &&
                "Duplicate maxUnconfirmedMessages in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            info.maxUnconfirmedMessages() = bsl::stoll(tokenizer.token());

            ++tokenizer;
        }
        else if (bdlb::String::areEqualCaseless("maxUnconfirmedBytes",
                                                attribute)) {
            BSLS_ASSERT_OPT(
                info.maxUnconfirmedBytes() ==
                    k_MAX_UNCONFIRMED_BYTES_DEFAULT_INITIAILIZER &&
                "Duplicate maxUnconfirmedBytes in 'attributesStr'");

            ++tokenizer;
            BSLS_ASSERT_OPT(!tokenizer.isTrailingHard());

            info.maxUnconfirmedBytes() = bsl::stoll(tokenizer.token());

            ++tokenizer;
        }
        else {
            BSLS_ASSERT_OPT(false && "Format error in 'clientText'");
        }

        ++numAttributes;
    }

    return numAttributes;
}

}  // close unnamed namespace

// -----------------------
// class QueueEngineTester
// -----------------------

// CREATORS
QueueEngineTester::QueueEngineTester(const mqbconfm::Domain& domainConfig,
                                     bool                    startScheduler,
                                     bslma::Allocator*       allocator)
: d_invalidGuid()
, d_bufferFactory(1024, allocator)
, d_handles(allocator)
, d_subIds(allocator)
, d_clientContexts(allocator)
, d_postedMessages(allocator)
, d_newMessages(allocator)
, d_clientCounter(1)
, d_subQueueIdCounter(bmqimp::QueueManager::k_INITIAL_SUBQUEUE_ID)
, d_deletedHandles(allocator)
, d_messageCount(0)
, d_allocator_p(allocator)
{
    oneTimeInit();

    mqbconfm::Domain config = domainConfig;

    config.deduplicationTimeMs() = 0;  // No history
    config.messageTtl()          = k_MAX_MESSAGES;

    init(config, startScheduler);
}

QueueEngineTester::~QueueEngineTester()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_handles.empty());
    BSLS_ASSERT_OPT(d_clientContexts.empty());

    d_deletedHandles.clear();
    d_mockDomain_mp->unregisterQueue(d_mockQueue_sp.get());
    d_mockCluster_mp->stop();
    oneTimeShutdown();
}

// PRIVATE MANIPULATORS
void QueueEngineTester::oneTimeInit()
{
    BSLMT_ONCE_DO
    {
        bmqsys::Time::initialize();
        // Make MessageGUID generation thread-safe by calling initialize
        mqbu::MessageGUIDUtil::initialize();
    }
}

void QueueEngineTester::init(const mqbconfm::Domain& domainConfig,
                             bool                    startScheduler)
{
    bmqu::MemOutStream errorDescription(d_allocator_p);
    int                rc = 0;

    // Dispatcher
    d_mockDispatcher_mp.load(new (*d_allocator_p)
                                 mqbmock::Dispatcher(d_allocator_p),
                             d_allocator_p);
    d_mockDispatcher_mp->_setInDispatcherThread(true);

    d_mockDispatcherClient_mp.load(
        new (*d_allocator_p) mqbmock::DispatcherClient(d_allocator_p),
        d_allocator_p);
    d_mockDispatcherClient_mp->_setDescription("test.tsk:1");

    // Cluster
    d_mockCluster_mp.load(
        new (*d_allocator_p) mqbmock::Cluster(&d_bufferFactory, d_allocator_p),
        d_allocator_p);
    d_mockCluster_mp->_setIsClusterMember(true);

    BSLS_ASSERT_OPT(d_mockCluster_mp->isClusterMember());

    if (startScheduler) {
        rc = d_mockCluster_mp->start(errorDescription);
        BSLS_ASSERT_OPT(rc == 0);
    }

    // Domain
    d_mockDomain_mp.load(new (*d_allocator_p)
                             mqbmock::Domain(d_mockCluster_mp.get(),
                                             d_allocator_p),
                         d_allocator_p);

    mqbconfm::Domain domainDef(d_allocator_p);
    domainDef.name() = "my.domain";
    // TODO: domainDef.version() = "0.0 (b1.0)";
    domainDef = domainConfig;
    rc        = d_mockDomain_mp->configure(errorDescription, domainDef);
    BSLS_ASSERT_OPT(rc == 0);

    BSLS_ASSERT_OPT(d_mockDomain_mp->cluster() == d_mockCluster_mp.get());

    // Queue
    d_mockQueue_sp.createInplace(d_allocator_p,
                                 d_mockDomain_mp.get(),
                                 d_allocator_p);

    d_mockQueue_sp->_setDispatcher(d_mockDispatcher_mp.get());

    // Register queue in domain
    bslma::ManagedPtr<mqbi::Queue> queueMp(d_mockQueue_sp.managedPtr());
    rc = d_mockDomain_mp->registerQueue(errorDescription, queueMp);
    BSLS_ASSERT_OPT(rc == 0);

    // VALIDATION
    BSLS_ASSERT_OPT(d_mockQueue_sp->domain() == d_mockDomain_mp.get());
    BSLS_ASSERT_OPT(d_mockQueue_sp->dispatcher() == d_mockDispatcher_mp.get());

    bsl::shared_ptr<mqbi::Queue> queueSp;
    rc = d_mockDomain_mp->lookupQueue(&queueSp, d_mockQueue_sp->uri());
    BSLS_ASSERT_OPT(rc == 0);
    BSLS_ASSERT_OPT(queueSp.get() == d_mockQueue_sp.get());

    // Simple sanity check on the mock dispatcher
    BSLS_ASSERT_OPT(
        d_mockDispatcher_mp->inDispatcherThread(d_mockQueue_sp.get()));
    BSLS_ASSERT_OPT(
        d_mockDispatcher_mp->inDispatcherThread(d_mockCluster_mp.get()));

    // Queue State
    d_queueState_mp.load(new (*d_allocator_p)
                             mqbblp::QueueState(d_mockQueue_sp.get(),
                                                d_mockQueue_sp->uri(),
                                                k_UNASSIGNED_QUEUE_ID,
                                                k_NULL_QUEUE_KEY,
                                                k_PARTITION_ID,
                                                d_mockDomain_mp.get(),
                                                d_mockCluster_mp->_resources(),
                                                d_allocator_p),
                         d_allocator_p);

    bmqp_ctrlmsg::RoutingConfiguration routingConfig;

    if (domainConfig.mode().isBroadcastValue()) {
        bmqp::RoutingConfigurationUtils::setDeliverAll(&routingConfig);
        bmqp::RoutingConfigurationUtils::setAtMostOnce(&routingConfig);
        bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(
            &routingConfig);
        d_mockQueue_sp->_setAtMostOnce(true);
        d_mockQueue_sp->_setDeliverAll(true);
    }
    else if (domainConfig.mode().isPriorityValue()) {
        bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(
            &routingConfig);
    }
    else if (domainConfig.mode().isFanoutValue()) {
        bmqp::RoutingConfigurationUtils::setHasMultipleSubStreams(
            &routingConfig);
        bmqp::RoutingConfigurationUtils::setDeliverConsumerPriority(
            &routingConfig);
        d_mockQueue_sp->_setHasMultipleSubStreams(true);
    }
    d_queueState_mp->setRoutingConfig(routingConfig);

    // Create Storage

    mqbi::Storage* storage_p = new (*d_allocator_p)
        mqbs::InMemoryStorage(d_mockQueue_sp->uri(),
                              k_NULL_QUEUE_KEY,
                              k_PARTITION_ID,
                              domainConfig,
                              d_mockDomain_mp->capacityMeter(),
                              d_allocator_p);

    mqbconfm::Storage config;
    config.makeInMemory();

    mqbconfm::Limits limits;
    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();

    storage_p->setConsistency(domainConfig.consistency());
    rc = storage_p->configure(errorDescription,
                              config,
                              limits,
                              domainConfig.messageTtl(),
                              domainConfig.maxDeliveryAttempts());
    BSLS_ASSERT_OPT(rc == 0 && "storage configure fail");

    // Add virtual storages
    const bool isFanoutMode = domainConfig.mode().isFanoutValue();
    if (isFanoutMode) {
        const bsl::vector<bsl::string>& cfgAppIds =
            domainConfig.mode().fanout().appIDs();
        bsl::unordered_set<mqbu::StorageKey> appKeys;
        for (bsl::vector<bsl::string>::size_type i = 0; i < cfgAppIds.size();
             ++i) {
            mqbu::StorageKey appKey;
            mqbs::StorageUtil::generateStorageKey(&appKey,
                                                  &appKeys,
                                                  cfgAppIds[i]);
            BSLS_ASSERT_SAFE(!appKey.isNull());
            BSLS_ASSERT_SAFE(appKeys.find(appKey) != appKeys.end());

            rc = storage_p->addVirtualStorage(errorDescription,
                                              cfgAppIds[i],
                                              appKey);
            BSLS_ASSERT_SAFE(rc == 0);
        }
    }
    else {
        rc = storage_p->addVirtualStorage(
            errorDescription,
            bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
            mqbi::QueueEngine::k_DEFAULT_APP_KEY);
        BSLS_ASSERT_SAFE(rc == 0);
    }

    d_mockQueue_sp->_setStorage(storage_p);
    storage_p->setQueue(d_mockQueue_sp.get());

    bslma::ManagedPtr<mqbi::Storage> storage_mp(storage_p, d_allocator_p);
    d_queueState_mp->setStorage(storage_mp);

    // Verify
    BSLS_ASSERT_OPT(d_mockQueue_sp->storage() == storage_p);
    BSLS_ASSERT_OPT(d_queueState_mp->queue() == d_mockQueue_sp.get());
    BSLS_ASSERT_OPT(d_queueState_mp->storage() == storage_p);

    // Create and set the Mock Handle Factory on the Queue Handle Catalog
    bslma::ManagedPtr<mqbi::QueueHandleFactory> handleFactory_mp;
    handleFactory_mp.load(new (*d_allocator_p) mqbmock::HandleFactory(),
                          d_allocator_p);

    const_cast<QueueHandleCatalog&>(d_queueState_mp->handleCatalog())
        .setHandleFactory(handleFactory_mp);
}

void QueueEngineTester::oneTimeShutdown()
{
    BSLMT_ONCE_DO
    {
        bmqsys::Time::shutdown();
    }
}

void QueueEngineTester::handleReleasedCallback(
    int*                                      rc,
    bool*                                     isDeletedOutput,
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const bsl::string&                        clientKey)
{
    BSLS_ASSERT_OPT(rc);
    BSLS_ASSERT_OPT(isDeletedOutput);

    dummyHandleReleasedCallback(rc, handle, result, clientKey);

    *isDeletedOutput = result.hasNoHandleClients();
}

void QueueEngineTester::dummyHandleReleasedCallback(
    int*                                      rc,
    const bsl::shared_ptr<mqbi::QueueHandle>& handle,
    const mqbi::QueueHandleReleaseResult&     result,
    const bsl::string&                        clientKey)
{
    BSLS_ASSERT_OPT(rc);

    BALL_LOG_TRACE << "handle with client " << handle->client()
                   << ": released handle "
                   << "[result: " << result
                   << ", handle parameters: " << handle->handleParameters()
                   << "]";

    if (result.hasNoHandleClients()) {
        BSLS_ASSERT_OPT(handle && "'isDeleted' is indicated but "
                                  "handle is null!");

        // Erase from maps of active handles and push back the shared ptr into
        // a vector of deleted handles to prolong the life of the object
        d_deletedHandles.push_back(handle);
        d_handles.erase(clientKey);
        d_clientContexts.erase(clientKey);
    }

    // releaseHandle operation succeeded if 'handle' is not null
    *rc = (handle.get() == 0);
}

// MANIPULATORS
mqbmock::QueueHandle*
QueueEngineTester::getHandle(const bsl::string& clientText)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    // 1. Parse 'clientText'
    bsl::string::size_type attributesPos = clientText.find_first_of("@ ");
    BSLS_ASSERT_OPT(attributesPos != bsl::string::npos);

    // 1.1 'clientKey'
    const bsl::string clientKey(clientText.substr(0, attributesPos),
                                d_allocator_p);

    // TODO: Refactor this chunk into a function (possibly all of 1.2, 1.3,
    //       and before 2. could all go into a function)
    // 1.2 appId (if any)
    bsl::string            appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    bsl::string::size_type appIdStartPosition = clientText.find_first_of('@');
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        appIdStartPosition += 1;
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());
        bsl::string::size_type appIdEndPosition =
            clientText.find_first_of(' ', appIdStartPosition);
        BSLS_ASSERT_OPT(appIdEndPosition != bsl::string::npos);

        bsl::string::size_type appIdLength = appIdEndPosition -
                                             appIdStartPosition;
        BSLS_ASSERT_OPT(appIdLength > 0);

        appId         = clientText.substr(appIdStartPosition, appIdLength);
        attributesPos = appIdEndPosition;
    }

    // 1.3. Handle Parameters
    bmqp_ctrlmsg::QueueHandleParameters handleParams(d_allocator_p);
    bmqt::UriBuilder uriBuilder(d_mockQueue_sp->uri(), d_allocator_p);
    BSLS_ASSERT_OPT(parseHandleParameters(&handleParams,
                                          0,
                                          clientText.substr(attributesPos)) >
                    0);

    // Consistency
    // NOTE: appId can only be specified in fanout mode, and it must be
    //       associated with a readCount of 1
    const mqbconfm::Domain& domainConfig = d_queueState_mp->domain()->config();
    const bool              isFanout     = domainConfig.mode().isFanoutValue();
    unsigned int upstreamSubQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    // Populate SubQueueIdInfo (if any)
    if (isFanout) {
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);

        uriBuilder.setId(appId);

        if (appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
            // use the same (unique per appId) id as both downstream and
            // upstream subQueueId.
            unsigned int subQueueId;
            if (subIdCiter != d_subIds.end()) {
                subQueueId = subIdCiter->second;
            }
            else {
                d_subIds.insert(bsl::make_pair(appId, d_subQueueIdCounter));
                subQueueId = d_subQueueIdCounter;

                ++d_subQueueIdCounter;
            }
            upstreamSubQueueId = subQueueId;

            bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo =
                handleParams.subIdInfo().makeValue();
            subQueueIdInfo.appId() = appId;
            subQueueIdInfo.subId() = subQueueId;
        }
        else {
            upstreamSubQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
        }
    }

    // Set the uri on the 'handleParameters'
    bmqt::Uri uri(d_allocator_p);
    int       rc = uriBuilder.uri(&uri);
    BSLS_ASSERT_OPT(rc == 0);

    handleParams.uri() = uri.asString();

    // 2. Get the handle
    bmqu::MemOutStream  errorDescription(d_allocator_p);
    mqbi::QueueHandle*  handle     = 0;
    HandleMap::iterator handleIter = d_handles.find(clientKey);
    if (handleIter != d_handles.end()) {
        // Already aware of this client
        ClientContextMap::const_iterator clientContextCiter =
            d_clientContexts.find(clientKey);
        BSLS_ASSERT_OPT(clientContextCiter != d_clientContexts.cend());

        const ClientContextSp& clientContext = clientContextCiter->second;

        mqbi::QueueHandle::GetHandleCallback callback = bdlf::BindUtil::bindS(
            d_allocator_p,
            &dummyGetHandleCallback,
            &handle,
            handleParams,
            bdlf::PlaceHolders::_1,   // status
            bdlf::PlaceHolders::_2);  // handle

        d_mockQueue_sp->getHandle(clientContext,
                                  handleParams,
                                  upstreamSubQueueId,
                                  callback);
    }
    else {
        // This is a new client, we need to create client data for it
        BSLS_ASSERT_OPT(d_clientContexts.find(clientKey) ==
                        d_clientContexts.end());

        // Client data
        ClientContextSp clientContext(
            new (*d_allocator_p)
                mqbi::QueueHandleRequesterContext(d_allocator_p),
            d_allocator_p);
        clientContext->setClient(d_mockDispatcherClient_mp.get())
            .setDescription("test.tsk:1")
            .setIsClusterMember(true)
            .setRequesterId(mqbi::QueueHandleRequesterContext ::
                                generateUniqueRequesterId());

        mqbi::QueueHandle::GetHandleCallback callback = bdlf::BindUtil::bindS(
            d_allocator_p,
            &dummyGetHandleCallback,
            &handle,
            handleParams,
            bdlf::PlaceHolders::_1,
            bdlf::PlaceHolders::_2);

        d_mockQueue_sp->getHandle(clientContext,
                                  handleParams,
                                  upstreamSubQueueId,
                                  callback);

        mqbmock::QueueHandle* mHandle = dynamic_cast<mqbmock::QueueHandle*>(
            handle);
        // Update internal state
        ++d_clientCounter;
        d_handles.insert(bsl::make_pair(clientKey, mHandle));
        d_clientContexts.insert(bsl::make_pair(clientKey, clientContext));
    }

    mqbmock::QueueHandle* mockHandle = dynamic_cast<mqbmock::QueueHandle*>(
        handle);

    return mockHandle;
}

int QueueEngineTester::configureHandle(const bsl::string& clientText)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    // 1. Parse 'clientText'
    bsl::string::size_type attributesPos = clientText.find_first_of("@ ");
    BSLS_ASSERT_OPT(attributesPos != bsl::string::npos);

    // 1.1 'clientKey'
    const bsl::string clientKey(clientText.substr(0, attributesPos),
                                d_allocator_p);

    // TODO: Refactor this chunk into a function (possibly all of 1.2, 1.3,
    //       and before 2. could all go into a function)
    // 1.2 appId (if any)
    bsl::string            appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    bsl::string::size_type appIdStartPosition = clientText.find_first_of('@');
    bsl::string::size_type appIdEndPosition   = attributesPos;
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        appIdStartPosition += 1;
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());

        appIdEndPosition = clientText.find_first_of(' ', appIdStartPosition);
        if (appIdEndPosition == bsl::string::npos) {
            // No attributes specified in the input
            appId = clientText.substr(appIdStartPosition);
        }
        else {
            const bsl::string::size_type appIdLength = appIdEndPosition -
                                                       appIdStartPosition;
            BSLS_ASSERT_OPT(appIdLength > 0);
            appId = clientText.substr(appIdStartPosition, appIdLength);
        }
    }

    // 1.3 attributes
    bsl::string attributes = "";
    if (appIdEndPosition != bsl::string::npos) {
        attributes = clientText.substr(appIdEndPosition);
    }

    // Consistency
    // NOTE: appId can only be specified in fanout mode
    const mqbconfm::Domain& domainConfig = d_queueState_mp->domain()->config();
    const bool              isFanout     = domainConfig.mode().isFanoutValue();
    BSLS_ASSERT_OPT((appId == bmqp::ProtocolUtil::k_DEFAULT_APP_ID) ||
                    isFanout);

    // Stream Parameters
    bmqp_ctrlmsg::StreamParameters streamParams(d_allocator_p);
    parseStreamParameters(&streamParams, attributes);

    // Populate SubQueueIdInfo (if any)
    if (isFanout) {
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        if (subIdCiter == d_subIds.end()) {
            d_subIds.insert(bsl::make_pair(appId, d_subQueueIdCounter));

            ++d_subQueueIdCounter;
        }
        streamParams.appId() = appId;

        // Temporary
        // Assume, RelayQueueEngine will use upstreramSubQueueIds as the
        // subscriptionIds.  This needs to be in accord with the 'post' logic.
        BSLS_ASSERT_SAFE(streamParams.subscriptions().size() == 1);

        streamParams.subscriptions()[0].sId() = subIdCiter->second;
    }

    // 2. Configure the handle
    mqbi::QueueHandle* handle = client(clientKey);

    int rc = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    const mqbi::QueueHandle::HandleConfiguredCallback configuredCb =
        bdlf::BindUtil::bindS(d_allocator_p,
                              &dummyHandleConfiguredCallback,
                              &rc,
                              bdlf::PlaceHolders::_1,  // status
                              bdlf::PlaceHolders::_2,  // streamParameters
                              handle);

    handle->configure(streamParams, configuredCb);

    return rc;
}

mqbmock::QueueHandle*
QueueEngineTester::client(const bslstl::StringRef& clientKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");
    BSLS_ASSERT_OPT(d_handles.find(clientKey) != d_handles.end());

    return d_handles.find(clientKey)->second;
}

void QueueEngineTester::post(const bslstl::StringRef& messages,
                             RelayQueueEngine*        downstream)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    bsl::vector<bsl::string> msgs(d_allocator_p);
    parseMessages(&msgs, messages);

    for (unsigned int i = 0; i < msgs.size(); ++i) {
        // Each message must have its own 'subscriptions'.
        bmqp::Protocol::SubQueueInfosArray subscriptions(d_allocator_p);

        if (d_subIds.empty()) {
            subscriptions.push_back(
                bmqp::SubQueueInfo(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID));
        }
        else {
            // Assume, RelayQueueEngine will use upstreamSubQueueIds as the
            // subscriptionIds.
            // This needs to be in accord with the 'configureHandle' logic.

            for (SubIdsMap::const_iterator cit = d_subIds.cbegin();
                 cit != d_subIds.cend();
                 ++cit) {
                subscriptions.push_back(bmqp::SubQueueInfo(cit->second));
            }
        }

        // Put in storage
        bmqt::MessageGUID              msgGUID;
        mqbi::StorageMessageAttributes msgAttributes;
        bsl::shared_ptr<bdlbb::Blob>   appData;
        bsl::shared_ptr<bdlbb::Blob>   options;

        ++d_messageCount;
        BSLS_ASSERT_SAFE(d_messageCount < k_MAX_MESSAGES);

        mqbu::MessageGUIDUtil::generateGUID(&msgGUID);
        msgAttributes.setRefCount(d_queueEngine_mp->messageReferenceCount());
        msgAttributes.setArrivalTimestamp(d_messageCount);

        appData.createInplace(d_allocator_p, &d_bufferFactory, d_allocator_p);
        bdlbb::BlobUtil::append(appData.get(),
                                msgs[i].data(),
                                msgs[i].length());

        // Consider this non-Proxy.  Imitate replication or Primary PUT
        // ('d_mockCluster_mp->_setIsClusterMember(true)')

        int rc = d_queueState_mp->storage()->put(&msgAttributes,
                                                 msgGUID,
                                                 appData,
                                                 options);
        BSLS_ASSERT_OPT((rc == 0) && "Storage put failure");

        if (downstream) {
            downstream->push(&msgAttributes,
                             msgGUID,
                             bsl::shared_ptr<bdlbb::Blob>(),
                             subscriptions,
                             false);
        }

        // Insert into messages maps
        bsl::pair<MessagesMap::iterator, bool> insertRC =
            d_postedMessages.insert(
                bsl::make_pair(bsl::string(msgs[i], d_allocator_p), msgGUID));
        BSLS_ASSERT_OPT(insertRC.second && "Message previously posted");

        insertRC = d_newMessages.insert(
            bsl::make_pair(bsl::string(msgs[i], d_allocator_p), msgGUID));
        BSLS_ASSERT_OPT(insertRC.second);
    }
}

void QueueEngineTester::afterNewMessage(const int numMessages)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");
    BSLS_ASSERT_OPT(numMessages >= 0 && "'numMessages' must be positive");
    BSLS_ASSERT_OPT(static_cast<unsigned int>(numMessages) <=
                    d_newMessages.size());

    int remaining = numMessages == 0 ? d_newMessages.size() : numMessages;

    for (MessagesMap::iterator it = d_newMessages.begin();
         (it != d_newMessages.end()) && (remaining > 0);
         --remaining) {
        const bmqt::MessageGUID& msgGUID = it->second;
        BSLS_ASSERT_OPT(!msgGUID.isUnset());

        // NOTE: At the time of this writing, only the 'RelayQueueEngine' uses
        //       the 'bmqt::MessageGUID msgGUID' parameter.
        //
        // NOTE: At the time of this writing, none of the Queue Engines use the
        //       'mqbi::QueueHandle source' parameter, but if that changes then
        //       we will need to keep track of that.
        d_queueEngine_mp->afterNewMessage(msgGUID, 0);

        // Advance and delete from 'd_newMessages'
        it = d_newMessages.erase(it);
    }
}

void QueueEngineTester::confirm(const bsl::string&       clientText,
                                const bslstl::StringRef& messages)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");
    // BSLS_ASSERT_OPT(client(clientKey) && "Unknown client 'clientKey'");

    // 1. Parse 'clientText'
    bsl::string::size_type appIdStartPosition = clientText.find_first_of("@");
    BSLS_ASSERT_OPT(clientText.find_first_of(' ', appIdStartPosition) ==
                    bsl::string::npos);
    ;

    // 1.1 'clientKey'
    const bsl::string clientKey(clientText.substr(0, appIdStartPosition),
                                d_allocator_p);

    // TODO: Refactor this chunk into a function
    // 1.2 appId (if any)
    bsl::string appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());
        appIdStartPosition += 1;
        appId = clientText.substr(appIdStartPosition);
    }

    // Extract subQueueId (if any)
    unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    if (appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        if (subIdCiter != d_subIds.end()) {
            subQueueId = subIdCiter->second;
        }
    }

    mqbmock::QueueHandle*    mockHandle = client(clientKey);
    bsl::vector<bsl::string> msgs(d_allocator_p);
    parseMessages(&msgs, messages);
    for (bsl::vector<bsl::string>::size_type i = 0; i < msgs.size(); ++i) {
        MessagesMap::iterator it = d_postedMessages.find(msgs[i]);

        if (it != d_postedMessages.end()) {
            // msgGUID was found
            const bmqt::MessageGUID& msgGUID = it->second;
            BSLS_ASSERT_OPT(!msgGUID.isUnset());

            mockHandle->confirmMessage(msgGUID, subQueueId);
        }
        else {
            // This message was never posted - this is a "legal" testing
            // scenario.
            mockHandle->confirmMessage(d_invalidGuid, subQueueId);
        }
    }
}

void QueueEngineTester::reject(const bsl::string&       clientText,
                               const bslstl::StringRef& messages)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    // 1. Parse 'clientText'
    bsl::string::size_type appIdStartPosition = clientText.find_first_of("@");
    BSLS_ASSERT_OPT(clientText.find_first_of(' ', appIdStartPosition) ==
                    bsl::string::npos);
    ;

    // 1.1 'clientKey'
    const bsl::string clientKey(clientText.substr(0, appIdStartPosition),
                                d_allocator_p);

    // TODO: Refactor this chunk into a function
    // 1.2 appId (if any)
    bsl::string appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());
        appIdStartPosition += 1;
        appId = clientText.substr(appIdStartPosition);
    }

    // Extract subQueueId (if any)
    unsigned int subQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    if (appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        if (subIdCiter != d_subIds.end()) {
            subQueueId = subIdCiter->second;
        }
    }

    mqbmock::QueueHandle*    mockHandle = client(clientKey);
    bsl::vector<bsl::string> msgs(d_allocator_p);
    parseMessages(&msgs, messages);
    for (bsl::vector<bsl::string>::size_type i = 0; i < msgs.size(); ++i) {
        MessagesMap::iterator it = d_postedMessages.find(msgs[i]);

        if (it != d_postedMessages.end()) {
            // msgGUID was found
            const bmqt::MessageGUID& msgGUID = it->second;
            BSLS_ASSERT_OPT(!msgGUID.isUnset());

            mockHandle->rejectMessage(msgGUID, subQueueId);
        }
        else {
            // This message was never posted - this is a "legal" testing
            // scenario.
            mockHandle->rejectMessage(d_invalidGuid, subQueueId);
        }
    }
}

void QueueEngineTester::garbageCollectMessages(const int numMessages)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");
    BSLS_ASSERT_OPT(numMessages >= 0 && "'numMessages' must be positive");
    BSLS_ASSERT_OPT(static_cast<unsigned int>(numMessages) <=
                    d_postedMessages.size());

    bsls::Types::Uint64 latestGcMsgTimestampEpoch = 0;
    bsls::Types::Int64  configuredTtlValue        = 0;

    int removing  = numMessages == 0 ? d_postedMessages.size() : numMessages;
    int remaining = removing;

    for (MessagesMap::iterator it = d_postedMessages.begin();
         (it != d_postedMessages.end()) && (remaining > 0);
         --remaining) {
        const bmqt::MessageGUID& msgGUID = it->second;
        BSLS_ASSERT_OPT(!msgGUID.isUnset());

        // Notify the queue engine since the storage does not know about the
        // queue
        d_queueEngine_mp->beforeMessageRemoved(msgGUID);

        // Advance and delete from 'd_newMessages'
        it = d_postedMessages.erase(it);
    }

    d_queueState_mp->storage()->gcExpiredMessages(&latestGcMsgTimestampEpoch,
                                                  &configuredTtlValue,
                                                  k_MAX_MESSAGES + removing +
                                                      1);
}

void QueueEngineTester::purgeQueue(const bslstl::StringRef& appId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    mqbi::StorageResult::Enum rc     = mqbi::StorageResult::e_SUCCESS;
    mqbu::StorageKey          appKey = mqbi::QueueEngine::k_DEFAULT_APP_KEY;

    bsl::string appIdInternal = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    const mqbconfm::Domain& domainConfig = d_queueState_mp->domain()->config();
    const bool              isFanout     = domainConfig.mode().isFanoutValue();
    if (isFanout) {
        appIdInternal = appId;
        appKey        = mqbu::StorageKey::k_NULL_KEY;
    }

    if (appIdInternal != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        // A specific subStream was specified
        if (!appIdInternal.empty()) {
            const bool isVStorageFound =
                d_queueState_mp->storage()->hasVirtualStorage(appIdInternal,
                                                              &appKey);
            BSLS_ASSERT_OPT(isVStorageFound &&
                            "Requested to purgeQueue for a subStream that"
                            " was not found");
        }
    }

    // Remove all messages from physical/in-memory storage
    rc = d_queueState_mp->storage()->removeAll(appKey);
    BSLS_ASSERT_OPT((rc == mqbi::StorageResult::e_SUCCESS) &&
                    "'msgGUID' was not found in physical/in-memory"
                    " storage");

    // Notify the Queue Engine
    if (appKey.isNull()) {
        d_queueEngine_mp->afterQueuePurged(bmqp::ProtocolUtil::k_NULL_APP_ID,
                                           appKey);
    }
    else {
        d_queueEngine_mp->afterQueuePurged(appIdInternal, appKey);
    }
}

int QueueEngineTester::releaseHandle(const bsl::string& clientText)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    // 1. Parse 'clientText'
    bsl::string::size_type attributesPos = clientText.find_first_of(" @");
    BSLS_ASSERT_OPT(attributesPos != bsl::string::npos);

    // 1.1 'clientKey'
    const bsl::string  clientKey(clientText.substr(0, attributesPos),
                                d_allocator_p);
    mqbi::QueueHandle* handle = client(clientKey);

    // TODO: Refactor this chunk into a function (possibly all of 1.2, 1.3,
    //       and before 2. could all go into a function)
    // 1.2 appId (if any)
    bsl::string            appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    bsl::string::size_type appIdStartPosition = clientText.find_first_of('@');
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        appIdStartPosition += 1;
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());
        bsl::string::size_type appIdEndPosition =
            clientText.find_first_of(' ', appIdStartPosition);
        BSLS_ASSERT_OPT(appIdEndPosition != bsl::string::npos);

        bsl::string::size_type appIdLength = appIdEndPosition -
                                             appIdStartPosition;
        BSLS_ASSERT_OPT(appIdLength > 0);

        appId         = clientText.substr(appIdStartPosition, appIdLength);
        attributesPos = appIdEndPosition;
    }

    // 1.3. Handle Parameters
    bmqp_ctrlmsg::QueueHandleParameters handleParams(d_allocator_p);
    bmqt::UriBuilder uriBuilder(d_mockQueue_sp->uri(), d_allocator_p);
    bool             isFinal = false;
    BSLS_ASSERT_OPT(parseHandleParameters(&handleParams,
                                          &isFinal,
                                          clientText.substr(attributesPos)) >
                    0);

    // Consistency
    // NOTE: Either FanoutQueueEngine and appId was specified along with a
    //       readCount of 1, or not FanoutQueueEngine and appId was not
    //       specified

    // Populate SubQueueIdInfo (if any)
    if (appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo =
            handleParams.subIdInfo().makeValue();
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        if (subIdCiter != d_subIds.end()) {
            subQueueIdInfo.appId() = appId;
            subQueueIdInfo.subId() = subIdCiter->second;
            uriBuilder.setId(appId);
        }
        else {
            d_subIds.insert(bsl::make_pair(appId, d_subQueueIdCounter));
            subQueueIdInfo.appId() = appId;
            subQueueIdInfo.subId() = d_subQueueIdCounter;
            ++d_subQueueIdCounter;

            uriBuilder.setId(appId);
        }
    }

    // Set the uri on the 'handleParameters'
    bmqt::Uri uri(d_allocator_p);
    int       rc = uriBuilder.uri(&uri);
    BSLS_ASSERT_OPT(rc == 0);

    handleParams.uri() = uri.asString();

    // 2. Release
    rc = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    const mqbi::QueueHandle::HandleReleasedCallback releasedCb =
        bdlf::BindUtil::bindS(d_allocator_p,
                              &QueueEngineTester::dummyHandleReleasedCallback,
                              this,
                              &rc,
                              bdlf::PlaceHolders::_1,  // handle
                              bdlf::PlaceHolders::_2,  // isDeleted
                              clientKey);

    d_queueEngine_mp->releaseHandle(handle, handleParams, isFinal, releasedCb);

    return rc;
}

int QueueEngineTester::releaseHandle(const bsl::string& clientText,
                                     bool*              isDeleted)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    // 1. Parse 'clientText'
    bsl::string::size_type attributesPos = clientText.find_first_of(" @");
    BSLS_ASSERT_OPT(attributesPos != bsl::string::npos);

    // 1.1 'clientKey'
    const bsl::string  clientKey(clientText.substr(0, attributesPos),
                                d_allocator_p);
    mqbi::QueueHandle* handle = client(clientKey);

    // TODO: Refactor this chunk into a function (possibly all of 1.2, 1.3,
    //       and before 2. could all go into a function)
    // 1.2 appId (if any)
    bsl::string            appId = bmqp::ProtocolUtil::k_DEFAULT_APP_ID;
    bsl::string::size_type appIdStartPosition = clientText.find_first_of('@');
    if (appIdStartPosition != bsl::string::npos) {
        // appId was specified.  Skip over the '@', capture the appId.
        appIdStartPosition += 1;
        BSLS_ASSERT_OPT(appIdStartPosition != clientText.size());
        bsl::string::size_type appIdEndPosition =
            clientText.find_first_of(' ', appIdStartPosition);
        BSLS_ASSERT_OPT(appIdEndPosition != bsl::string::npos);

        bsl::string::size_type appIdLength = appIdEndPosition -
                                             appIdStartPosition;
        BSLS_ASSERT_OPT(appIdLength > 0);

        appId         = clientText.substr(appIdStartPosition, appIdLength);
        attributesPos = appIdEndPosition;
    }

    // 1.3. Handle Parameters
    bmqp_ctrlmsg::QueueHandleParameters handleParams(d_allocator_p);
    bmqt::UriBuilder uriBuilder(d_mockQueue_sp->uri(), d_allocator_p);
    bool             isFinal = false;
    BSLS_ASSERT_OPT(parseHandleParameters(&handleParams,
                                          &isFinal,
                                          clientText.substr(attributesPos)) >
                    0);

    // Consistency
    // NOTE: Either FanoutQueueEngine and appId was specified along with a
    //       readCount of 1, or not FanoutQueueEngine and appId was not
    //       specified

    // Populate SubQueueIdInfo (if any)
    if (appId != bmqp::ProtocolUtil::k_DEFAULT_APP_ID) {
        bmqp_ctrlmsg::SubQueueIdInfo& subQueueIdInfo =
            handleParams.subIdInfo().makeValue();
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        subQueueIdInfo.appId()               = appId;
        if (subIdCiter != d_subIds.end()) {
            subQueueIdInfo.subId() = subIdCiter->second;
        }
        else {
            d_subIds.insert(bsl::make_pair(appId, d_subQueueIdCounter));
            subQueueIdInfo.subId() = d_subQueueIdCounter;

            ++d_subQueueIdCounter;
        }

        uriBuilder.setId(appId);
    }

    // Set the uri on the 'handleParameters'
    bmqt::Uri uri(d_allocator_p);
    int       rc = uriBuilder.uri(&uri);
    BSLS_ASSERT_OPT(rc == 0);

    handleParams.uri() = uri.asString();

    // 2. Release
    rc = bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    const mqbi::QueueHandle::HandleReleasedCallback releasedCb =
        bdlf::BindUtil::bindS(d_allocator_p,
                              &QueueEngineTester::handleReleasedCallback,
                              this,
                              &rc,
                              isDeleted,
                              bdlf::PlaceHolders::_1,  // handle
                              bdlf::PlaceHolders::_2,  // isDeleted
                              clientKey);

    d_queueEngine_mp->releaseHandle(handle, handleParams, isFinal, releasedCb);

    return rc;
}

void QueueEngineTester::dropHandle(const bslstl::StringRef& clientKey)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");
    BSLS_ASSERT_OPT(d_handles.find(clientKey) != d_handles.end());
    BSLS_ASSERT_OPT(d_clientContexts.find(clientKey) !=
                    d_clientContexts.end());

    mqbmock::QueueHandle* handle = client(clientKey);
    if (handle != 0) {
        handle->clearClient(true);
        handle->drop();
    }

    BSLS_ASSERT_OPT(!d_queueState_mp->handleCatalog().hasHandle(handle) &&
                    "Handle was not dropped");
    BSLS_ASSERT_OPT(d_handles.erase(clientKey) == 1);
    BSLS_ASSERT_OPT(d_clientContexts.erase(clientKey) == 1);
}

void QueueEngineTester::dropHandles()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_mp &&
                    "'createQueueEngine()' was not called");

    HandleMap::iterator iter = d_handles.begin();
    while (iter != d_handles.end()) {
        BSLS_ASSERT_OPT(d_clientContexts.find(iter->first) !=
                            d_clientContexts.end() &&
                        "No client data corresponding to handle");

        const bsl::string clientKey(iter->first, d_allocator_p);
        ++iter;

        dropHandle(clientKey);
    }

    // Reset internal state
    d_handles.clear();
    d_clientContexts.clear();
}

bool QueueEngineTester::getUpstreamParameters(
    bmqp_ctrlmsg::StreamParameters* value,
    const bslstl::StringRef&        appId) const
{
    unsigned int upstreamSubQueueId = bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
    const mqbconfm::Domain& domainConfig = d_queueState_mp->domain()->config();
    const bool              isFanout     = domainConfig.mode().isFanoutValue();

    if (isFanout) {
        SubIdsMap::const_iterator subIdCiter = d_subIds.find(appId);
        BSLS_ASSERT_SAFE(subIdCiter != d_subIds.end());

        upstreamSubQueueId = subIdCiter->second;
    }

    return d_queueState_mp->getUpstreamParameters(value, upstreamSubQueueId);
}

// --------------------------
// struct QueueEngineTestUtil
// --------------------------

bsl::string QueueEngineTestUtil::getMessages(bsl::string messages,
                                             bsl::string indices)
{
    bsl::string              result("");
    bsl::vector<int>         idxs;
    bsl::vector<bsl::string> msgs;
    parseMessages(&msgs, messages);

    // Parse indices
    bdlb::Tokenizer tokenizer(indices, " ", ",");
    BSLS_ASSERT_OPT(tokenizer.isValid() && "format error in 'indices'");

    while (tokenizer.isValid()) {
        int index = bsl::stoi(tokenizer.token());
        BSLS_ASSERT_OPT(index >= 0 &&
                        static_cast<bsl::vector<bsl::string>::size_type>(
                            index) < msgs.size());

        idxs.push_back(index);

        ++tokenizer;
    }

    // Build output
    for (bsl::vector<int>::size_type i = 0; i < idxs.size(); ++i) {
        int index = idxs[i];
        result += msgs[index];

        if (i != idxs.size() - 1) {
            result += ",";
        }
    }

    return result;
}

}  // close namespace mqbblp
}  // close namespace BloombergLP
