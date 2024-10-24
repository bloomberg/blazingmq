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

// mqbblp_remotequeue.cpp                                             -*-C++-*-
#include <mqbblp_remotequeue.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_storagemanager.h>
#include <mqbcmd_messages.h>
#include <mqbconfm_messages.h>
#include <mqbi_cluster.h>
#include <mqbi_dispatcher.h>
#include <mqbi_domain.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_inmemorystorage.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

#include <bmqsys_time.h>
#include <bmqtsk_alarmlog.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_severity.h>
#include <bdlb_print.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

// -----------------
// class RemoteQueue
// -----------------

int RemoteQueue::configureAsProxy(bsl::ostream& errorDescription,
                                  bool          isReconfigure)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_STORAGE_CFG_FAILURE      = -1,
        rc_INCOMPATIBLE_STORAGE     = -2,
        rc_UNKNOWN_DOMAIN_CONFIG    = -3,
        rc_QUEUE_ENGINE_CFG_FAILURE = -4
    };

    if (isReconfigure) {
        // Proxies don't actually use the domain configuration to configure
        // storage and queue-engine, so we can just return early.
        return rc_SUCCESS;  // RETURN
    }

    mqbconfm::Domain domainCfg;
    domainCfg.deduplicationTimeMs() = 0;  // No history is maintained at proxy
    domainCfg.messageTtl() = bsl::numeric_limits<bsls::Types::Int64>::max();
    // TTL is not applicable at proxy

    // Create the associated storage.
    bslma::ManagedPtr<mqbi::Storage> storageMp;
    storageMp.load(new (*d_allocator_p) mqbs::InMemoryStorage(
                       d_state_p->uri(),
                       d_state_p->key(),
                       mqbs::DataStore::k_INVALID_PARTITION_ID,
                       domainCfg,
                       d_state_p->domain()->capacityMeter(),
                       d_allocator_p),
                   d_allocator_p);

    mqbconfm::Storage config;
    mqbconfm::Limits  limits;
    config.makeInMemory();
    limits.messages() = bsl::numeric_limits<bsls::Types::Int64>::max();
    limits.bytes()    = bsl::numeric_limits<bsls::Types::Int64>::max();

    int rc = storageMp->configure(errorDescription,
                                  config,
                                  limits,
                                  domainCfg.messageTtl(),
                                  domainCfg.maxDeliveryAttempts());
    if (0 != rc) {
        return 10 * rc + rc_STORAGE_CFG_FAILURE;  // RETURN
    }

    storageMp->capacityMeter()->disable();
    // In a remote queue, we don't care about monitoring, so disable it for
    // efficiency performance.

    if (!d_state_p->isStorageCompatible(storageMp)) {
        errorDescription << "Incompatible storage type for ProxyRemoteQueue "
                         << "[uri: " << d_state_p->uri()
                         << ", id: " << d_state_p->id() << "]";
        return rc_INCOMPATIBLE_STORAGE;  // RETURN
    }

    d_state_p->setStorage(storageMp);

    // Create the queueEngine.
    d_queueEngine_mp.load(
        new (*d_allocator_p)
            RelayQueueEngine(d_state_p, mqbconfm::Domain(), d_allocator_p),
        d_allocator_p);

    rc = d_queueEngine_mp->configure(errorDescription);
    if (rc != 0) {
        return 10 * rc + rc_QUEUE_ENGINE_CFG_FAILURE;  // RETURN
    }

    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_CHANGE_ROLE,
        mqbstat::QueueStatsDomain::Role::e_PROXY);

    BALL_LOG_INFO
        << "Created a ProxyRemoteQueue "
        << "[uri: " << d_state_p->uri() << ", id: " << d_state_p->id()
        << ", dispatcherProcessor: "
        << d_state_p->queue()->dispatcherClientData().processorHandle() << "]";

    return rc_SUCCESS;
}

int RemoteQueue::configureAsClusterMember(bsl::ostream& errorDescription,
                                          bool          isReconfigure)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));
    enum {
        rc_SUCCESS                   = 0,
        rc_QUEUE_CONFIGURE_FAILURE   = -1,
        rc_ENGINE_CONFIGURE_FAILURE  = -2,
        rc_STORAGE_CONFIGURE_FAILURE = -3
    };

    int                     rc        = 0;
    mqbi::Queue*            queue     = d_state_p->queue();
    const mqbconfm::Domain& domainCfg = d_state_p->domain()->config();

    if (!isReconfigure) {
        // Only create a storage if this is the initial configure; reconfigure
        // (which happens during conversion to local/remote) should reuse the
        // previously created storage.
        bslma::ManagedPtr<mqbi::Storage>      storageMp;
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqu::MemOutStream                    errorDesc(&localAllocator);
        rc = d_state_p->storageManager()->makeStorage(
            errorDesc,
            &storageMp,
            d_state_p->uri(),
            d_state_p->key(),
            d_state_p->partitionId(),
            domainCfg.messageTtl(),
            domainCfg.maxDeliveryAttempts(),
            domainCfg.storage());
        if (rc != 0) {
            // This most likely means that this queue's partition at this
            // replica is out of sync.

            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << d_state_p->domain()->cluster()->name() << ": Partition ["
                << d_state_p->partitionId()
                << "]: failed to retrieve storage for remote queue ["
                << d_state_p->uri() << "], queueKey [" << d_state_p->key()
                << "], rc: " << rc << ", reason [" << errorDesc.str() << "]."
                << BMQTSK_ALARMLOG_END;

            return 10 * rc + rc_QUEUE_CONFIGURE_FAILURE;  // RETURN
        }

        if (d_state_p->isAtMostOnce()) {
            storageMp->capacityMeter()->disable();
        }

        if (!d_state_p->isStorageCompatible(storageMp)) {
            BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
                << d_state_p->domain()->cluster()->name() << ": Partition ["
                << d_state_p->partitionId()
                << "]: incompatible storage type for remote queue ["
                << d_state_p->uri() << "], queueKey [" << d_state_p->key()
                << "]" << BMQTSK_ALARMLOG_END;
            return 10 * rc + rc_QUEUE_CONFIGURE_FAILURE;  // RETURN
        }

        d_state_p->setStorage(storageMp);

        // Create the queueEngine.
        d_queueEngine_mp.load(
            new (*d_allocator_p)
                RelayQueueEngine(d_state_p, domainCfg, d_allocator_p),
            d_allocator_p);
    }
    else {
        rc = d_state_p->storage()->configure(
            errorDescription,
            domainCfg.storage().config(),
            domainCfg.storage().domainLimits(),
            domainCfg.messageTtl(),
            domainCfg.maxDeliveryAttempts());
        if (rc) {
            return 10 * rc + rc_STORAGE_CONFIGURE_FAILURE;  // RETURN
        }
    }

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqu::MemOutStream                    errorDesc(&localAllocator);
    rc = d_queueEngine_mp->configure(errorDesc);
    if (rc != 0) {
        BMQTSK_ALARMLOG_ALARM("CLUSTER_STATE")
            << d_state_p->domain()->cluster()->name() << ": Partition ["
            << d_state_p->partitionId()
            << "]: failed to configure queue engine for remote queue ["
            << d_state_p->uri() << "], queueKey [" << d_state_p->key()
            << "], rc: " << rc << ", reason [" << errorDesc.str() << "]."
            << BMQTSK_ALARMLOG_END;
        return 10 * rc + rc_ENGINE_CONFIGURE_FAILURE;  // RETURN
    }

    // Inform the storage about the queue in the appropriate thread.  This must
    // be done only after queue and its engine have been configured.

    d_state_p->storageManager()->setQueueRaw(queue,
                                             d_state_p->uri(),
                                             d_state_p->partitionId());
    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_CHANGE_ROLE,
        mqbstat::QueueStatsDomain::Role::e_REPLICA);

    BALL_LOG_INFO << d_state_p->domain()->cluster()->name()
                  << ": Created a ClusterMemberRemoteQueue "
                  << "[uri: '" << d_state_p->uri() << "'"
                  << ", qId: " << d_state_p->id() << ", key: '"
                  << d_state_p->key()
                  << "', partitionId: " << d_state_p->partitionId()
                  << ", processorHandle: "
                  << queue->dispatcherClientData().processorHandle() << "]";

    return rc_SUCCESS;
}

