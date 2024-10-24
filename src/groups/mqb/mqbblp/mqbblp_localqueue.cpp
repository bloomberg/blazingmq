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

// mqbblp_localqueue.cpp                                              -*-C++-*-
#include <mqbblp_localqueue.h>

#include <mqbscm_version.h>
// MQB
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_rootqueueengine.h>
#include <mqbblp_storagemanager.h>
#include <mqbcmd_messages.h>
#include <mqbi_domain.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_capacitymeter.h>
#include <mqbu_storagekey.h>

// BMQ
#include <bmqp_protocolutil.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_uri.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlb_print.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

// ----------------
// class LocalQueue
// ----------------

// CREATORS
LocalQueue::LocalQueue(QueueState* state, bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_state_p(state)
, d_queueEngine_mp(0)
, d_throttledFailedPutMessages(5000, 1)  // 1 log per 5s interval
, d_hasNewMessages(false)
, d_throttledDuplicateMessages()
, d_haveStrongConsistency(false)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->id() == bmqp::QueueId::k_PRIMARY_QUEUE_ID);
    // A LocalQueue has no 'upstream', hence no id

    d_throttledDuplicateMessages.initialize(
        1,
        1 * bdlt::TimeUnitRatio::k_NS_PER_S);
    // Maximum one per 1 seconds

    d_state_p->setDescription(d_state_p->uri().asString());
}

int LocalQueue::configure(bsl::ostream& errorDescription, bool isReconfigure)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_STORAGE_CREATION_FAILURE = -1,
        rc_INCOMPATIBLE_STORAGE     = -2,
        rc_UNKNOWN_DOMAIN_CONFIG    = -3,
        rc_QUEUE_ENGINE_CFG_FAILURE = -4,
        rc_STORAGE_CFG_FAILURE      = -5
    };

    int                     rc        = 0;
    mqbi::Queue*            queue     = d_state_p->queue();
    const mqbconfm::Domain& domainCfg = d_state_p->domain()->config();

    // Create the associated storage.
    if (!d_state_p->storage()) {
        BSLS_ASSERT_OPT(!isReconfigure);  // Should be first configure.

        // Only create a storage if this is the initial configure; reconfigure
        // should reuse the previously created storage.
        bslma::ManagedPtr<mqbi::Storage> storageMp;
        rc = d_state_p->storageManager()->makeStorage(
            errorDescription,
            &storageMp,
            d_state_p->uri(),
            d_state_p->key(),
            d_state_p->partitionId(),
            domainCfg.messageTtl(),
            domainCfg.maxDeliveryAttempts(),
            domainCfg.storage());
        if (rc != 0) {
            return 10 * rc + rc_STORAGE_CREATION_FAILURE;  // RETURN
        }

        if (d_state_p->isAtMostOnce()) {
            storageMp->capacityMeter()->disable();
        }

        if (!d_state_p->isStorageCompatible(storageMp)) {
            errorDescription << "Incompatible storage type for LocalQueue "
                             << "[uri: " << d_state_p->uri() << ", key: '"
                             << d_state_p->key()
                             << "', partitionId: " << d_state_p->partitionId()
                             << "]";
            return rc_INCOMPATIBLE_STORAGE;  // RETURN
        }

        d_state_p->setStorage(storageMp);
    }
    else {
        rc = d_state_p->storage()->configure(errorDescription,
                                             domainCfg.storage().config(),
                                             domainCfg.storage().queueLimits(),
                                             domainCfg.messageTtl(),
                                             domainCfg.maxDeliveryAttempts());
        if (rc) {
            return 10 * rc + rc_STORAGE_CFG_FAILURE;  // RETURN
        }
    }

    // Create the associated queue-engine. This can either be on first
    // configure, or during transition from remote to local queue. Note that
    // during update of a domain's configuration, this object already exists,
    // and should not be re-created.
    if (!d_queueEngine_mp) {
        RootQueueEngine::create(&d_queueEngine_mp,
                                d_state_p,
                                domainCfg,
                                d_allocator_p);
    }

    rc = d_queueEngine_mp->configure(errorDescription);
    if (rc != 0) {
        return 10 * rc + rc_QUEUE_ENGINE_CFG_FAILURE;  // RETURN
    }

    d_haveStrongConsistency = domainCfg.consistency().isStrongValue();

    // Inform the storage about the queue.
    d_state_p->storageManager()->setQueueRaw(queue,
                                             d_state_p->uri(),
                                             d_state_p->partitionId());

    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_CHANGE_ROLE,
        mqbstat::QueueStatsDomain::Role::e_PRIMARY);

    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_CFG_MSGS,
        domainCfg.storage().queueLimits().messages());

    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_CFG_BYTES,
        domainCfg.storage().queueLimits().bytes());

    if (isReconfigure) {
        if (domainCfg.mode().isFanoutValue()) {
            d_state_p->stats().updateDomainAppIds(
                domainCfg.mode().fanout().appIDs());
        }
    }

    BALL_LOG_INFO << "Created a LocalQueue "
                  << "[uri: '" << d_state_p->uri() << "'"
                  << ", key: '" << d_state_p->key()
                  << "', partitionId: " << d_state_p->partitionId()
                  << ", processorHandle: "
                  << queue->dispatcherClientData().processorHandle() << "]";

    return rc_SUCCESS;
}

