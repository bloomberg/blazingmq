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

// mqbblp_queue.cpp                                                   -*-C++-*-
#include <mqbblp_queue.h>

#include <mqbscm_version.h>
/// Implementation Notes
///====================
// This component implements a sort of strategy design pattern where the logic
// is implemented in 'mqbblp::LocalQueue' or 'mqbblp::RemoteQueue'; depending
// on the role of the broker (proxy, replica, primary).  The type of queue
// (whether local or remote) can dynamically be changed from one to the other.
// This conversion always happens from within the queue's associated dispatcher
// thread, in order to guarantee thread safety of the operation.  Therefore, no
// method executing on a different thread should ever directly bind to the
// 'd_localQueue_mp' or the 'd_remoteQueue_mp' object, but rather it should
// first blindly be dispatched to the queue dispatcher thread, in which then
// the decision can be made depending on the current state of the object.

// MQB
#include <mqbblp_storagemanager.h>
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbs_storageprintutil.h>
#include <mqbstat_queuestats.h>

// BMQ
#include <bmqt_queueflags.h>

#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_string.h>
#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_functional.h>  // for bsl::ref()
#include <bsl_iostream.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbblp {

namespace {
const char k_LOG_CATEGORY[] = "MQBBLP.QUEUE";

/// This method performs the actual drop. It dispatches to the templated
/// (local or remote) specified `queue`.  The specified `handle` will be
/// released.
///
/// THREAD: This method is called from the Queue's dispatcher thread.
template <typename QueueType>
void doDropHandle(QueueType&         queue,
                  mqbi::QueueHandle* handle,
                  bool               doDeconfigure)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(handle);
    BSLS_ASSERT_SAFE(
        handle->queue()->dispatcher()->inDispatcherThread(handle->queue()));

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    // Since the handle is being dropped (which typically occurs if a client is
    // stopping without explicitly closing its queues, or if a client crashes),
    // we need to configure the handle's stream with null
    // (e.g. 'consumerPriority == k_CONSUMER_PRIORITY_INVALID') parameters
    // before we release the handle.  'configureHandle' call is eventually
    // processed by the queue engine, which may choose to simply ignore this
    // request if handle's stream parameters are already zero (which could
    // occur if a client crashed immediately after sending a close-queue
    // request).  Note that we must execute the 'configureHandle' &
    // 'releaseHandle' operations seperately for each subStream of the
    // 'handle'.

    // Execute the 'configureHandle' & 'releaseHandle' sequence to drop each
    // subStream of the handle in turn.

    mqbi::QueueHandle::SubStreams::const_iterator citer =
        handle->subStreamInfos().begin();
    bool isFinal = (citer == handle->subStreamInfos().end());
    while (!isFinal) {
        const bsl::string&                   appId = citer->first;
        const mqbi::QueueHandle::StreamInfo& info  = citer->second;

        BSLS_ASSERT_SAFE(appId != bmqp::ProtocolUtil::k_NULL_APP_ID);

        bmqp_ctrlmsg::SubQueueIdInfo subStreamInfo;
        subStreamInfo.appId() = appId;
        subStreamInfo.subId() = info.d_downstreamSubQueueId;

        bmqp_ctrlmsg::QueueHandleParameters consumerHandleParams =
            bmqp::QueueUtil::createHandleParameters(handle->handleParameters(),
                                                    subStreamInfo,
                                                    info.d_counts.d_readCount);
        if (doDeconfigure) {
            bmqp_ctrlmsg::StreamParameters nullStreamParameters;
            nullStreamParameters.appId() = appId;

            queue.configureHandle(
                handle,
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
        queue.releaseHandle(handle,
                            consumerHandleParams,
                            isFinal,  // isFinal flag
                            mqbi::QueueHandle::HandleReleasedCallback());
    }
}

}  // close unnamed namespace

// -----------
// class Queue
// -----------

void Queue::configureDispatched(int*          result,
                                bsl::ostream* errorDescription,
                                bool          isReconfigure)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    bmqu::MemOutStream throwaway(d_allocator_p);
    bsl::ostream&      errStream = (errorDescription ? *errorDescription
                                                     : throwaway);