bool RemoteQueue::loadSubQueueInfos(
    bmqp::Protocol::SubQueueInfosArray* subQueueInfos,
    const bdlbb::Blob&                  options,
    const bmqt::MessageGUID&            msgGUID)
{
    // executed by the *DISPATCHER* thread

    // Load 'SubQueueIdsOption' from 'options'.

    int rc = d_optionsView.reset(&options,
                                 bmqu::BlobPosition(),
                                 options.length());
    if (rc) {
        BALL_LOG_ERROR
            << "#CORRUPTED_MESSAGE "
            << "Failed to load options for a PUSH message for queue ["
            << d_state_p->description() << "], GUID [" << msgGUID
            << "], upstream queueId [" << d_state_p->id() << ", rc: " << rc
            << ". Message will be dropped.";
        return false;  // RETURN
    }

    BSLS_ASSERT_SAFE(
        d_optionsView.end() !=
            d_optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) ||
        d_optionsView.end() !=
            d_optionsView.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD));

    rc = d_optionsView.loadSubQueueInfosOption(subQueueInfos);
    BSLS_ASSERT_SAFE(0 == rc);

    return subQueueInfos->size();
}

//  We have two VirtualStorageCatalog in the case of RemoteQueue (one for PUTs,
//  one for PUSHes.
//  And one in the case of LocalQueue.
//
//  LocalQueue   ---------------->  Storage
//                              /       |
//  RemoteQueue  ---------------        |
//          |                           |
//          V                           V
//  VirtualStorageCatalog (2)     VirtualStorageCatalog (1)
//

//              PUT     Replication     Broadcast PUSH      non-Broadcast PUSH
//  --------------------------------------------------------------------------
//  LocalQueue  (1)
//  Replica                 (1)         (1) (2 (some Apps))     (2 (some Apps))
//  Proxy                               (1 (some Apps))         (1 (some Apps))

// Redundant storage at Proxy

void RemoteQueue::pushMessage(
    const bmqt::MessageGUID&             msgGUID,
    const bsl::shared_ptr<bdlbb::Blob>&  appData,
    const bsl::shared_ptr<bdlbb::Blob>&  options,
    const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    bool                                 isOutOfOrder)
{
    // executed by the *QUEUE DISPATCHER* thread

    mqbi::StorageMessageAttributes attributes(0ULL,  // Timestamp; unused
                                              1,     // RefCount
                                              messagePropertiesInfo,
                                              compressionAlgorithmType);
    mqbi::StorageResult::Enum      result  = mqbi::StorageResult::e_SUCCESS;
    mqbi::Storage*                 storage = d_state_p->storage();
    int                            msgSize = 0;

    if (d_state_p->domain()->cluster()->isRemote()) {
        // In a proxy, 'appData' will always be non-null, irrespective of the
        // queue mode.
        BSLS_ASSERT_SAFE(appData);
        msgSize = appData->length();
    }
    else {
        if (d_state_p->isAtMostOnce()) {
            BSLS_ASSERT_SAFE(appData);

            result = storage->put(&attributes, msgGUID, appData, options);

            if (result != mqbi::StorageResult::e_SUCCESS) {
                if (d_throttledFailedPushMessages.requestPermission()) {
                    BALL_LOG_WARN << d_state_p->uri()
                                  << " failed to store broadcast PUSH ["
                                  << msgGUID << "], result = " << result;
                }
            }
        }
        else {
            // In a replica, 'appData' must be empty in non-broadcast mode.
            BSLS_ASSERT_SAFE(!appData);

            // Insert into the ShortList
        }

        // 'msgGUID' must be present in the storage.
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                storage->getMessageSize(&msgSize, msgGUID) !=
                mqbi::StorageResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // This can occur under 2 situations:
            // 1) The message was deleted by the primary because of TTL
            //    expiration, *after* that message guid was enqueued in
            //    primary's cluster-dispatcher thread to be sent to this node
            //    as PUSH message.  In other words, the guid was sitting in the
            //    PUSH-event builder for this node in the primary, and the
            //    builder was flushed after primary replicated message's
            //    DELETION record due to TTL expiration.
            // 2) This message was not replicated by primary to this node.

            // (1) is a valid (but extremely rare) scenario and (2) is a bug.
            // But we have no way to distinguish the two scenarios as of yet.
            BALL_LOG_ERROR
                << "#QUEUE_UNKNOWN_MESSAGE "
                << "Replica remote queue: " << d_state_p->uri()
                << " (id: " << d_state_p->id()
                << ") received a PUSH message from primary for guid "
                << msgGUID << ", which payload does not exist in the storage.";
            return;  // RETURN
        }
    }

    bmqp::Protocol::SubQueueInfosArray subQueueInfos;
    StorageKeys                        storageKeys;

    // Retrieve subQueueInfos from 'options'
    // Need to look up subQueueIds by subscriptionIds.
    if (options) {
        if (!loadSubQueueInfos(&subQueueInfos, *options, msgGUID)) {
            return;  // RETURN
        }
    }
    else {
        // No options.  Must be non-fanout.
        BSLS_ASSERT_SAFE(!d_state_p->hasMultipleSubStreams());

        subQueueInfos.push_back(
            bmqp::SubQueueInfo(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID));
    }

    BSLS_ASSERT_SAFE(d_state_p->hasMultipleSubStreams() ||
                     subQueueInfos.size() == 1);

    d_queueEngine_mp->push(&attributes,
                           msgGUID,
                           appData,
                           subQueueInfos,
                           isOutOfOrder);

    // 'flush' will inform the queue engine so it delivers the message.
}