void LocalQueue::resetState()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_queueEngine_mp->resetState();
    d_queueEngine_mp->afterQueuePurged(bmqp::ProtocolUtil::k_NULL_APP_ID,
                                       mqbu::StorageKey::k_NULL_KEY);
}

int LocalQueue::importState(bsl::ostream& errorDescription)
{
    return d_queueEngine_mp->rebuildInternalState(errorDescription);
}

void LocalQueue::close()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));
    BSLS_ASSERT_SAFE(d_state_p->storage());

    mqbi::Storage* storage = d_state_p->storage();
    BALL_LOG_INFO << "Closing queue "
                  << "[ uri: '" << d_state_p->uri() << "'"
                  << ", key:" << d_state_p->key()
                  << ", storageOutstandingMessages: "
                  << storage->numMessages(mqbu::StorageKey::k_NULL_KEY) << "]";

    // TBD: [REFACTOR]
    //
    // Currently we assume that Queue.close is the last event for this queue in
    // the dispatcher (this assumption is based on the fact that
    // transportManager is stopped before domainManager).  If this ever
    // changes, we may need to do something similar to session by keeping a
    // 'd_shutdownInProgress' variable.

    // NOTE: we do not delete the storage because in case of persistent queues,
    //       a storage may need to outlive its queue.  In such cases, storage
    //       is eventually deleted by the StorageManager.

    storage->close();
}

void LocalQueue::getHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bmqp_ctrlmsg::QueueHandleParameters&                handleParameters,
    unsigned int                                upstreamSubQueueId,
    const mqbi::QueueHandle::GetHandleCallback& callback)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_queueEngine_mp->getHandle(clientContext,
                                handleParameters,
                                upstreamSubQueueId,
                                callback);
}

void LocalQueue::configureHandle(
    mqbi::QueueHandle*                                 handle,
    const bmqp_ctrlmsg::StreamParameters&              streamParameters,
    const mqbi::QueueHandle::HandleConfiguredCallback& configuredCb)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // As part of configuring a handle, engine may attempt to deliver messages
    // to it.  We need to make sure that storage/replication is in sync, and
    // thus, we force-flush the file store.

    d_state_p->storage()->dispatcherFlush(true, false);

    // Attempt to deliver all data in the storage.  Otherwise, broadcast
    // can get dropped if the incoming configure request removes consumers.

    deliverIfNeeded();

    d_queueEngine_mp->configureHandle(handle, streamParameters, configuredCb);
}

void LocalQueue::releaseHandle(
    mqbi::QueueHandle*                               handle,
    const bmqp_ctrlmsg::QueueHandleParameters&       handleParameters,
    bool                                             isFinal,
    const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_state_p->storage()->dispatcherFlush(true, false);

    d_queueEngine_mp->releaseHandle(handle,
                                    handleParameters,
                                    isFinal,
                                    releasedCb);
}

