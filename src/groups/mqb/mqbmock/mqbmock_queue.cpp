// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbmock_queue.cpp                                                  -*-C++-*-
#include <mqbmock_queue.h>

#include <mqbscm_version.h>
// MQB
#include <bmqp_queueutil.h>
#include <mqbcmd_messages.h>
#include <mqbi_domain.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbmock_queuehandle.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bsl_iostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbmock {

// -----------
// class Queue
// -----------

// CREATORS
Queue::Queue(mqbi::Domain* domain, bslma::Allocator* allocator)
: d_uri("bmq://bmq.test.local/test_queue", allocator)
, d_description(allocator)
, d_atMostOnce(false)
, d_deliverAll(false)
, d_hasMultipleSubStreams(false)
, d_handleParameters(allocator)
, d_streamParameters(allocator)
, d_stats()
, d_domain_p(domain)
, d_dispatcher_p(0)
, d_queueEngine_p(0)
, d_storage_p(0)
, d_schemaLearner(allocator)
{
    BSLS_ASSERT_SAFE(d_uri.isValid());

    mwcu::MemOutStream ss(allocator);
    ss << "|mock-queue|" << d_uri.asString();
    d_description.assign(ss.str());

    // Initialize stats
    if (domain) {
        d_stats.initialize(d_uri, domain, allocator);
    }
}

Queue::~Queue()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual: mqbi::DispatcherClient)
mqbi::Dispatcher* Queue::dispatcher()
{
    return d_dispatcher_p;
}

mqbi::DispatcherClientData& Queue::dispatcherClientData()
{
    return d_dispatcherClientData;
}

void Queue::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    if (d_dispatcherEventHandler) {
        d_dispatcherEventHandler(event);
    }
}

void Queue::flush()
{
    // NOTHING
}

// MANIPULATORS
//   (virtual: mqbi::Queue)
int Queue::configure(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
                     BSLS_ANNOTATION_UNUSED bool          isReconfigure,
                     BSLS_ANNOTATION_UNUSED bool          wait)
{
    return 0;
}

void Queue::getHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the *QUEUE_DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_queueEngine_p && "Queue Engine has not been set");
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    d_queueEngine_p->getHandle(clientContext,
                               handleParameters,
                               upstreamSubQueueId,
                               callback);
}

void Queue::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *QUEUE_DISPATCHER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_queueEngine_p && "Queue Engine has not been set");

    d_queueEngine_p->configureHandle(handle, streamParameters, configuredCb);
}

void Queue::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    d_queueEngine_p->releaseHandle(handle,
                                   handleParameters,
                                   isFinal,
                                   releasedCb);
}

void Queue::dropHandle(mqbi::QueueHandle* handle, bool doDeconfigure)
{
    // executed by the *QUEUE_DISPATCHER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(
        handle->queue()->dispatcher()->inDispatcherThread(handle->queue()));
    BSLS_ASSERT_SAFE(d_queueEngine_p && "Queue Engine has not been set");

    // Since the handle is being dropped (which typically occurs if a client is
    // stopping without explicitly closing its queues, or if a client crashes),
    // we need to configure the handle's stream with null (e.g.,
    // 'consumerPriority == k_CONSUMER_PRIORITY_INVALID') parameters before we
    // release the handle.  'configureHandle' call is eventually processed by
    // the queue engine, which may choose to simply ignore this request if
    // handle's stream parameters are already zero (which could occur if a
    // client crashed immediately after sending a close-queue request).

    // Execute the 'configureHandle' & 'releaseHandle' sequence to drop each
    // subStream of the handle in turn.

    mqbi::QueueHandle::SubStreams::const_iterator citer =
        handle->subStreamInfos().begin();
    bool isFinal = (citer == handle->subStreamInfos().end());
    while (!isFinal) {
        const bsl::string& appId = citer->first;

        BSLS_ASSERT_SAFE(appId != bmqp::ProtocolUtil::k_NULL_APP_ID);

        bmqp_ctrlmsg::SubQueueIdInfo         subStreamInfo;
        const mqbi::QueueHandle::StreamInfo& info = citer->second;
        subStreamInfo.appId()                     = appId;
        subStreamInfo.subId() = info.d_downstreamSubQueueId;

        bmqp_ctrlmsg::QueueHandleParameters consumerHandleParams =
            bmqp::QueueUtil::createHandleParameters(handle->handleParameters(),
                                                    subStreamInfo,
                                                    info.d_counts.d_readCount);
        if (doDeconfigure) {
            bmqp_ctrlmsg::StreamParameters nullStreamParameters;
            nullStreamParameters.appId() = appId;

            configureHandle(handle,
                            nullStreamParameters,
                            mqbi::QueueHandle::HandleConfiguredCallback());
        }

        // Set 'isFinal' when releasing the last subStream of this handle
        isFinal = ((++citer) == handle->subStreamInfos().end());

        BALL_LOG_INFO << "For queue [" << handle->queue()->description()
                      << "] and handle [" << handle->client() << ":"
                      << handle->id() << "] "
                      << "having [handleParamerers: "
                      << handle->handleParameters() << "], dropping subStream "
                      << "[" << subStreamInfo << "] having [streamParameters: "
                      << info.d_streamParameters
                      << "]. 'isFinal' flag: " << bsl::boolalpha << isFinal
                      << ".";

        // 'releaseHandle' erases from 'handle->subStreamInfos()' invalidating
        // the iterator.
        releaseHandle(handle,
                      consumerHandleParams,
                      isFinal,  // isFinal flag
                      mqbi::QueueHandle::HandleReleasedCallback());
    }
}