RemoteQueue::RemoteQueue(QueueState*       state,
                         int               deduplicationTimeMs,
                         int               ackWindowSize,
                         StateSpPool*      statePool,
                         bslma::Allocator* allocator)
: d_state_p(state)
, d_queueEngine_mp(0)
, d_pendingMessages(allocator)
, d_pendingConfirms(allocator)
, d_subStreamMessages_mp()
, d_optionsView(allocator)
, d_pendingPutsTimeoutNs(deduplicationTimeMs *
                         bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND)
, d_pendingMessagesTimerEventHandle()
, d_ackWindowSize(ackWindowSize)
, d_unackedPutCounter(0)
, d_subStreams(allocator)
, d_statePool_p(statePool)
, d_producerState()
, d_allocator_p(allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->id() != bmqp::QueueId::k_UNASSIGNED_QUEUE_ID);
    BSLS_ASSERT_SAFE(d_state_p->id() != bmqp::QueueId::k_PRIMARY_QUEUE_ID);
    // A RemoteQueue must have an upstream id

    d_throttledFailedPutMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    d_throttledFailedPushMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    d_throttledFailedAckMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    d_throttledFailedConfirmMessages.initialize(
        1,
        5 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // 1 log per 5s interval

    // Description, to identify a 'remote' queue, prefix its URI with an '@'.
    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    bmqu::MemOutStream                    os(&localAllocator);
    os << '@' << d_state_p->uri().asString();
    d_state_p->setDescription(os.str());

    BALL_LOG_INFO << "Remote queue: " << d_state_p->uri()
                  << " [id: " << d_state_p->id() << "]";
}

RemoteQueue::~RemoteQueue()
{
    BSLS_ASSERT_SAFE(d_pendingMessages.empty());
    BSLS_ASSERT_SAFE(!d_pendingMessagesTimerEventHandle);
}

int RemoteQueue::configure(bsl::ostream& errorDescription, bool isReconfigure)
{
    // executed by the queue *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // Update stats
    if (isReconfigure) {
        const mqbconfm::Domain& domainCfg = d_state_p->domain()->config();
        if (domainCfg.mode().isFanoutValue()) {
            d_state_p->stats().updateDomainAppIds(
                domainCfg.mode().fanout().appIDs());
        }
    }

    if (d_state_p->domain()->cluster()->isRemote()) {
        return configureAsProxy(errorDescription, isReconfigure);  // RETURN
    }
    else {
        return configureAsClusterMember(errorDescription, isReconfigure);
        // RETURN
    }
}

void RemoteQueue::resetState()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    erasePendingMessages(d_pendingMessages.end());

    if (d_pendingMessagesTimerEventHandle) {
        scheduler()->cancelEventAndWait(&d_pendingMessagesTimerEventHandle);
        // 'expirePendingMessagesDispatched' does not restart timer if
        // 'd_pendingMessages' is empty
    }
    d_queueEngine_mp->resetState();
    d_queueEngine_mp->afterQueuePurged(bmqp::ProtocolUtil::k_NULL_APP_ID,
                                       mqbu::StorageKey::k_NULL_KEY);
    d_pendingConfirms.clear();
}

void RemoteQueue::close()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // This function is invoked when DomainManager stops.  Self node is past
    // e_STOPPING state meaning it has received all StopResponses meaning
    // (unless StopRequest has timed out) all downstreams have closed all
    // queues.

    size_t numMessages = erasePendingMessages(d_pendingMessages.end());

    BALL_LOG_INFO << d_state_p->uri() << ": erased all " << numMessages
                  << " pending message(s)"
                  << " while closing the queue.";

    BSLS_ASSERT_SAFE(d_pendingMessages.size() == 0);

    if (d_pendingMessagesTimerEventHandle) {
        scheduler()->cancelEventAndWait(&d_pendingMessagesTimerEventHandle);
        // 'expirePendingMessagesDispatched' does not restart timer if
        // 'd_pendingMessages' is empty
    }
}

void RemoteQueue::getHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    mqbi::Cluster* cluster = d_state_p->domain()->cluster();

    if (cluster->isRemote()) {
        if (d_state_p->hasMultipleSubStreams() &&
            !handleParameters.subIdInfo().isNull()) {
            // In fanout mode at proxy need to add
            // virtual storage to the physical storage
            bsl::ostringstream errorDesc;
            mqbu::StorageKey   appKey(upstreamSubQueueId);

            d_state_p->storage()->addVirtualStorage(
                errorDesc,
                handleParameters.subIdInfo().value().appId(),
                appKey);

            // Do not check the result code; the VS may already exists if there
            // is another consumer for the same 'upstreamSubQueueId'.
        }
        else if (bmqt::QueueFlagsUtil::isReader(handleParameters.flags())) {
            // Reader in non-fanout mode
            bsl::ostringstream errorDesc;
            d_state_p->storage()->addVirtualStorage(
                errorDesc,
                bmqp::ProtocolUtil::k_DEFAULT_APP_ID,
                mqbi::QueueEngine::k_DEFAULT_APP_KEY);
        }
    }

    d_queueEngine_mp->getHandle(clientContext,
                                handleParameters,
                                upstreamSubQueueId,
                                callback);
}

void RemoteQueue::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_queueEngine_mp->configureHandle(handle, streamParameters, configuredCb);
}

void RemoteQueue::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    mqbi::QueueHandle::HandleReleasedCallback onReleasedCallback =
        bdlf::BindUtil::bind(&RemoteQueue::onHandleReleased,
                             this,
                             handleParameters,
                             releasedCb,
                             bdlf::PlaceHolders::_1,   // handle
                             bdlf::PlaceHolders::_2);  // isDeleted

    d_queueEngine_mp->releaseHandle(handle,
                                    handleParameters,
                                    isFinal,
                                    onReleasedCallback);
}