    int rc = 0;
    if (d_localQueue_mp) {
        rc = d_localQueue_mp->configure(errStream, isReconfigure);
    }
    else if (d_remoteQueue_mp) {
        rc = d_remoteQueue_mp->configure(errStream, isReconfigure);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }

    if (result) {
        *result = rc;
    }
}

void Queue::getHandleDispatched(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(
        clientContext->requesterId() !=
        mqbi::QueueHandleRequesterContext::k_INVALID_REQUESTER_ID);

    if (d_localQueue_mp) {
        d_localQueue_mp->getHandle(clientContext,
                                   handleParameters,
                                   upstreamSubQueueId,
                                   callback);
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->getHandle(clientContext,
                                    handleParameters,
                                    upstreamSubQueueId,
                                    callback);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }

    updateStats();
}

void Queue::releaseHandleDispatched(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->releaseHandle(handle,
                                       handleParameters,
                                       isFinal,
                                       releasedCb);
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->releaseHandle(handle,
                                        handleParameters,
                                        isFinal,
                                        releasedCb);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }

    updateStats();
}

void Queue::dropHandleDispatched(mqbi::QueueHandle* handle, bool doDeconfigure)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (!d_state.handleCatalog().hasHandle(handle)) {
        // Specified 'handle' may have been destroyed by the time this routine
        // is invoked in the queue's dispatcher thread.  Here's how:
        // 1) Client closes the queue, and as a result, client session invokes
        //    'releaseHandle'.
        // 2) Immediately after that, client session invokes 'dropHandle'
        //    (either because client went down, stopped the session, etc).
        // 3) The 'releaseHandle' call reaches queue dispatcher thread, is
        //    processed and the 'handle' is removed from the queue catalog.
        //    The queue invokes 'onHandleReleased' callback to inform the
        //    client session with a shared_ptr of the removed handle, which is
        //    then enqueued to the client dispatcher thread.
        // 4) 'onHandleReleasedDispatched' with handle's shared_ptr is invoked
        //    in the client dispatcher thread, client session removes 'handle'
        //    from its 'd_queues' and lets go of the handle's shared_ptr, which
        //    ends up deleting the handle.
        // 5) The 'dropHandle' routine with handle's ptr is scheduled in the
        //    queue dispatcher thread, which is how we end up in this routine.

        BALL_LOG_INFO << "Skipping QueueHandle drop "
                      << "[queue: " << description()
                      << ", reason: 'unknown queueHandle, most likely already"
                      << " destroyed'].";
        return;  // RETURN
    }

    if (bmqp::QueueUtil::isEmpty(handle->handleParameters())) {
        BALL_LOG_INFO << "Skipping QueueHandle drop "
                      << "[queue: " << description()
                      << ", reason: 'queueHandle already fully released']";
        return;  // RETURN
    }

    BALL_LOG_INFO << "Dropping QueueHandle [" << handle << "] for queue ["
                  << description() << "].";

    if (d_localQueue_mp) {
        doDropHandle(*d_localQueue_mp, handle, doDeconfigure);
    }
    else if (d_remoteQueue_mp) {
        doDropHandle(*d_remoteQueue_mp, handle, doDeconfigure);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }

    updateStats();
}

void Queue::closeDispatched()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->close();
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->close();
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }

    updateStats();
}

