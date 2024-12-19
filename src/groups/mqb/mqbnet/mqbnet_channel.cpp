// Copyright 2020-2023 Bloomberg Finance L.P.
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

// mqbnet_channel.cpp                                                 -*-C++-*-
#include <mqbnet_channel.h>

#include <mqbscm_version.h>
// BDE
#include <bdlf_bind.h>
#include <bslmt_lockguard.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_systemtime.h>

#include <bmqsys_threadutil.h>
#include <bmqu_printutil.h>

namespace BloombergLP {
namespace mqbnet {

// --------------------------
// class Channel::ControlArgs
// --------------------------

size_t Channel::ControlArgs::eventSize() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    return d_data_sp->length();
}

// --------------------
// class Channel::Stats
// --------------------

void Channel::Stats::reset()
{
    for (int i = 0; i < k_MAX_ITEM_TYPE; ++i) {
        d_numItems[i] = 0;
    }

    d_numItemsTotal = 0;
    d_numBytes      = 0;
}

// -------------
// class Channel
// -------------

Channel::Channel(bdlbb::BlobBufferFactory* blobBufferFactory,
                 const bsl::string&        name,
                 bslma::Allocator*         allocator)
: d_allocators(allocator)
, d_allocator_p(d_allocators.get("Channel"))
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(blobBufferFactory,
                                         d_allocators.get("BlobSpPool")))
, d_putBuilder(&d_blobSpPool, d_allocator_p)
, d_pushBuilder(&d_blobSpPool, d_allocator_p)
, d_ackBuilder(&d_blobSpPool, d_allocator_p)
, d_confirmBuilder(&d_blobSpPool, d_allocator_p)
, d_rejectBuilder(&d_blobSpPool, d_allocator_p)
, d_itemPool(sizeof(Item),
             bsls::BlockGrowth::BSLS_CONSTANT,
             d_allocators.get("ItemPool"))
, d_buffer(1024, allocator)
, d_secondaryBuffer(1024, allocator)
, d_doStop(false)
, d_state(e_INITIAL)
, d_description(name + " - ", d_allocator_p)
, d_name(name, d_allocator_p)
, d_stats()
{
    bslmt::ThreadAttributes attr = bmqsys::ThreadUtil::defaultAttributes();
    bsl::string             threadName("bmqNet-");
    attr.setThreadName(threadName + d_name);
    d_buffer.setWatermarks(50000, 100000, 500000);
    d_buffer.setStateCallback(
        bdlf::MemFnUtil::memFn(&Channel::onBufferStateChange, this));
    int rc = bslmt::ThreadUtil::createWithAllocator(
        &d_threadHandle,
        attr,
        bdlf::MemFnUtil::memFn(&Channel::threadFn, this),
        d_allocator_p);
    BSLS_ASSERT_OPT(rc == 0 && "Failed to create channel thread");
}

Channel::~Channel()
{
    d_doStop = true;

    resetChannel();

    BSLA_MAYBE_UNUSED const int rc = bslmt::ThreadUtil::join(d_threadHandle);
    BSLS_ASSERT_SAFE(rc == 0);
}

void Channel::deleteItem(void* item, void* cookie)
{
    static_cast<Channel*>(cookie)->d_itemPool.deleteObject(
        static_cast<Item*>(item));
}