void RemoteQueue::onHandleReleased(
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb,
    const bsl::shared_ptr<mqbi::QueueHandle>&        handle,
    const mqbi::QueueHandleReleaseResult&            result)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_state_p->updateStats();

    mqbi::Cluster* cluster = d_state_p->domain()->cluster();
    if (result.hasNoHandleStreamConsumers()) {
        // Lost last reader for the specified subStream for the handle
        size_t numProcessed = 0;
        for (Confirms::iterator it = d_pendingConfirms.begin();
             it != d_pendingConfirms.end();) {
            if (it->d_handle == handle.get()) {
                if (d_throttledFailedConfirmMessages.requestPermission()) {
                    BALL_LOG_WARN << "Dropping CONFIRM because downstream ["
                                  << handle << "] is gone. [queue: '"
                                  << d_state_p->description() << "', GUID: '"
                                  << it->d_guid << "']";
                }
                it = d_pendingConfirms.erase(it);
                ++numProcessed;
            }
            else {
                ++it;
            }
        }

        BALL_LOG_INFO << d_state_p->uri() << ": iterated pending CONFIRM(s); "
                      << numProcessed << " erased";
    }
    if (result.hasNoQueueStreamConsumers()) {
        if (cluster->isRemote()) {
            if (d_state_p->hasMultipleSubStreams()) {
                if (handle && !handleParameters.subIdInfo().isNull()) {
                    // Received Successful 'releaseHandle' in fanout mode at
                    // proxy: need to remove from physical storage the
                    // previously added virtual storage
                    const bsl::string& appId =
                        handleParameters.subIdInfo().value().appId();
                    mqbu::StorageKey appKey;
                    const bool       hasVirtualStorage =
                        d_state_p->storage()->hasVirtualStorage(appId,
                                                                &appKey);
                    BSLS_ASSERT_SAFE(hasVirtualStorage);
                    d_state_p->storage()->removeVirtualStorage(appKey);

                    (void)
                        hasVirtualStorage;  // Compiler happiness in opt build
                }
            }
            else if (!bmqt::QueueFlagsUtil::isReader(
                         d_state_p->handleParameters().flags())) {
                // Lost last reader in non-fanout mode
                d_state_p->storage()->removeVirtualStorage(
                    mqbi::QueueEngine::k_DEFAULT_APP_KEY);
                d_state_p->storage()->removeAll(mqbu::StorageKey::k_NULL_KEY);
            }
        }
    }
    if (result.hasNoHandleStreamProducers()) {
        // Leaving it to the downstream to NACK pending messages.
        cleanPendingMessages(handle.get());
    }

    if (releasedCb) {
        releasedCb(handle, result);
    }
}