void Queue::convertToLocalDispatched()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_remoteQueue_mp);

    BALL_LOG_INFO << d_state.uri() << ": converting to local "
                  << "[handlesCount: "
                  << d_state.handleCatalog().handlesCount()
                  << ", handle parameters: " << d_state.handleParameters()
                  << ", stream parameters: " << d_state.subQueuesParameters()
                  << "]";

    int                                   rc;
    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqu::MemOutStream                    errorDescription(&localAllocator);

    // Move remoteQueue to a temporary stack based managed pointer, so that
    // we can bypass all precondition checks (since we are temporarily
    // going to have both local and remote queue co-existing).
    bslma::ManagedPtr<RemoteQueue> remoteQueue = d_remoteQueue_mp;

    d_state.setId(bmqp::QueueId::k_PRIMARY_QUEUE_ID);
    createLocal();
    rc = d_localQueue_mp->configure(errorDescription, true);
    if (rc != 0) {
        BALL_LOG_ERROR
            << "#QUEUE_CONVERTION_FAILURE " << d_state.uri()
            << ": failed to configure localQueue during conversion [rc: " << rc
            << ", error: '" << errorDescription.str() << "']";
        BSLS_ASSERT_SAFE(false &&
                         "Failed to configure localQueue during conversion");
        return;  // RETURN
    }

    rc = d_localQueue_mp->importState(errorDescription);
    if (rc != 0) {
        BALL_LOG_ERROR << "#QUEUE_CONVERTION_FAILURE " << d_state.uri()
                       << ": failed to import state during conversion [rc: "
                       << rc << ", error: '" << errorDescription.str() << "']";
        BSLS_ASSERT_SAFE(false && "Failed to import state during conversion");
        return;  // RETURN
    }

    remoteQueue->iteratePendingMessages(
        bdlf::BindUtil::bind(&LocalQueue::postMessage,
                             d_localQueue_mp.get(),
                             bdlf::PlaceHolders::_1,    // putHeader
                             bdlf::PlaceHolders::_2,    // appData
                             bdlf::PlaceHolders::_3,    // options
                             bdlf::PlaceHolders::_4));  // source

    remoteQueue->iteratePendingConfirms(
        bdlf::BindUtil::bind(&LocalQueue::confirmMessage,
                             d_localQueue_mp.get(),
                             bdlf::PlaceHolders::_1,    // GUID
                             bdlf::PlaceHolders::_2,    // subQueueId
                             bdlf::PlaceHolders::_3));  // source

    remoteQueue->resetState();  // ResetState to prevent assert at destruction
    remoteQueue.clear();

    updateStats();

    // If some of the above (buffered) Confirms had made queue handle usable,
    // the remote queue might not be able to deliver because the old primary
    // had controlled delivery (and did not receive the confirm).
    // In this case, 'onHandleUsable' event did not trigger any delivery.
    // Now that the queue is local, nudge its delivery to cover for this case.

    d_localQueue_mp->queueEngine()->afterNewMessage(bmqt::MessageGUID(), 0);
}

void Queue::updateStats()
{
    d_state.updateStats();
}

void Queue::listMessagesDispatched(mqbcmd::QueueResult* result,
                                   const bsl::string&   appId,
                                   bsls::Types::Int64   offset,
                                   bsls::Types::Int64   count)
{
    // executed by the *QUEUE* dispatcher thread

    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (!appId.empty() && !d_state.storage()->hasVirtualStorage(appId)) {
        mqbcmd::Error& error = result->makeError();
        error.message()      = "Invalid 'LIST' command: invalid APPID";
        return;  // RETURN
    }

    mqbcmd::QueueContents& queueContents = result->makeQueueContents();
    if (0 != mqbs::StoragePrintUtil::listMessages(&queueContents,
                                                  appId,
                                                  offset,
                                                  count,
                                                  d_state.storage())) {
        mqbcmd::Error& error = result->makeError();
        error.message()      = "Internal error";
        return;  // RETURN
    }
}

void Queue::loadInternals(mqbcmd::QueueInternals* out)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    // State
    d_state.loadInternals(&out->state());

    // Queue
    mqbcmd::Queue& queue = out->queue();
    if (d_localQueue_mp) {
        d_localQueue_mp->loadInternals(&queue.makeLocalQueue());
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->loadInternals(&queue.makeRemoteQueue());
    }
    else {
        queue.makeUninitializedQueue();
    }
}

Queue::Queue(const bmqt::Uri&                          uri,
             unsigned int                              id,
             const mqbu::StorageKey&                   key,
             int                                       partitionId,
             mqbi::Domain*                             domain,
             mqbi::StorageManager*                     storageManager,
             const mqbi::ClusterResources&             resources,
             bdlmt::FixedThreadPool*                   threadPool,
             const bmqp_ctrlmsg::RoutingConfiguration& routingCfg,
             bslma::Allocator*                         allocator)