bmqt::GenericResult::Enum
Channel::writePut(const bmqp::PutHeader&                    ph,
                  const bsl::shared_ptr<bdlbb::Blob>&       data,
                  const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(ph, data, state, d_allocator_p),
                                 this,
                                 deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writePush(const bsl::shared_ptr<bdlbb::Blob>&       payload,
                   int                                       queueId,
                   const bmqt::MessageGUID&                  msgId,
                   int                                       flags,
                   bmqt::CompressionAlgorithmType::Enum      compressionType,
                   const bmqp::MessagePropertiesInfo&        logic,
                   const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
                   const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(queueId,
                                          msgId,
                                          flags,
                                          compressionType,
                                          logic,
                                          payload,
                                          subQueueInfos,
                                          state,
                                          d_allocator_p),
                                 this,
                                 deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writePush(int                                       queueId,
                   const bmqt::MessageGUID&                  msgId,
                   int                                       flags,
                   bmqt::CompressionAlgorithmType::Enum      compressionType,
                   const bmqp::MessagePropertiesInfo&        logic,
                   const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
                   const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(queueId,
                                          msgId,
                                          flags,
                                          compressionType,
                                          logic,
                                          subQueueInfos,
                                          state,
                                          d_allocator_p),
                                 this,
                                 deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writeAck(int                                       status,
                  int                                       correlationId,
                  const bmqt::MessageGUID&                  guid,
                  int                                       queueId,
                  const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(
        new (d_itemPool.allocate())
            Item(status, correlationId, guid, queueId, state, d_allocator_p),
        this,
        deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writeConfirm(int                                       queueId,
                      int                                       subQueueId,
                      const bmqt::MessageGUID&                  guid,
                      const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(queueId,
                                          subQueueId,
                                          guid,
                                          state,
                                          bmqp::EventType::e_CONFIRM,
                                          d_allocator_p),
                                 this,
                                 deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writeReject(int                                       queueId,
                     int                                       subQueueId,
                     const bmqt::MessageGUID&                  guid,
                     const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(queueId,
                                          subQueueId,
                                          guid,
                                          state,
                                          bmqp::EventType::e_REJECT,
                                          d_allocator_p),
                                 this,
                                 deleteItem);
    return enqueue(item);
}

bmqt::GenericResult::Enum
Channel::writeBlob(const bsl::shared_ptr<bdlbb::Blob>&       data,
                   bmqp::EventType::Enum                     type,
                   const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(data, type, state, d_allocator_p),
                                 this,
                                 deleteItem);

    return enqueue(item);
}

void Channel::resetChannel()
{
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

        BALL_LOG_INFO << "Disconnected " << d_description;

        // Need to 'reset' which only the writing thread can do.
        d_state       = e_RESET;
        d_description = d_name + " - ";
        d_channel_wp.reset();

        d_stateCondition.signal();
    }
    // Wake up the writing thread in case it is blocked by 'popFront'
    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(d_allocator_p),
                                 this,
                                 deleteItem);

    d_buffer.pushBack(bslmf::MovableRefUtil::move(item));
}

void Channel::closeChannel()
{
    bsl::shared_ptr<bmqio::Channel> channel;
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
        channel = d_channel_wp.lock();
    }  // UNLOCK

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(channel)) {
        channel->close();
        // Set the state to e_CLOSE to avoid repeated attempts to write until
        // 'OnClose' calls 'resetChannel'.

        d_state.testAndSwap(e_READY, e_CLOSE);
    }
}

void Channel::setChannel(const bsl::weak_ptr<bmqio::Channel>& value)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    bsl::shared_ptr<bmqio::Channel> channelSp = value.lock();
    BSLS_ASSERT(channelSp);
    BSLS_ASSERT(!isAvailable());

    d_description = d_name + " - " + channelSp->peerUri();
    channelSp->onWatermark(bdlf::BindUtil::bind(&Channel::onWatermark,
                                                this,
                                                bdlf::PlaceHolders::_1));
    // type
    d_channel_wp = value;
    d_state      = e_RESET;
    // Signal the writing thread to pick up new channel.
    d_stateCondition.signal();

    while (d_state == e_RESET) {
        // Synchronize with the writing thread.
        // This is to not reject writes after 'setChannel' returns.
        d_stateCondition.wait(&d_mutex);
    }

    BALL_LOG_INFO << "Connected " << d_description;
}

void Channel::reset()
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    d_buffer.reset();

    bsls::Types::Int64 count = numItems();
    if (count) {
        BALL_LOG_INFO << "Reset '" << d_description << "', dropping "
                      << bmqu::PrintUtil::prettyNumber(count) << " items and "
                      << bmqu::PrintUtil::prettyBytes(numBytes()) << " bytes.";
    }

    d_secondaryBuffer.reset();

    d_stats.reset();
    d_putBuilder.reset();
    d_confirmBuilder.reset();
    d_rejectBuilder.reset();
    d_pushBuilder.reset();
    d_ackBuilder.reset();
}

void Channel::flush()
{
    if (!isAvailable()) {
        return;  // RETURN
    }

    bslma::ManagedPtr<Item> item(new (d_itemPool.allocate())
                                     Item(d_allocator_p),
                                 this,
                                 deleteItem);

    d_buffer.pushBack(bslmf::MovableRefUtil::move(item));
}