void RemoteQueue::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    BALL_LOG_TRACE << d_state_p->description()
                   << ": processing dispatcher event '" << event << "'";

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_CONTROL_MSG: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT " << d_state_p->description()
                       << "CONTROL_MSG event not yet implemented";
    } break;
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            &event.getAs<mqbi::DispatcherCallbackEvent>();
        BSLS_ASSERT_SAFE(realEvent->callback());
        realEvent->callback()(
            d_state_p->queue()->dispatcherClientData().processorHandle());
    } break;
    case mqbi::DispatcherEventType::e_PUSH: {
        const mqbi::DispatcherPushEvent* realEvent =
            &event.getAs<mqbi::DispatcherPushEvent>();
        pushMessage(realEvent->guid(),
                    realEvent->blob(),
                    realEvent->options(),
                    realEvent->messagePropertiesInfo(),
                    realEvent->compressionAlgorithmType(),
                    realEvent->isOutOfOrderPush());
    } break;
    case mqbi::DispatcherEventType::e_PUT: {
        const mqbi::DispatcherPutEvent* realEvent =
            &event.getAs<mqbi::DispatcherPutEvent>();
        postMessage(realEvent->putHeader(),
                    realEvent->blob(),
                    realEvent->options(),
                    realEvent->queueHandle());
    } break;
    case mqbi::DispatcherEventType::e_ACK: {
        onAckMessageDispatched(event.getAs<mqbi::DispatcherAckEvent>());
    } break;
    case mqbi::DispatcherEventType::e_CONFIRM: {
        BSLS_ASSERT_OPT(false && "'CONFIRM' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_REJECT: {
        BSLS_ASSERT_OPT(false && "'REJECT' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_DISPATCHER: {
        BSLS_ASSERT_OPT(false &&
                        "'DISPATCHER' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_CLUSTER_STATE: {
        BSLS_ASSERT_OPT(false &&
                        "'CLUSTER_STATE' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    case mqbi::DispatcherEventType::e_STORAGE: {
        BSLS_ASSERT_OPT(false && "'STORAGE' type dispatcher event unexpected");
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
    case mqbi::DispatcherEventType::e_UNDEFINED: {
        BSLS_ASSERT_OPT(false && "'NONE' type dispatcher event unexpected");
        return;  // RETURN
    }  // break;
    default: {
        BALL_LOG_ERROR << "#QUEUE_UNEXPECTED_EVENT "
                       << d_state_p->description()
                       << ": received unexpected dispatcher event [type: "
                       << event.type() << "]";
    }
    }
}

void RemoteQueue::flush()
{
    if (d_queueEngine_mp) {
        const bmqt::MessageGUID dummy;
        d_queueEngine_mp->afterNewMessage(dummy, 0);
    }
}

void RemoteQueue::postMessage(const bmqp::PutHeader&              putHeaderIn,
                              const bsl::shared_ptr<bdlbb::Blob>& appData,
                              const bsl::shared_ptr<bdlbb::Blob>& options,
                              mqbi::QueueHandle*                  source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));
    BSLS_ASSERT_SAFE(
        source->subStreamInfos().find(bmqp::ProtocolUtil::k_DEFAULT_APP_ID) !=
        source->subStreamInfos().end());

    // REVISIT: This assumes no MessageProperties access before this point.
    //          As a result, SchemaLearner can be per queue and therefore
    //          single-threaded.
    // We two options for communicating `translation`:
    //  1. `DispatcherEvent:setHasMessageProperties(translation)` or
    //  2. rewrite `putHeader`
    bmqp::MessagePropertiesInfo translation =
        d_state_p->queue()->schemaLearner().multiplex(
            source->schemaLearnerContext(),
            bmqp::MessagePropertiesInfo(putHeaderIn));

    bmqp::PutHeader putHeader(putHeaderIn);
    translation.applyTo(&putHeader);

    // Relay the PUT message via clusterProxy/cluster
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!bmqt::QueueFlagsUtil::isWriter(
            source->handleParameters().flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Either queue was not opened in the WRITE mode (which should have
        // been caught in the SDK) or client is posting a message after closing
        // or reconfiguring the queue (which may not be caught in the SDK).

        if (d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_WARN
                << "#CLIENT_IMPROPER_BEHAVIOR "
                << "Failed PUT message for queue [" << d_state_p->uri()
                << "] from client [" << source->client()->description()
                << "]. Queue not opened in WRITE mode by the client.";
        }

        // Note that a NACK is not sent in this case.  This is a case of client
        // violating the contract, by attempting to post a message after
        // closing/reconfiguring the queue.  Since this is out of contract, its
        // ok not to send the NACK.  If it is still desired to send a NACK, it
        // will need some enqueuing b/w client and queue dispatcher threads to
        // ensure that despite NACKs being sent, closeQueue response is still
        // the last event to be sent to the client for the given queue.

        return;  // RETURN
    }

    SubStreamContext& ctx = d_producerState;

    if (ctx.d_state == SubStreamContext::e_NONE) {
        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << d_state_p->uri()
                      << ": has received PUT [GUID: '"
                      << putHeader.messageGUID() << "'] for unknown upstream";
        return;  // RETURN
    }
    if (ctx.d_state == SubStreamContext::e_CLOSED) {
        if (!d_state_p->isAtMostOnce()) {
            bmqp::AckMessage ackMessage;

            ackMessage.setStatus(bmqp::ProtocolUtil::ackResultToCode(
                bmqt::AckResult::e_REFUSED));
            ackMessage.setMessageGUID(putHeader.messageGUID());

            d_state_p->stats().onEvent(
                mqbstat::QueueStatsDomain::EventType::e_NACK,
                1);

            // CorrelationId & QueueId are left unset as those fields
            // will be filled downstream.
            source->onAckMessage(ackMessage);
        }
        return;  // RETURN
    }
    // Always add to the 'd_pendingMessages':
    //  - if this is broadcast PUT, just GUID unless there is no upstream
    //  - else, also keep 'appData' and 'options'

    bsl::shared_ptr<bmqu::AtomicState> state = d_statePool_p->getObject();
    bsls::Types::Int64                 now   = 0;

    if (d_state_p->isAtMostOnce()) {
        BSLS_ASSERT_SAFE(!bmqp::PutHeaderFlagUtil::isSet(
            putHeader.flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED));

        if (d_unackedPutCounter == d_ackWindowSize) {
            // request an ACK
            int flags = putHeader.flags();
            bmqp::PutHeaderFlagUtil::setFlag(
                &flags,
                bmqp::PutHeaderFlags::e_ACK_REQUESTED);
            const_cast<bmqp::PutHeader&>(putHeader).setFlags(flags);
            d_unackedPutCounter = 0;
        }
        else {
            ++d_unackedPutCounter;
        }

        // No retransmission for broadcast PUTs unless NOT_READY NACK is
        // received which will carry the data and options.
        // No need for the expiration timer since cleaning is done by receiving
        // an ACK for 'd_ackWindowSize' PUTs.
    }
    else {
        now = bmqsys::Time::highResolutionTimer();

        if (!d_pendingMessagesTimerEventHandle) {
            bsls::TimeInterval time;
            time.setTotalNanoseconds(now + d_pendingPutsTimeoutNs);
            scheduler()->scheduleEvent(
                &d_pendingMessagesTimerEventHandle,
                time,
                bdlf::BindUtil::bind(&RemoteQueue::expirePendingMessages,
                                     this));
        }
    }

    if (ctx.d_state == SubStreamContext::e_OPENED &&
        d_state_p->isAtMostOnce()) {
        d_pendingMessages.insert(
            bsl::make_pair(putHeader.messageGUID(),
                           PutMessage(source, putHeader, 0, 0, 0, state)));
    }
    else {
        d_pendingMessages.insert(bsl::make_pair(
            putHeader.messageGUID(),
            PutMessage(source, putHeader, appData, options, now, state)));
    }

    if (ctx.d_state == SubStreamContext::e_OPENED) {
        sendPutMessage(putHeader, appData, options, state, ctx.d_genCount);
    }

    // Update the domain's PUT stats (note that ideally this should be done at
    // the time the message is actually sent upstream, i.e. in
    // cluster/clusterProxy) for the most exact accuracy, but doing it here is
    // good enough.
    d_state_p->stats().onEvent(mqbstat::QueueStatsDomain::EventType::e_PUT,
                               appData->length());
}

void RemoteQueue::confirmMessage(const bmqt::MessageGUID& msgGUID,
                                 unsigned int             upstreamSubQueueId,
                                 mqbi::QueueHandle*       source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    if (1 != d_queueEngine_mp->onConfirmMessage(source,
                                                msgGUID,
                                                upstreamSubQueueId)) {
        // Ok to delete the message.
        d_state_p->storage()->remove(msgGUID);
    }

    SubStreamContext& ctx = subStreamContext(upstreamSubQueueId);

    switch (ctx.d_state) {
    case SubStreamContext::e_NONE: {
        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << d_state_p->uri()
                      << ": has received confirm [upstreamSubQueueId: "
                      << upstreamSubQueueId << ", GUID: '" << msgGUID
                      << "'] for unknown upstream";
    } break;
    case SubStreamContext::e_CLOSED: {
        BALL_LOG_INFO << d_state_p->uri()
                      << ": dropping confirm [upstreamSubQueueId: "
                      << upstreamSubQueueId << ", GUID: '" << msgGUID
                      << "'] for the upstream is not available";

        // Upstream is not available.  Drop this CONFIRM.  New primary will
        // re-deliver PUSH
    } break;
    case SubStreamContext::e_STOPPED: {
        d_pendingConfirms.emplace_back(msgGUID, upstreamSubQueueId, source);
    } break;
    case SubStreamContext::e_OPENED: {
        sendConfirmMessage(msgGUID, upstreamSubQueueId, source);
    } break;
    }
}

int RemoteQueue::rejectMessage(const bmqt::MessageGUID& msgGUID,
                               unsigned int             upstreamSubQueueId,
                               mqbi::QueueHandle*       source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    BALL_LOG_TRACE << "OnReject [queue: '" << d_state_p->description()
                   << "', client: '" << *(source->client()) << "', GUID: '"
                   << msgGUID << "']";

    int result = d_queueEngine_mp->onRejectMessage(source,
                                                   msgGUID,
                                                   upstreamSubQueueId);

    SubStreamContext& ctx = subStreamContext(upstreamSubQueueId);

    switch (ctx.d_state) {
    case SubStreamContext::e_NONE: {
        BALL_LOG_WARN << "#CLIENT_IMPROPER_BEHAVIOR " << d_state_p->uri()
                      << ": has received reject [upstreamSubQueueId: "
                      << upstreamSubQueueId << ", GUID: '" << msgGUID
                      << "'] for unknown upstream";
    } break;
    case SubStreamContext::e_CLOSED:
    case SubStreamContext::e_STOPPED: {
        // REVISIT: could keep Rejects and transmit when upstream is available.
        if (ctx.d_state == SubStreamContext::e_CLOSED) {
            BALL_LOG_INFO << d_state_p->uri() << ": dropping reject";
        }
    } break;
    case SubStreamContext::e_OPENED: {
        // Relay the REJECT message via clusterProxy/cluster.
        mqbi::Queue*        queue   = d_state_p->queue();
        mqbi::Cluster*      cluster = d_state_p->domain()->cluster();
        bmqp::RejectMessage rejectMessage;
        rejectMessage.setQueueId(d_state_p->id())
            .setSubQueueId(upstreamSubQueueId)
            .setMessageGUID(msgGUID);

        mqbi::Dispatcher*      dispatcher = queue->dispatcher();
        mqbi::DispatcherEvent* dispEvent  = dispatcher->getEvent(cluster);
        (*dispEvent)
            .setSource(queue)
            .makeRejectEvent()
            .setRejectMessage(rejectMessage)
            .setPartitionId(d_state_p->partitionId())
            .setIsRelay(true);  // Relay message
                                // partitionId is needed only by replica
        dispatcher->dispatchEvent(dispEvent, cluster);
    } break;
    }
    return result;
}

void RemoteQueue::sendConfirmMessage(const bmqt::MessageGUID& msgGUID,
                                     unsigned int       upstreamSubQueueId,
                                     mqbi::QueueHandle* source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    BALL_LOG_TRACE << "OnConfirm [queue: '" << d_state_p->description()
                   << "', client: '" << *(source->client()) << "', GUID: '"
                   << msgGUID << "']";

    // Relay the CONFIRM message via clusterProxy/cluster.
    mqbi::Queue*         queue   = d_state_p->queue();
    mqbi::Cluster*       cluster = d_state_p->domain()->cluster();
    bmqp::ConfirmMessage confirmMessage;
    confirmMessage.setQueueId(d_state_p->id())
        .setSubQueueId(upstreamSubQueueId)
        .setMessageGUID(msgGUID);

    mqbi::Dispatcher*      dispatcher = queue->dispatcher();
    mqbi::DispatcherEvent* dispEvent  = dispatcher->getEvent(cluster);
    (*dispEvent)
        .setSource(queue)
        .makeConfirmEvent()
        .setConfirmMessage(confirmMessage)
        .setPartitionId(d_state_p->partitionId())
        .setIsRelay(true);  // Relay message
                            // partitionId is needed only by replica
    dispatcher->dispatchEvent(dispEvent, cluster);
}

void RemoteQueue::onAckMessageDispatched(const mqbi::DispatcherAckEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // This routine is invoked on the RemoteQueue by clusterProxy/cluster
    // whenever it receives an ACK from upstream.

    // Look for the handle in the guid->queueHandle map
    mqbi::QueueHandle*      source     = 0;
    const bmqp::AckMessage& ackMessage = event.ackMessage();
    bmqt::AckResult::Enum   ackResult  = bmqp::ProtocolUtil::ackResultFromCode(
        ackMessage.status());
    Puts::iterator it = d_pendingMessages.find(ackMessage.messageGUID());

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(it != d_pendingMessages.end())) {
        source = it->second.d_handle;
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(source == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Didn't find the GUID.  This means that we got an ack from upstream
        // for a guid which we are not aware of.  Log it.
        //
        // NOTE: this could happen if the downstream client sent a message,
        //       then closes the queue/disconnect, before the cluster sent back
        //       the ACK.
        // TBD: Until closeQueue sequence is revisited to prevent this
        //      situation, log at INFO level.

        ball::Severity::Level severity = (ackResult ==
                                                  bmqt::AckResult::e_SUCCESS
                                              ? ball::Severity::DEBUG
                                              : ball::Severity::INFO);

        if (d_throttledFailedAckMessages.requestPermission()) {
            BALL_LOG_STREAM(severity)
                << "Received ACK message [" << ackResult
                << ", queue: " << d_state_p->description()
                << "] for unknown guid: " << ackMessage.messageGUID();
        }
        return;  // RETURN
    }

    if (ackResult == bmqt::AckResult::e_NOT_READY) {
        // Drop this "soft" NACK since retransmission is still possible even
        // for broadcast queue since the PUT did not go upstream.
        // If this is broadcast queue, enable this PUT for re-transmission by
        // storing data and options.  (Initially PUTs in broadcast mode do not
        // keep data and options.)
        if (d_state_p->isAtMostOnce() && event.blob()) {
            BSLS_ASSERT_SAFE(!it->second.d_appData && !it->second.d_options);
            it->second.d_appData = event.blob();
            it->second.d_options = event.options();
        }
        return;  // RETURN
    }

    if (d_state_p->isAtMostOnce()) {
        // Consider this a "non-soft" ACK for all previous broadcasted PUTs.
        size_t numErased = 1 + erasePendingMessages(it);

        erasePendingMessage(it);

        BALL_LOG_INFO << d_state_p->uri() << ": erased window of " << numErased
                      << " cached broadcasted PUTs upon "
                      << bmqt::AckResult::toAscii(ackResult);

        return;  // RETURN
    }

    erasePendingMessage(it);
    source->onAckMessage(ackMessage);
}

size_t RemoteQueue::iteratePendingMessages(const PutsVisitor& visitor)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    size_t numMessages  = d_pendingMessages.size();
    size_t numProcessed = 0;

    for (Puts::iterator it = d_pendingMessages.begin();
         it != d_pendingMessages.end();
         ++it) {
        if (it->second.d_appData) {
            visitor(it->second.d_header,
                    it->second.d_appData,
                    it->second.d_options,
                    it->second.d_handle);
            ++numProcessed;
        }
    }

    BALL_LOG_INFO << d_state_p->uri() << ": iterated " << numMessages
                  << " pending message(s); " << numProcessed << " processed.";

    return numMessages;
}

size_t RemoteQueue::iteratePendingConfirms(const ConfirmsVisitor& visitor)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    size_t numProcessed = 0;
    if (!d_pendingConfirms.empty()) {
        const bmqt::MessageGUID firstGUID = d_pendingConfirms.begin()->d_guid;

        for (Confirms::iterator it = d_pendingConfirms.begin();
             it != d_pendingConfirms.end();
             ++it) {
            visitor(it->d_guid, it->d_upstreamSubQueueId, it->d_handle);

            ++numProcessed;
        }

        BALL_LOG_INFO << d_state_p->uri() << ": iterated pending CONFIRM(s); "
                      << numProcessed << " processed starting from "
                      << firstGUID;
    }

    return numProcessed;
}