: d_allocator_p(allocator)
, d_schemaLearner(allocator)
, d_state(this, uri, id, key, partitionId, domain, resources, allocator)
, d_localQueue_mp(0)
, d_remoteQueue_mp(0)
{
    BALL_LOG_INFO << d_state.uri() << ": constructor (" << this << ")";

    const mqbcfg::MessageThrottleConfig& messageThrottleConfig =
        domain->cluster()->isClusterMember()
            ? domain->cluster()->clusterConfig()->messageThrottleConfig()
            : domain->cluster()->clusterProxyConfig()->messageThrottleConfig();

    // You should not use `domain->loadRoutingConfiguration()` to get
    // 'routingCfg' because at proxies, domains are not configured thus would
    // cause an error and only give the (irrelevant) value 0.  The correct way
    // to get 'RoutingConfiguration' is through the 'routingCfg' argument.

    // TBD: For now taking a blobBufferFactory because ClusterProxy doesn't
    // have
    //      a 'storageManager', so we can't get a blobBufferFactory out of it.
    //      StorageManager's purpose is currently wrong: every type of cluster
    //      should have a storage manager, even if they don't use FileBacked
    //      storage.

    d_state.setStorageManager(storageManager)
        .setAppKeyGenerator(storageManager)
        .setMiscWorkThreadPool(threadPool)
        .setRoutingConfig(routingCfg)
        .setMessageThrottleConfig(messageThrottleConfig);
}

Queue::~Queue()
{
    BALL_LOG_INFO << d_state.uri() << ": destructor (" << this << ")";

    dispatcher()->unregisterClient(this);
    // TBD: It should wait for flush of the dispatcher
}

void Queue::createLocal()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_localQueue_mp);
    BSLS_ASSERT_SAFE(!d_remoteQueue_mp);

    d_localQueue_mp.load(new (*d_allocator_p)
                             LocalQueue(&d_state, d_allocator_p),
                         d_allocator_p);
}

void Queue::createRemote(int                       deduplicationTimeoutMs,
                         int                       ackWindowSize,
                         RemoteQueue::StateSpPool* statePool)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_localQueue_mp);
    BSLS_ASSERT_SAFE(!d_remoteQueue_mp);

    d_remoteQueue_mp.load(new (*d_allocator_p)
                              RemoteQueue(&d_state,
                                          deduplicationTimeoutMs,
                                          ackWindowSize,
                                          statePool,
                                          d_allocator_p),
                          d_allocator_p);
}

void Queue::convertToLocal()
{
    // executed by *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&Queue::convertToLocalDispatched, this),
        this);
}

void Queue::convertToRemote()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    BSLS_ASSERT_SAFE(d_localQueue_mp);

    BALL_LOG_INFO << d_state.uri() << ": converting to remote";

    // TBD: Not yet implemented !
}

void Queue::onLostUpstream()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_remoteQueue_mp) {
        d_remoteQueue_mp->onLostUpstream();
    }
}

void Queue::onOpenFailure(unsigned int subQueueId)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_remoteQueue_mp) {
        d_remoteQueue_mp->onOpenFailure(subQueueId);
    }
}

void Queue::onOpenUpstream(bsls::Types::Uint64 genCount,
                           unsigned int        subQueueId,
                           bool                isWriterOnly)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_remoteQueue_mp) {
        d_remoteQueue_mp->onOpenUpstream(genCount, subQueueId, isWriterOnly);
    }
}

void Queue::onReceipt(const bmqt::MessageGUID&  msgGUID,
                      mqbi::QueueHandle*        queueHandle,
                      const bsls::Types::Int64& arrivalTimepoint)
{
    BSLS_ASSERT_SAFE(d_localQueue_mp);

    d_localQueue_mp->onReceipt(msgGUID, queueHandle, arrivalTimepoint);
}

void Queue::onRemoval(const bmqt::MessageGUID& msgGUID,
                      mqbi::QueueHandle*       queueHandle,
                      bmqt::AckResult::Enum    result)
{
    BSLS_ASSERT_SAFE(d_localQueue_mp);

    d_localQueue_mp->onRemoval(msgGUID, queueHandle, result);
}