void LocalQueue::onDispatcherEvent(const mqbi::DispatcherEvent& event)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    switch (event.type()) {
    case mqbi::DispatcherEventType::e_PUT: {
        const mqbi::DispatcherPutEvent* realEvent =
            &event.getAs<mqbi::DispatcherPutEvent>();

        postMessage(realEvent->putHeader(),
                    realEvent->blob(),
                    realEvent->options(),
                    realEvent->queueHandle());
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_CALLBACK: {
        const mqbi::DispatcherCallbackEvent* realEvent =
            &event.getAs<mqbi::DispatcherCallbackEvent>();
        BSLS_ASSERT_SAFE(realEvent->callback());
        realEvent->callback()(
            d_state_p->queue()->dispatcherClientData().processorHandle());
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_ACK: {
        BALL_LOG_INFO << "Skipping dispatcher event [" << event << "] "
                      << "for queue [" << d_state_p->description() << "], "
                      << "which is a leftover from remote->local queue "
                      << "conversion.";
    } break;  // BREAK
    case mqbi::DispatcherEventType::e_UNDEFINED:
    case mqbi::DispatcherEventType::e_DISPATCHER:
    case mqbi::DispatcherEventType::e_CONTROL_MSG:
    case mqbi::DispatcherEventType::e_CONFIRM:
    case mqbi::DispatcherEventType::e_REJECT:
    case mqbi::DispatcherEventType::e_PUSH:
    case mqbi::DispatcherEventType::e_CLUSTER_STATE:
    case mqbi::DispatcherEventType::e_STORAGE:
    case mqbi::DispatcherEventType::e_RECOVERY:
    case mqbi::DispatcherEventType::e_REPLICATION_RECEIPT:
    default: {
        BALL_LOG_ERROR << "#UNEXPECTED_EVENT "
                       << "Queue '" << d_state_p->description()
                       << "' received an invalid"
                       << " dispatcher event [" << event << "]";
    } break;  // BREAK
    }
}

void LocalQueue::flush()
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    // If 'configure' fails, the queue remains registered with the dispatcher
    // until it gets rolled back.  If 'flush' gets called in between, the queue
    // may have no storage.
    if (d_state_p->storage()) {
        d_state_p->storage()->dispatcherFlush(true, false);
        // See notes in 'FileStore::dispatcherFlush' for motivation behind
        // this flush.
    }

    deliverIfNeeded();
}