RemoteQueue::Puts::iterator
RemoteQueue::erasePendingMessage(Puts::iterator& it)
{
    if (it->second.d_state_sp) {
        it->second.d_state_sp->cancel();
    }
    return d_pendingMessages.erase(it);
}

size_t RemoteQueue::erasePendingMessages(const Puts::iterator& it)
{
    size_t numErased = 0;
    for (Puts::iterator previous = d_pendingMessages.begin(); previous != it;
         previous                = erasePendingMessage(previous)) {
        ++numErased;
    }
    return numErased;
}

void RemoteQueue::expirePendingMessages()
{
    // executed by the *SCHEDULER* thread
    d_state_p->queue()->dispatcher()->execute(
        bdlf::BindUtil::bind(&RemoteQueue::expirePendingMessagesDispatched,
                             this),
        d_state_p->queue());
}

void RemoteQueue::expirePendingMessagesDispatched()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    bsls::Types::Int64 now         = bmqsys::Time::highResolutionTimer();
    bsls::Types::Int64 nextTime    = 0;
    bsls::Types::Int64 numExpired  = 0;
    bsls::Types::Int64 numMessages = d_pendingMessages.size();
    bmqp::AckMessage   ackMessage;

    ackMessage.setStatus(
        bmqp::ProtocolUtil::ackResultToCode(bmqt::AckResult::e_UNKNOWN));
    for (Puts::iterator it = d_pendingMessages.begin();
         it != d_pendingMessages.end();) {
        // Must be non-broadcast
        BSLS_ASSERT_SAFE(it->second.d_timeReceived);

        if (now - it->second.d_timeReceived >= d_pendingPutsTimeoutNs) {
            // Expired.  Regardless of the upstream state, NACK it and erase
            it = nack(it, ackMessage);

            ++numExpired;
        }
        else {
            // when to wake up next time.
            nextTime = it->second.d_timeReceived;

            // no need to continue for PUTs are in chronological order
            break;  // BREAK
        }
    }

    if (numExpired) {
        if (d_throttledFailedPutMessages.requestPermission()) {
            BALL_LOG_INFO << "[THROTTLED] " << d_state_p->uri() << ": expired "
                          << bmqu::PrintUtil::prettyNumber(numExpired)
                          << " pending PUT messages ("
                          << bmqu::PrintUtil::prettyNumber(numMessages -
                                                           numExpired)
                          << " remaining messages).";
        }
    }

    // reschedule
    if (nextTime) {
        nextTime += d_pendingPutsTimeoutNs;

        bsls::TimeInterval time;
        time.setTotalNanoseconds(nextTime);
        scheduler()->scheduleEvent(
            &d_pendingMessagesTimerEventHandle,
            time,
            bdlf::BindUtil::bind(&RemoteQueue::expirePendingMessages, this));

        BALL_LOG_DEBUG << d_state_p->uri() << ": will check again to expire"
                       << " pending PUSH messages in "
                       << bmqu::PrintUtil::prettyTimeInterval(
                              nextTime - bmqsys::Time::highResolutionTimer());
    }
    else {
        d_pendingMessagesTimerEventHandle.release();
        BALL_LOG_INFO << d_state_p->uri() << ": "
                      << "no more timer scheduled to check expiration of "
                      << "pending PUSH messages";
    }
}