void Queue::onReplicatedBatch()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->deliverIfNeeded();
    }
}

int Queue::configure(bsl::ostream& errorDescription,
                     bool          isReconfigure,
                     bool          wait)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        dispatcher()->inDispatcherThread(d_state.domain()->cluster()));

    if (!isReconfigure) {
        // Register this queue to the dispatcher.
        if (d_state.domain()->cluster()->isRemote()) {
            dispatcher()->registerClient(this,
                                         mqbi::DispatcherClientType::e_QUEUE);
        }
        else {
            dispatcher()->registerClient(
                this,
                mqbi::DispatcherClientType::e_QUEUE,
                d_state.storageManager()->processorForPartition(
                    d_state.partitionId()));
        }
    }

    // Enqueue a configure callback in the queue-dispatcher thread.
    int result = 0;
    dispatcher()->execute(
        bdlf::BindUtil::bind(&Queue::configureDispatched,
                             this,
                             &result,
                             (wait ? &errorDescription : NULL),
                             isReconfigure),
        this);
    if (!wait) {
        return 0;  // RETURN
    }

    dispatcher()->synchronize(this);
    return result;
}

void Queue::getHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(
        dispatcher()->inDispatcherThread(d_state.domain()->cluster()));

    dispatcher()->execute(bdlf::BindUtil::bind(&Queue::getHandleDispatched,
                                               this,
                                               clientContext,
                                               handleParameters,
                                               upstreamSubQueueId,
                                               callback),
                          this);
}

void Queue::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->configureHandle(handle,
                                         streamParameters,
                                         configuredCb);
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->configureHandle(handle,
                                          streamParameters,
                                          configuredCb);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }
}

void Queue::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by *ANY* thread

    dispatcher()->execute(bdlf::BindUtil::bind(&Queue::releaseHandleDispatched,
                                               this,
                                               handle,
                                               handleParameters,
                                               isFinal,
                                               releasedCb),
                          this);
}

void Queue::dropHandle(mqbi::QueueHandle* handle, bool doDeconfigure)
{
    // executed by *ANY* thread

    dispatcher()->execute(bdlf::BindUtil::bind(&Queue::dropHandleDispatched,
                                               this,
                                               handle,
                                               doDeconfigure),
                          this);
}

void Queue::close()
{
    // executed by *ANY* thread

    dispatcher()->execute(bdlf::BindUtil::bind(&Queue::closeDispatched, this),
                          this);
}

void Queue::onPushMessage(
    const bmqt::MessageGUID&             msgGUID,
    const bsl::shared_ptr<bdlbb::Blob>&  appData,
    const bsl::shared_ptr<bdlbb::Blob>&  options,
    const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    bool                                 isOutOfOrder)
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(domain()->cluster()));

    // NOTE: This routine is invoked by clusterProxy/cluster whenever it
    //       receives a PUSH message from upstream.  It should only be used on
    //       a 'remoteQueue', but since this method is executed on the CLUSTER
    //       dispatcher thread, and the Queue might be converted on the QUEUE
    //       dispatcher thread, we can not check that here, so we rely on the
    //       LocalQueue dispatcherEvent method to event warn on that invalid
    //       usage.

    mqbi::DispatcherEvent* dispEvent = dispatcher()->getEvent(this);

    (*dispEvent)
        .setSource(this)
        .makePushEvent(appData,
                       options,
                       msgGUID,
                       messagePropertiesInfo,
                       compressionAlgorithmType,
                       isOutOfOrder);

    dispatcher()->dispatchEvent(dispEvent, this);
}

void Queue::confirmMessage(const bmqt::MessageGUID& msgGUID,
                           unsigned int             upstreamSubQueueId,
                           mqbi::QueueHandle*       source)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->confirmMessage(msgGUID, upstreamSubQueueId, source);
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->confirmMessage(msgGUID, upstreamSubQueueId, source);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }
}