void Channel::onWatermark(bmqio::ChannelWatermarkType::Enum type)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    switch (type) {
    case bmqio::ChannelWatermarkType::e_LOW_WATERMARK:
        BALL_LOG_INFO << "[CHANNEL_LOW_WATERMARK] hit for '" << d_description
                      << "' with "
                      << bmqu::PrintUtil::prettyNumber(
                             bsls::Types::Int64(d_stats.d_numItemsTotal))
                      << " items and "
                      << bmqu::PrintUtil::prettyBytes(d_stats.d_numBytes)
                      << " pending bytes.";
        d_state = e_LWM;
        d_stateCondition.signal();
        break;
    case bmqio::ChannelWatermarkType::e_HIGH_WATERMARK:
        BALL_LOG_WARN << "[CHANNEL_HIGH_WATERMARK] hit for '" << d_description
                      << "' with "
                      << bmqu::PrintUtil::prettyNumber(
                             bsls::Types::Int64(d_stats.d_numItemsTotal))
                      << " items and "
                      << bmqu::PrintUtil::prettyBytes(d_stats.d_numBytes)
                      << " pending bytes.";
        d_state = e_HWM;
        break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown watermark type");
    }
    }
}

// PRIVATE MANIPULATORS

bmqt::GenericResult::Enum
Channel::writeBufferedItem(bool*                                  isConsumed,
                           const bsl::shared_ptr<bmqio::Channel>& channel,
                           const bsl::string&                     description,
                           Item&                                  item)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    // Convert 'Item' into corresponding args and execute the write.
    bmqt::GenericResult::Enum rc = bmqt::GenericResult::e_SUCCESS;
    switch (item.d_type) {
    case bmqp::EventType::e_UNDEFINED:
        BSLS_ASSERT_SAFE(false && "unexpected UNDEFINED item");
        break;
    case bmqp::EventType::e_PUT:
        rc = writeImmediate(isConsumed,
                            channel,
                            description,
                            d_putBuilder,
                            PutArgs(item),
                            item.d_state);
        break;
    case bmqp::EventType::e_PUSH:
        if (item.d_data_sp) {
            rc = writeImmediate(isConsumed,
                                channel,
                                description,
                                d_pushBuilder,
                                ExplicitPushArgs(item),
                                item.d_state);
        }
        else {
            rc = writeImmediate(isConsumed,
                                channel,
                                description,
                                d_pushBuilder,
                                ImplicitPushArgs(item),
                                item.d_state);
        }
        break;
    case bmqp::EventType::e_CONFIRM:
        rc = writeImmediate(isConsumed,
                            channel,
                            description,
                            d_confirmBuilder,
                            ConfirmArgs(item),
                            item.d_state);
        break;
    case bmqp::EventType::e_ACK:
        rc = writeImmediate(isConsumed,
                            channel,
                            description,
                            d_ackBuilder,
                            AckArgs(item),
                            item.d_state);
        break;
    case bmqp::EventType::e_REJECT:
        rc = writeImmediate(isConsumed,
                            channel,
                            description,
                            d_rejectBuilder,
                            RejectArgs(item),
                            item.d_state);
        break;
    case bmqp::EventType::e_CONTROL:
    case bmqp::EventType::e_CLUSTER_STATE:
    case bmqp::EventType::e_ELECTOR:
    case bmqp::EventType::e_STORAGE:
    case bmqp::EventType::e_RECOVERY:
    case bmqp::EventType::e_PARTITION_SYNC:
    case bmqp::EventType::e_HEARTBEAT_REQ:
    case bmqp::EventType::e_HEARTBEAT_RSP:
    case bmqp::EventType::e_REPLICATION_RECEIPT:
    default: {
        ControlArgs x(item);
        rc = writeImmediate(isConsumed,
                            channel,
                            description,
                            x,
                            x,
                            item.d_state);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                rc == bmqt::GenericResult::e_NOT_READY)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // not keeping the control pseudo builder
            *isConsumed = false;
        }
    } break;
    }

    return rc;
}

bmqt::EventBuilderResult::Enum Channel::pack(bmqp::PutEventBuilder& builder,
                                             const PutArgs&         args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    if (!args.d_data_sp.get()) {
        return bmqt::EventBuilderResult::e_PAYLOAD_EMPTY;
    }
    const bmqp::PutHeader& ph = args.d_putHeader;

    builder.startMessage();
    builder.setMessageGUID(ph.messageGUID())
        .setFlags(ph.flags())
        .setMessagePayload(args.d_data_sp.get())
        .setCompressionAlgorithmType(ph.compressionAlgorithmType())
        .setCrc32c(ph.crc32c())
        .setMessagePropertiesInfo(bmqp::MessagePropertiesInfo(ph));

    // TBD: groupId: use PutEventBuilder::setOptionsRaw(optionsSp.get()) here

    return builder.packMessageRaw(ph.queueId());
}