void RemoteQueue::retransmitPendingMessagesDispatched(
    bsls::Types::Uint64 genCount)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    if (!d_pendingMessages.empty()) {
        size_t                  numTransmitted = 0;
        size_t                  numMessages    = d_pendingMessages.size();
        const bmqt::MessageGUID firstGUID =
            d_pendingMessages.begin()->second.d_header.messageGUID();

        for (Puts::iterator it = d_pendingMessages.begin();
             it != d_pendingMessages.end();) {
            if (it->second.d_appData) {
                // cancel previous attempt to send the message
                if (it->second.d_state_sp) {
                    it->second.d_state_sp->cancel();
                    it->second.d_state_sp = d_statePool_p->getObject();
                }
                sendPutMessage(it->second.d_header,
                               it->second.d_appData,
                               it->second.d_options,
                               it->second.d_state_sp,
                               genCount);
                ++numTransmitted;
                if (0 == it->second.d_timeReceived) {
                    // This is broadcast (not time-controlled); no more
                    // retransmission unless NOT_READY NACK is received in
                    // which case NACK will supply the data.
                    it->second.d_appData.clear();
                    it->second.d_options.clear();
                }
                ++it;
            }
            else {
                // This is previously transmitted broadcast for which there was
                // no NOT_READY NACK.  Since we are in reopen queue processing,
                // do not expect NOT_READY NACK anymore.
                it = erasePendingMessage(it);
            }
        }

        BALL_LOG_INFO << d_state_p->uri() << ": processed " << numMessages
                      << " pending message(s); " << numTransmitted
                      << " (re)transmitted starting from " << firstGUID;
    }
}

void RemoteQueue::retransmitPendingConfirmsDispatched(
    unsigned int upstreamSubQueueId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    if (d_pendingConfirms.empty()) {
        return;  // RETURN
    }

    bmqt::MessageGUID firstGUID;
    size_t            numProcessed = 0;
    for (Confirms::iterator it = d_pendingConfirms.begin();
         it != d_pendingConfirms.end();) {
        if (upstreamSubQueueId == it->d_upstreamSubQueueId) {
            sendConfirmMessage(it->d_guid,
                               it->d_upstreamSubQueueId,
                               it->d_handle);

            if (++numProcessed == 1) {
                firstGUID = it->d_guid;
            }
            it = d_pendingConfirms.erase(it);
        }
        else {
            ++it;
        }
    }
    if (numProcessed) {
        BALL_LOG_INFO << d_state_p->uri() << ": transmitted " << numProcessed
                      << " pending CONFIRM(s) for subQueueId "
                      << upstreamSubQueueId << " starting from " << firstGUID;
    }
}

void RemoteQueue::cleanPendingMessages(mqbi::QueueHandle* handle)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));
    BSLS_ASSERT_SAFE(handle);

    size_t numMessages = 0;
    for (Puts::iterator it = d_pendingMessages.begin();
         it != d_pendingMessages.end();) {
        if (it->second.d_handle == handle) {
            it = erasePendingMessage(it);
            ++numMessages;
        }
        else {
            ++it;
        }
    }

    if (d_pendingMessages.size() == 0 && d_pendingMessagesTimerEventHandle) {
        scheduler()->cancelEventAndWait(&d_pendingMessagesTimerEventHandle);
        // 'expirePendingMessagesDispatched' does not restart timer if
        // 'd_pendingMessages' is empty
    }

    BALL_LOG_INFO << d_state_p->uri() << ": erased " << numMessages
                  << " pending message(s) while releasing handle.";
}

RemoteQueue::Puts::iterator& RemoteQueue::nack(Puts::iterator&   it,
                                               bmqp::AckMessage& ackMessage)
{
    ackMessage.setMessageGUID(it->first);

    d_state_p->stats().onEvent(mqbstat::QueueStatsDomain::EventType::e_NACK,
                               1);

    // CorrelationId & QueueId are left unset as those fields
    // will be filled downstream.
    it->second.d_handle->onAckMessage(ackMessage);

    it = erasePendingMessage(it);
    return it;
}

void RemoteQueue::sendPutMessage(
    const bmqp::PutHeader&                    putHeader,
    const bsl::shared_ptr<bdlbb::Blob>&       appData,
    const bsl::shared_ptr<bdlbb::Blob>&       options,
    const bsl::shared_ptr<bmqu::AtomicState>& state,
    bsls::Types::Uint64                       genCount)
{
    mqbi::Cluster* cluster = d_state_p->domain()->cluster();

    // Replica or Proxy.  Update queueId to the one known upstream.
    bmqp::PutHeader& ph = const_cast<bmqp::PutHeader&>(putHeader);
    ph.setQueueId(d_state_p->id());

    mqbi::Dispatcher*      dispatcher = d_state_p->queue()->dispatcher();
    mqbi::DispatcherEvent* dispEvent  = dispatcher->getEvent(cluster);
    (*dispEvent)
        .setSource(d_state_p->queue())
        .makePutEvent()
        .setIsRelay(true)  // Relay message
        .setPutHeader(ph)
        .setPartitionId(d_state_p->partitionId())  // Only replica uses
        .setBlob(appData)
        .setOptions(options)
        .setGenCount(genCount)
        .setState(state);
    dispatcher->dispatchEvent(dispEvent, cluster);
}