void Queue::close()
{
    // NOTHING
}

mqbu::CapacityMeter* Queue::capacityMeter()
{
    return d_storage_p->capacityMeter();
}

mqbi::QueueEngine* Queue::queueEngine()
{
    return d_queueEngine_p;
}

mqbstat::QueueStatsDomain* Queue::stats()
{
    return &d_stats;
}

bsls::Types::Int64
Queue::countUnconfirmed(BSLS_ANNOTATION_UNUSED unsigned int subId)
{
    // NOT IMPLENTED
    return 0;
}

void Queue::onPushMessage(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& options,
    BSLS_ANNOTATION_UNUSED const bmqp::MessagePropertiesInfo&
                                 hasMessageProperties,
    BSLS_ANNOTATION_UNUSED       bmqt::CompressionAlgorithmType::Enum
                                 compressionAlgorithmType)
{
    // NOTHING
}

void Queue::confirmMessage(const bmqt::MessageGUID& msgGUID,
                           unsigned int             upstreamSubQueueId,
                           mqbi::QueueHandle*       source)
// NOTE: While an 'mqbblp::Queue' could internally refer to both a remote
//       and local queue, only the local queue implementation is provided
//       below. In the future, can add a method '_setIsRemote(bool)' and
//       have an if statement below.
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_OPT(d_queueEngine_p && "Queue Engine has not been set");

    BALL_LOG_TRACE << "confirmMessage [queue: '" << description()
                   << "', client: '" << source->client() << "', GUID: '"
                   << msgGUID << "', appKey: " << upstreamSubQueueId << "']";

    int rc = d_queueEngine_p->onConfirmMessage(source,
                                               msgGUID,
                                               upstreamSubQueueId);

    if (rc == 1) {
        // Still some references to that message, we can't delete it.
        return;  // RETURN
    }
    BSLS_ASSERT_OPT(!(rc > 0));
    // Engine returned zero on success or non-negative error value.

    if (rc < 0) {
        // Engine failed to process confirm message.  Some of the possible
        // reasons:
        // 1) GUID doesn't exist.
        // 2) Partition is unavailable (full).
        // 3) Self node is shutting down.
        // 4) Etc...
        return;  // RETURN
    }

    // Since there are no references, there should be no app holding msgGUID
    // and no need to call `beforeMessageRemoved`

    // OK to delete the message.

    if (d_storage_p) {
        int msgSize = 0;
        d_storage_p->remove(msgGUID, &msgSize);
    }
}

int Queue::rejectMessage(const bmqt::MessageGUID& msgGUID,
                         unsigned int             upstreamSubQueueId,
                         mqbi::QueueHandle*       source)
// NOTE: While an 'mqbblp::Queue' could internally refer to both a remote
//       and local queue, only the local queue implementation is provided
//       below. In the future, can add a method '_setIsRemote(bool)' and
//       have an if statement below.
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_OPT(d_queueEngine_p && "Queue Engine has not been set");
    BSLS_ASSERT_OPT(d_storage_p && "Storage has not been set");

    BALL_LOG_TRACE << "rejectMessage [queue: '" << description()
                   << "', client: '" << source->client() << "', GUID: '"
                   << msgGUID << "', appKey: " << upstreamSubQueueId << "']";

    int rc = d_queueEngine_p->onRejectMessage(source,
                                              msgGUID,
                                              upstreamSubQueueId);

    return rc;
}

void Queue::onAckMessage(
    BSLS_ANNOTATION_UNUSED const bmqp::AckMessage& ackMessage)
{
    // NOTHING
}

void Queue::onLostUpstream()
{
    // NOTHING
}