void LocalQueue::postMessage(const bmqp::PutHeader&              putHeader,
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
    bmqp::MessagePropertiesInfo translation =
        d_state_p->queue()->schemaLearner().multiplex(
            source->schemaLearnerContext(),
            bmqp::MessagePropertiesInfo(putHeader));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!bmqt::QueueFlagsUtil::isWriter(
            source->handleParameters().flags()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Either queue was not opened in the WRITE mode (which should have
        // been caught in the SDK) or client is posting a message after closing
        // or reconfiguring the queue (which may not be caught in the SDK).
        BMQU_THROTTLEDACTION_THROTTLE(
            d_throttledFailedPutMessages,
            BALL_LOG_WARN
                << "#CLIENT_IMPROPER_BEHAVIOR "
                << "Failed PUT message for queue [" << d_state_p->uri()
                << "] from client [" << source->client()->description()
                << "]. Queue not opened in WRITE mode by the client.";);

        // Note that a NACK is not sent in this case.  This is a case of client
        // violating the contract, by attempting to post a message after
        // closing/reconfiguring the queue.  Since this is out of contract, its
        // ok not to send the NACK.  If it is still desired to send a NACK, it
        // will need some enqueuing b/w client and queue dispatcher threads to
        // ensure that despite NACKs being sent, closeQueue response is still
        // the last event to be sent to the client for the given queue.

        return;  // RETURN
    }

    const bsls::Types::Int64 timePoint = bmqsys::Time::highResolutionTimer();
    const bool               doAck     = bmqp::PutHeaderFlagUtil::isSet(
        putHeader.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    // Absence of 'queueHandle' in the 'attributes' means no 'e_ACK_REQUESTED'.

    bsls::Types::Uint64 timestamp = bdlt::EpochUtil::convertToTimeT64(
        bdlt::CurrentTime::utc());

    // EXPERIMENTAL:
    //  Evaluate 'auto' subscriptions
    mqbi::StorageResult::Enum res =
        d_queueEngine_mp->evaluateAutoSubscriptions(putHeader,
                                                    appData,
                                                    translation,
                                                    timestamp);

    bool         haveReceipt = true;
    unsigned int refCount    = d_queueEngine_mp->messageReferenceCount();

    if (res == mqbi::StorageResult::e_SUCCESS) {
        if (refCount) {
            // Note that arrival timepoint is used only at the primary node,
            // for calculating and reporting the time interval for which a
            // message stays in the queue.
            mqbi::StorageMessageAttributes attributes(
                timestamp,
                refCount,
                translation,
                putHeader.compressionAlgorithmType(),
                !d_haveStrongConsistency,
                doAck ? source : 0,
                putHeader.crc32c(),
                timePoint);  // Arrival Timepoint

            res = d_state_p->storage()->put(&attributes,
                                            putHeader.messageGUID(),
                                            appData,
                                            options);

            haveReceipt = attributes.hasReceipt();
        }
        // else all subscriptions are negative
    }

    // Send acknowledgement if post failed or if ack was requested (both could
    // be true as well).
    if (res != mqbi::StorageResult::e_SUCCESS || haveReceipt) {
        // Calculate time delta between PUT and ACK
        const bsls::Types::Int64 timeDelta =
            bmqsys::Time::highResolutionTimer() - timePoint;
        d_state_p->stats().onEvent(
            mqbstat::QueueStatsDomain::EventType::e_ACK_TIME,
            timeDelta);
        if (res != mqbi::StorageResult::e_SUCCESS || doAck) {
            bmqp::AckMessage ackMessage;
            ackMessage
                .setStatus(bmqp::ProtocolUtil::ackResultToCode(
                    mqbi::StorageResult::toAckResult(res)))
                .setMessageGUID(putHeader.messageGUID());
            // CorrelationId & QueueId are left unset as those fields will
            // be filled downstream.

            source->onAckMessage(ackMessage);
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(res ==
                                            mqbi::StorageResult::e_SUCCESS)) {
        // Message has been saved in the storage, but we don't indicate the
        // engine yet of the new message, instead we just update the
        // 'd_hasNewMessages' flag.  This is because storage (replicated)
        // messages are nagled, and we don't want to indicate to a peer to
        // deliver a particular guid downstream, before actually replicating
        // that message.  So notification to deliver a particular guid
        // downstream is sent to the peer only after storage messages have been
        // flushed (which occurs in 'flush' routine).  In no case should
        // 'afterNewMessage' be called here.

        d_state_p->stats().onEvent(mqbstat::QueueStatsDomain::EventType::e_PUT,
                                   appData->length());

        if (haveReceipt && refCount) {
            d_hasNewMessages = true;
        }
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (res == mqbi::StorageResult::e_DUPLICATE) {
            if (d_throttledDuplicateMessages.requestPermission()) {
                BALL_LOG_WARN << "Duplicate PUT message queue ["
                              << d_state_p->uri() << "] from client ["
                              << source->client()->description() << "], GUID ["
                              << putHeader.messageGUID() << "]";
            }
        }
        else {
            d_state_p->stats().onEvent(
                mqbstat::QueueStatsDomain::EventType::e_NACK,
                1);
        }
    }

    // If 'FileStore::d_storageEventBuilder' is flushed, flush all relevant
    // queues (call 'afterNewMessage' to deliver accumulated data)
    d_state_p->storage()->dispatcherFlush(false, true);
}

void LocalQueue::onPushMessage(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& blob)
{
    BSLS_ASSERT_OPT(false &&
                    "onPushMessage should not be called on LocalQueue");
}

void LocalQueue::onReceipt(const bmqt::MessageGUID&  msgGUID,
                           mqbi::QueueHandle*        qH,
                           const bsls::Types::Int64& arrivalTimepoint)
{
    // Calculate time delta between PUT and ACK
    const bsls::Types::Int64 timeDelta = bmqsys::Time::highResolutionTimer() -
                                         arrivalTimepoint;

    d_state_p->stats().onEvent(
        mqbstat::QueueStatsDomain::EventType::e_ACK_TIME,
        timeDelta);

    if (d_state_p->handleCatalog().hasHandle(qH)) {
        // Send acknowledgement
        bmqp::AckMessage ackMessage;
        ackMessage
            .setStatus(bmqp::ProtocolUtil::ackResultToCode(
                bmqt::AckResult::e_SUCCESS))
            .setMessageGUID(msgGUID);
        // CorrelationId & QueueId are left unset as those fields will be
        // filled downstream.
        qH->onAckMessage(ackMessage);
    }  // else the handle is gone

    d_hasNewMessages = true;
}

void LocalQueue::onRemoval(const bmqt::MessageGUID& msgGUID,
                           mqbi::QueueHandle*       qH,
                           bmqt::AckResult::Enum    result)
{
    // TODO: do we need to update NACK stats considering that downstream can
    // NACK the same GUID as well?

    d_state_p->stats().onEvent(mqbstat::QueueStatsDomain::EventType::e_NACK,
                               1);

    if (d_state_p->handleCatalog().hasHandle(qH)) {
        // Send negative acknowledgement
        bmqp::AckMessage ackMessage;
        ackMessage.setStatus(bmqp::ProtocolUtil::ackResultToCode(result))
            .setMessageGUID(msgGUID);
        // CorrelationId & QueueId are left unset as those fields will be
        // filled downstream.
        qH->onAckMessage(ackMessage);
    }  // else the handle is gone
}

void LocalQueue::deliverIfNeeded()
{
    // Now that storage messages have been flushed, notify the engine (and thus
    // any peer or downstream client) to deliver next applicable message.

    if (d_hasNewMessages) {
        d_hasNewMessages = false;
        d_queueEngine_mp->afterNewMessage(bmqt::MessageGUID(),
                                          static_cast<mqbi::QueueHandle*>(0));
    }
}

void LocalQueue::confirmMessage(const bmqt::MessageGUID& msgGUID,
                                unsigned int             upstreamSubQueueId,
                                mqbi::QueueHandle*       source)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    BALL_LOG_TRACE << "OnConfirm [queue: '" << d_state_p->description()
                   << "', client: '" << *(source->client()) << "', GUID: '"
                   << msgGUID << "']";

    int rc = d_queueEngine_mp->onConfirmMessage(source,
                                                msgGUID,
                                                upstreamSubQueueId);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc == 1)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Still some references to that message, we can't delete it.
        return;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
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

    int                       msgSize = 0;
    mqbi::StorageResult::Enum res     = d_state_p->storage()->remove(msgGUID,
                                                                 &msgSize);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            res != mqbi::StorageResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_WARN << "#QUEUE_CONFIRM_FAILURE "
                      << "Error '" << res
                      << "' while writing deletion record for queue:"
                      << " '" << d_state_p->description() << "', client: '"
                      << *(source->client()) << "', GUID: '" << msgGUID
                      << "']";
        return;  // RETURN
    }
}

int LocalQueue::rejectMessage(const bmqt::MessageGUID& msgGUID,
                              unsigned int             upstreamSubQueueId,
                              mqbi::QueueHandle*       source)
{
    return d_queueEngine_mp->onRejectMessage(source,
                                             msgGUID,
                                             upstreamSubQueueId);
}

void LocalQueue::loadInternals(mqbcmd::LocalQueue* out) const
{
    // executed by the *QUEUE* dispatcher thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_state_p->queue()->dispatcher()->inDispatcherThread(
        d_state_p->queue()));

    d_queueEngine_mp->loadInternals(&out->queueEngine());
}

mqbi::Domain* LocalQueue::domain() const
{
    return d_state_p->domain();
}

}  // close package namespace
}  // close enterprise namespace