void RemoteQueue::onOpenFailure(unsigned int upstreamSubQueueId)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    SubStreamContext& ctx = subStreamContext(upstreamSubQueueId);
    ctx.d_state           = SubStreamContext::e_CLOSED;
    ctx.d_genCount        = 0;

    d_producerState.d_state    = SubStreamContext::e_CLOSED;
    d_producerState.d_genCount = 0;

    if (upstreamSubQueueId == bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        size_t numMessages = d_pendingMessages.size();
        if (numMessages) {
            if (d_state_p->isAtMostOnce()) {
                erasePendingMessages(d_pendingMessages.end());

                BALL_LOG_INFO << d_state_p->uri() << ": erased all "
                              << numMessages
                              << " pending message(s) due to Reopen failure";
            }
            else {
                bmqp::AckMessage ackMessage;

                ackMessage.setStatus(bmqp::ProtocolUtil::ackResultToCode(
                    bmqt::AckResult::e_UNKNOWN));
                for (Puts::iterator it = d_pendingMessages.begin();
                     it != d_pendingMessages.end();) {
                    it = nack(it, ackMessage);
                }

                BALL_LOG_INFO << d_state_p->uri() << ": NACK'ed all "
                              << numMessages
                              << " pending message(s) due to Reopen failure";
            }
        }

        BSLS_ASSERT_SAFE(d_pendingMessages.size() == 0);

        if (d_pendingMessagesTimerEventHandle) {
            scheduler()->cancelEventAndWait(
                &d_pendingMessagesTimerEventHandle);
            // 'expirePendingMessagesDispatched' does not restart timer if
            // 'd_pendingMessages' is empty
        }
    }
    size_t numConfirms = d_pendingConfirms.size();

    if (numConfirms == 0) {
        return;  // RETURN
    }

    bmqt::MessageGUID firstGUID;
    size_t            numProcessed = 0;
    for (Confirms::iterator it = d_pendingConfirms.begin();
         it != d_pendingConfirms.end();) {
        if (upstreamSubQueueId == it->d_upstreamSubQueueId) {
            if (++numProcessed == 1) {
                firstGUID = it->d_guid;
            }
            it = d_pendingConfirms.erase(it);
        }
        else {
            ++it;
        }
    }
    if (numProcessed) {
        BALL_LOG_INFO << d_state_p->uri() << ": erased " << numProcessed
                      << " out of " << numConfirms
                      << " pending CONFIRM(s) due to Reopen failure"
                      << " starting from " << firstGUID;
    }
}

void RemoteQueue::onLostUpstream()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // Connection with upstream was lost.
    for (SubQueueIds::iterator i = d_subStreams.begin();
         i != d_subStreams.end();
         ++i) {
        i->d_state    = SubStreamContext::e_STOPPED;
        i->d_genCount = 0;
    }

    d_producerState.d_state    = SubStreamContext::e_STOPPED;
    d_producerState.d_genCount = 0;

    BALL_LOG_INFO << d_state_p->uri() << ": has lost the upstream";
}

void RemoteQueue::onOpenUpstream(bsls::Types::Uint64 genCount,
                                 unsigned int        upstreamSubQueueId,
                                 bool                isWriterOnly)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    SubStreamContext& ctx = isWriterOnly
                                ? d_producerState
                                : subStreamContext(upstreamSubQueueId);

    if (genCount == 0) {
        // This is a result of StopRequest processing.
        // Now we need to wait for new genCount (!= d_upstreamGenCount).
        // Until then, we buffer.
        if (ctx.d_state == SubStreamContext::e_OPENED) {
            if (upstreamSubQueueId == bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
                if (isWriterOnly) {
                    BALL_LOG_INFO << d_state_p->uri()
                                  << ": buffering PUTs with generation count "
                                  << ctx.d_genCount
                                  << " because upstream is stopping.";
                }
                else {
                    BALL_LOG_INFO << d_state_p->uri()
                                  << ": buffering PUTs and CONFIRMs with"
                                  << " generation count " << ctx.d_genCount
                                  << " because upstream is stopping.";
                    d_producerState.d_state    = SubStreamContext::e_STOPPED;
                    d_producerState.d_genCount = 0;
                }
            }
            else {
                BALL_LOG_INFO << d_state_p->uri()
                              << ": buffering CONFIRMS for subQueueId: "
                              << upstreamSubQueueId
                              << " with generation count " << ctx.d_genCount
                              << " because upstream is stopping.";
            }

            ctx.d_state    = SubStreamContext::e_STOPPED;
            ctx.d_genCount = 0;
        }
    }
    else if (ctx.d_genCount == genCount) {
        // This can be ignored
        BALL_LOG_INFO << d_state_p->uri()
                      << ": ignoring open upstream notification for subQueueId"
                      << ": " << upstreamSubQueueId
                      << " with the same generation count " << genCount << ".";

        BSLS_ASSERT_SAFE(ctx.d_state == SubStreamContext::e_OPENED);
    }
    else {
        ctx.d_genCount = genCount;

        BALL_LOG_INFO << d_state_p->uri()
                      << ": now has an upstream for subQueueId: "
                      << upstreamSubQueueId << " with generation count "
                      << ctx.d_genCount << ".";

        ctx.d_state = SubStreamContext::e_OPENED;

        if (upstreamSubQueueId == bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
            d_producerState.d_genCount = genCount;
            d_producerState.d_state    = SubStreamContext::e_OPENED;

            retransmitPendingMessagesDispatched(genCount);
        }
        retransmitPendingConfirmsDispatched(upstreamSubQueueId);
    }
}

// ACCESSORS
void RemoteQueue::loadInternals(mqbcmd::RemoteQueue* out) const
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    out->numPendingPuts()                 = d_pendingMessages.size();
    out->numPendingConfirms()             = d_pendingConfirms.size();
    out->isPushExpirationTimerScheduled() = d_pendingMessagesTimerEventHandle;
    int i                                 = 0;
    for (SubQueueIds::const_iterator cit = d_subStreams.begin();
         cit != d_subStreams.end();
         ++cit, ++i) {
        if (cit->d_state != SubStreamContext::e_NONE) {
            mqbcmd::RemoteStreamInfo info(d_allocator_p);
            info.id()       = i;
            info.state()    = SubStreamContext::toAscii(cit->d_state);
            info.genCount() = cit->d_genCount;
            out->streams().push_back(info);
        }
    }

    // QueueEngine
    d_queueEngine_mp->loadInternals(&out->queueEngine());
}

}  // close package namespace
}  // close enterprise namespace