bmqt::EventBuilderResult::Enum Channel::pack(bmqp::PushEventBuilder& builder,
                                             const ExplicitPushArgs& args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    bmqt::EventBuilderResult::Enum rc = builder.addSubQueueInfosOption(
        args.d_subQueueInfos);
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            rc == bmqt::EventBuilderResult::e_SUCCESS)) {
        rc = builder.packMessage(*args.d_data_sp,
                                 args.d_queueId,
                                 args.d_msgId,
                                 args.d_flags,
                                 args.d_compressionAlgorithmType,
                                 args.d_messagePropertiesInfo);
    }
    return rc;
}

bmqt::EventBuilderResult::Enum Channel::pack(bmqp::PushEventBuilder& builder,
                                             const ImplicitPushArgs& args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    bmqt::EventBuilderResult::Enum rc = builder.addSubQueueInfosOption(
        args.d_subQueueInfos);
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            rc == bmqt::EventBuilderResult::e_SUCCESS)) {
        rc = builder.packMessage(args.d_queueId,
                                 args.d_msgId,
                                 args.d_flags,
                                 args.d_compressionAlgorithmType,
                                 args.d_messagePropertiesInfo);
    }
    return rc;
}

bmqt::EventBuilderResult::Enum Channel::pack(bmqp::AckEventBuilder& builder,
                                             const AckArgs&         args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    return builder.appendMessage(args.d_status,
                                 args.d_correlationId,
                                 args.d_guid,
                                 args.d_queueId);
}

bmqt::EventBuilderResult::Enum Channel::pack(bmqp::RejectEventBuilder& builder,
                                             const RejectArgs&         args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    return builder.appendMessage(args.d_queueId,
                                 args.d_subQueueId,
                                 args.d_guid);
}

bmqt::EventBuilderResult::Enum
Channel::pack(bmqp::ConfirmEventBuilder& builder, const ConfirmArgs& args)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    return builder.appendMessage(args.d_queueId,
                                 args.d_subQueueId,
                                 args.d_guid);
}

bmqt::EventBuilderResult::Enum Channel::pack(ControlArgs& builder,
                                             const ControlArgs&)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    if (builder.d_messageCount) {
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    return bmqt::EventBuilderResult::e_SUCCESS;
}

bmqio::StatusCategory::Enum
Channel::flushAll(const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    bmqio::StatusCategory::Enum rc;

    rc = flushBuilder(d_pushBuilder, channel);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }
    rc = flushBuilder(d_confirmBuilder, channel);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }
    rc = flushBuilder(d_putBuilder, channel);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }
    rc = flushBuilder(d_ackBuilder, channel);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }
    rc = flushBuilder(d_rejectBuilder, channel);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }

    return bmqio::StatusCategory::e_SUCCESS;
}