void Queue::onOpenUpstream(BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 genCount,
                           BSLS_ANNOTATION_UNUSED unsigned int subQueueId)
{
    // NOTHING
}

void Queue::onOpenFailure(BSLS_ANNOTATION_UNUSED unsigned int subQueueId)
{
    // NOTHING
}

void Queue::onReceipt(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* qH,
    BSLS_ANNOTATION_UNUSED const bsls::Types::Int64& arrivalTimepoint)
{
    // NOTHING
}

void Queue::onRemoval(BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
                      BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* qH,
                      BSLS_ANNOTATION_UNUSED bmqt::AckResult::Enum result)
{
    // NOTHING
}

void Queue::onReplicatedBatch()
{
    // NOTHING
}

int Queue::processCommand(mqbcmd::QueueResult*        result,
                          const mqbcmd::QueueCommand& command)
{
    mqbcmd::Error      error = result->makeError();
    mwcu::MemOutStream os;
    os << "MockQueue::processCommand " << command << " not implemented!";
    error.message() = os.str();
    return -1;
}

void Queue::purge(BSLS_ANNOTATION_UNUSED mqbcmd::PurgeQueueResult* queueResult,
                  BSLS_ANNOTATION_UNUSED const bsl::string& appId)
{
    // NOT IMPLENTED
}

// MANIPULATORS
//   (specific to mqbmock::Queue)
Queue& Queue::_setDispatcher(mqbi::Dispatcher* value)
{
    d_dispatcher_p = value;
    return *this;
}

Queue& Queue::_setQueueEngine(mqbi::QueueEngine* value)
{
    d_queueEngine_p = value;
    return *this;
}

Queue& Queue::_setStorage(mqbi::Storage* value)
{
    d_storage_p = value;
    return *this;
}

Queue& Queue::_setAtMostOnce(const bool value)
{
    d_atMostOnce = value;
    return *this;
}

Queue& Queue::_setDeliverAll(const bool value)
{
    d_deliverAll = value;
    return *this;
}

Queue& Queue::_setHasMultipleSubStreams(const bool value)
{
    d_hasMultipleSubStreams = value;
    return *this;
}

Queue& Queue::_setDispatcherEventHandler(const DispatcherEventHandler& handler)
{
    d_dispatcherEventHandler = handler;
    return *this;
}

// ACCESSORS
//   (virtual: mqbi::DispatcherClient)
const mqbi::Dispatcher* Queue::dispatcher() const
{
    return d_dispatcher_p;
}

const mqbi::DispatcherClientData& Queue::dispatcherClientData() const
{
    return d_dispatcherClientData;
}

const bsl::string& Queue::description() const
{
    return d_description;
}

// ACCESSORS
//   (virtual: mqbi::Queue)
mqbi::Domain* Queue::domain() const
{
    return d_domain_p;
}

mqbi::Storage* Queue::storage() const
{
    return d_storage_p;
}

int Queue::partitionId() const
{
    return -1;
}

const bmqt::Uri& Queue::uri() const
{
    return d_uri;
}

unsigned int Queue::id() const
{
    return 0;
}

bool Queue::isAtMostOnce() const
{
    return d_atMostOnce;
}

bool Queue::isDeliverAll() const
{
    return d_deliverAll;
}

bool Queue::hasMultipleSubStreams() const
{
    return d_hasMultipleSubStreams;
}

const bmqp_ctrlmsg::QueueHandleParameters& Queue::handleParameters() const
{
    return d_handleParameters;
}

bool Queue::getUpstreamParameters(
    bmqp_ctrlmsg::StreamParameters*     value,
    BSLS_ANNOTATION_UNUSED unsigned int upstreamSubQueueId) const
{
    *value = d_streamParameters;
    return true;
}

const mqbcfg::MessageThrottleConfig& Queue::messageThrottleConfig() const
{
    return d_messageThrottleConfig;
}

bmqp::SchemaLearner& Queue::schemaLearner() const
{
    return d_schemaLearner;
}

// -------------------
// class HandleFactory
// -------------------

HandleFactory::~HandleFactory()
{
    // NOTHING
}

mqbi::QueueHandle* HandleFactory::makeHandle(
    const bsl::shared_ptr<mqbi::Queue>&                       queue,
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    mqbstat::QueueStatsDomain*                                stats,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    bslma::Allocator*                                         allocator)
{
    return new (*allocator) mqbmock::QueueHandle(queue,
                                                 clientContext,
                                                 stats,
                                                 handleParameters,
                                                 allocator);
}

}  // close package namespace
}  // close enterprise namespace