int Queue::rejectMessage(const bmqt::MessageGUID& msgGUID,
                         unsigned int             upstreamSubQueueId,
                         mqbi::QueueHandle*       source)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));
    int result = false;

    if (d_localQueue_mp) {
        result = d_localQueue_mp->rejectMessage(msgGUID,
                                                upstreamSubQueueId,
                                                source);
    }
    else if (d_remoteQueue_mp) {
        result = d_remoteQueue_mp->rejectMessage(msgGUID,
                                                 upstreamSubQueueId,
                                                 source);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }
    return result;
}

void Queue::onAckMessage(const bmqp::AckMessage& ackMessage)
{
    // executed by the *CLUSTER* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(domain()->cluster()));

    // NOTE: This routine is invoked by clusterProxy/cluster whenever it
    //       receives an ACK from upstream.  It should only be used on a
    //       'remoteQueue', but since this method is executed on the CLUSTER
    //       dispatcher thread, and the Queue might be converted on the QUEUE
    //       dispatcher thread, we can not check that here, so we rely on the
    //       LocalQueue dispatcherEvent method to event warn on that invalid
    //       usage.

    mqbi::DispatcherEvent* dispEvent = dispatcher()->getEvent(this);

    (*dispEvent).makeAckEvent().setAckMessage(ackMessage);

    dispatcher()->dispatchEvent(dispEvent, this);
}

int Queue::processCommand(mqbcmd::QueueResult*        result,
                          const mqbcmd::QueueCommand& command)
{
    // executed by *ANY* thread

    if (command.isPurgeAppIdValue()) {
        BSLS_ASSERT_SAFE(false && "Should not get here. PURGE QUEUE command "
                                  "must be processed on a storage level.");
    }
    else if (command.isInternalsValue()) {
        mqbcmd::QueueInternals& queueInternals = result->makeQueueInternals();
        dispatcher()->execute(
            bdlf::BindUtil::bind(&Queue::loadInternals, this, &queueInternals),
            this);
        dispatcher()->synchronize(this);
        return 0;  // RETURN
    }
    else if (command.isMessagesValue()) {
        dispatcher()->execute(
            bdlf::BindUtil::bind(&Queue::listMessagesDispatched,
                                 this,
                                 result,
                                 command.messages().appId().valueOr(""),
                                 command.messages().offset(),
                                 command.messages().count()),
            this);
        dispatcher()->synchronize(this);
        return (result->isQueueContentsValue() ? 0 : -1);  // RETURN
    }

    mqbcmd::Error&     error = result->makeError();
    bmqu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    error.message() = os.str();
    return -1;
}

void Queue::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->onDispatcherEvent(event);
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->onDispatcherEvent(event);
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }
}

void Queue::flush()
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (d_localQueue_mp) {
        d_localQueue_mp->flush();
    }
    else if (d_remoteQueue_mp) {
        d_remoteQueue_mp->flush();
    }
    else {
        BSLS_ASSERT_OPT(false && "Uninitialized queue");
    }
}

bsls::Types::Int64 Queue::countUnconfirmed(unsigned int subId)
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(this));

    if (subId == bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID) {
        return d_state.handleCatalog().countUnconfirmed();  // RETURN
    }

    // TODO(shutdown-v2): TEMPORARY, remove when all switch to StopRequest V2.
    struct local {
        static void sum(bsls::Types::Int64*                  sum,
                        mqbi::QueueHandle*                   handle,
                        const mqbi::QueueHandle::StreamInfo& info,
                        unsigned int                         sample)
        {
            if (info.d_downstreamSubQueueId == sample) {
                *sum += handle->countUnconfirmed();
            }
        }
    };
    bsls::Types::Int64 result = 0;

    d_state.handleCatalog().iterateConsumers(
        bdlf::BindUtil::bind(&local::sum,
                             &result,
                             bdlf::PlaceHolders::_1,  // handle
                             bdlf::PlaceHolders::_2,  // info
                             subId));

    return result;
}

void Queue::stopPushing()
{
    queueEngine()->resetState(true);  // isShuttingDown
}

}  // close package namespace
}  // close enterprise namespace