template <typename Builder, typename Args>
bmqt::GenericResult::Enum
Channel::writeImmediate(bool*                                     isConsumed,
                        const bsl::shared_ptr<bmqio::Channel>&    channel,
                        const bsl::string&                        description,
                        Builder&                                  builder,
                        const Args&                               args,
                        const bsl::shared_ptr<bmqu::AtomicState>& state)
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    if (state) {
        if (!state->process()) {
            // That state had 'cancel' call.
            return bmqt::GenericResult::e_CANCELED;
        }
    }

    bmqt::GenericResult::Enum rc = bmqt::GenericResult::e_SUCCESS;
    *isConsumed                  = true;

    // This can take 2 iterations in the case when 'e_EVENT_TOO_BIG'.
    do {
        bmqt::EventBuilderResult::Enum buildRc = pack(builder, args);
        bool                           doFlush = false;

        switch (buildRc) {
        case bmqt::EventBuilderResult::e_SUCCESS: {
            *isConsumed = true;  // The builder has the data
            if (builder.eventSize() >= k_NAGLE_PACKET_SIZE) {
                doFlush = true;
            }
            else {
                rc = bmqt::GenericResult::e_SUCCESS;
            }
        } break;
        case bmqt::EventBuilderResult::e_EVENT_TOO_BIG:
        case bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG:
        case bmqt::EventBuilderResult::e_OPTION_TOO_BIG: {
            if (*isConsumed) {
                // First iteration.
                doFlush = true;
            }
            else {
                // Second iteration.  Do not retry more.
                rc = bmqt::GenericResult::e_INVALID_ARGUMENT;
            }
            *isConsumed = false;
        } break;
#ifdef BMQ_ENABLE_MSG_GROUPID
        case bmqt::EventBuilderResult::e_INVALID_MSG_GROUP_ID:
#endif
        case bmqt::EventBuilderResult::e_PAYLOAD_EMPTY:
        case bmqt::EventBuilderResult::e_MISSING_CORRELATION_ID:
        case bmqt::EventBuilderResult::e_QUEUE_READONLY:
        case bmqt::EventBuilderResult::e_QUEUE_INVALID:
        case bmqt::EventBuilderResult::e_QUEUE_SUSPENDED:
        case bmqt::EventBuilderResult::e_UNKNOWN:
        default: rc = bmqt::GenericResult::e_INVALID_ARGUMENT; break;
        }

        if (doFlush) {
            bmqio::StatusCategory::Enum writeRc = flushBuilder(builder,
                                                               channel);
            switch (writeRc) {
            case bmqio::StatusCategory::e_SUCCESS:
                rc = bmqt::GenericResult::e_SUCCESS;
                break;
            case bmqio::StatusCategory::e_LIMIT:
                rc = bmqt::GenericResult::e_NOT_READY;
                break;
            case bmqio::StatusCategory::e_GENERIC_ERROR:
            case bmqio::StatusCategory::e_CONNECTION:
            case bmqio::StatusCategory::e_TIMEOUT:
            case bmqio::StatusCategory::e_CANCELED:
            default:
                BALL_LOG_ERROR
                    << "#CLUSTER_SEND_FAILURE "
                    << "Failed to write event to the node " << description
                    << ", rc: " << writeRc << ". Number of msgs in the event: "
                    << builder.messageCount()
                    << ", event size: " << builder.eventSize() << " bytes.";
                rc = bmqt::GenericResult::e_NOT_CONNECTED;
                channel->close();
                break;
            }
        }

    } while (rc == bmqt::GenericResult::e_SUCCESS && !*isConsumed);

    return rc;
}

void Channel::threadFn()
{
    // executed by the internal thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_internalThreadChecker.inSameThread());

    bslma::ManagedPtr<Item>         item;
    bsl::shared_ptr<bmqio::Channel> channel;
    bsl::string                     description;
    int                             mode = e_BLOCK;

    BSLS_ASSERT(d_state == e_INITIAL || d_state == e_RESET);

    while (!d_doStop) {
        bmqc::MonitoredQueueState::Enum queueState;
        while (d_queueStates.tryPopFront(&queueState) == 0) {
            switch (queueState) {
            case bmqc::MonitoredQueueState::e_NORMAL: {
                BALL_LOG_INFO << "Buffer is in normal state ("
                              << d_buffer.lowWatermark() << ") for channel "
                              << description << " with " << numItems()
                              << " items and "
                              << bmqu::PrintUtil::prettyBytes(numBytes())
                              << " pending bytes";
            } break;
            case bmqc::MonitoredQueueState::e_HIGH_WATERMARK_REACHED: {
                BALL_LOG_WARN << "[CHANNEL_BUFFER_HIGH_WATERMARK] reached ("
                              << d_buffer.highWatermark() << ") for channel "
                              << description << " with " << numItems()
                              << " items and "
                              << bmqu::PrintUtil::prettyBytes(numBytes())
                              << " pending bytes";
            } break;
            case bmqc::MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED: {
                BALL_LOG_ERROR << "[CHANNEL_BUFFER_HIGH_WATERMARK2] reached ("
                               << d_buffer.highWatermark2() << ") for channel "
                               << description << " with " << numItems()
                               << " items and "
                               << bmqu::PrintUtil::prettyBytes(numBytes())
                               << " pending bytes";
            } break;
            case bmqc::MonitoredQueueState::e_QUEUE_FILLED: {
                // We're using an unbounded queue so this state is not
                // expected.
                BALL_LOG_ERROR << "Unexpected queue state for buffer "
                               << "associated with channel " << description;
            } break;
            default: {
                BSLS_ASSERT_SAFE(false && "Unknown queue state");
            }
            }
        }
        if (d_state != e_READY) {
            bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

            if (d_state == e_RESET) {
                // This is the only place to get out of the 'e_RESET' state.
                item.reset();
                reset();

                d_state = e_INITIAL;
                mode    = e_BLOCK;
            }

            if (d_state == e_INITIAL) {
                channel     = d_channel_wp.lock();
                description = d_description;

                if (channel) {
                    // This is the only place for transitions:
                    //  e_IDLE  -> e_READY
                    //  e_RESET -> e_READY
                    d_state = e_READY;
                }
            }
            else if (d_state == e_LWM) {
                d_state = e_READY;
            }
            // The state can be 'e_HWM' in which case 'onWatermark' transitions
            //  e_HWM  -> e_READY

            if (d_state == e_READY) {
                // 'setChannel' is waiting for the writing thread to pick up
                // new channel after resetting 'd_buffer' and all builders.
                d_stateCondition.signal();
                BALL_LOG_INFO << "Ready to write to " << description
                              << " with " << numItems() << " items and "
                              << bmqu::PrintUtil::prettyBytes(numBytes())
                              << " pending bytes";
            }
            else {
                // wait for 'setChannel' or LWM
                BALL_LOG_INFO << "Waiting for " << description << " with "
                              << numItems() << " items and "
                              << bmqu::PrintUtil::prettyBytes(numBytes())
                              << " pending bytes";
                d_stateCondition.wait(&d_mutex);
            }
        }
        else if (!item) {  // UNLOCK

            switch (mode) {
            case e_BLOCK: {
                if (d_buffer.popFront(&item) == 0) {
                    BSLS_ASSERT_SAFE(item);
                    mode = e_PRIMARY;
                }
            } break;
            case e_PRIMARY: {
                if (d_buffer.tryPopFront(&item)) {
                    // Primary is empty.  Flush builders and drain secondary.

                    mode = e_IDLE;
                }
                else if (item->isSecondary()) {
                    // Instead of writing immediately, push it to the secondary
                    // buffer which will get flushed later.
                    // This is done to reduce the rate of 'compactable' items
                    // such as Replication Receipts which can accumulate
                    // multiple receipts in one item.
                    d_secondaryBuffer.pushBack(
                        bslmf::MovableRefUtil::move(item));
                    item.reset();
                }
            } break;
            case e_IDLE: {
                // Idle.  First, flush all builders.
                if (flushAll(channel) == bmqio::StatusCategory::e_SUCCESS) {
                    // Then, drain the secondary buffer
                    mode = e_SECONDARY;
                }
            } break;
            case e_SECONDARY: {
                if (d_secondaryBuffer.tryPopFront(&item)) {
                    mode = e_BLOCK;
                    BSLS_ASSERT_SAFE(!item);
                }
            } break;
            }
        }
        else if (item->d_type == bmqp::EventType::e_UNDEFINED) {
            // Enqueued by 'resetChannel' to wake us up.  Ignore.
            item.reset();
            // If this was 'flush', drain secondary items.
            mode = e_IDLE;
        }
        else {
            // e_READY and have channel and an item
            BSLS_ASSERT_SAFE(item);
            BSLS_ASSERT_SAFE(channel);

            bool                      isConsumed = true;
            bmqt::GenericResult::Enum rc =
                writeBufferedItem(&isConsumed, channel, description, *item);

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    rc == bmqt::GenericResult::e_NOT_READY)) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                BALL_LOG_WARN << "Reached a limit while writing event "
                              << item->d_type << " to " << description
                              << " with " << numItems() << " items and "
                              << bmqu::PrintUtil::prettyBytes(numBytes())
                              << " pending bytes";
                // If 'onWatermark' did not happen yet, do go into e_HWM to
                // avoid calling BTE until LWM
                d_state.testAndSwap(e_READY, e_HWM);
            }
            else if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                         rc == bmqt::GenericResult::e_NOT_CONNECTED)) {
                // Set the state to e_CLOSE to avoid repeated attempts to write
                // until 'OnClose' calls 'resetChannel'.

                d_state.testAndSwap(e_READY, e_CLOSE);
            }

            // 'e_NOT_READY' can be the result of flashing the builder
            // either before or after the builder consumes the item.
            // 'isConsumed' indicates the latter.
            if (isConsumed) {
                // done with the 'item'

                d_stats.removeItem(item->d_type, item->d_numBytes);
                item.reset();
            }
            // else keep the item
        }
    }
    reset();
}

void Channel::onBufferStateChange(bmqc::MonitoredQueueState::Enum state)
{
    // Assuming 'd_buffer' is not empty.  Signal 'd_stateCondition' in case
    // 'threadFn' is waiting (for LWM).  Signal without locking since
    // 'threadFn' does not wait on 'd_queueState'.

    d_queueStates.pushBack(state);

    // 'threadFn' can be waiting for 'd_stateCondition' as in the case when in
    // HWM.  Signal to wake up 'threadFn' so it can log the event.
    d_stateCondition.signal();
}

}  // close package namespace
}  // close enterprise namespace
