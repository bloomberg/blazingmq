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

#ifndef INCLUDED_MQBNET_CHANNEL
#define INCLUDED_MQBNET_CHANNEL

//@PURPOSE: Provide a mechanism to handle bmqio::Channel watermarks.  This is
// different from ChannelPool buffer for the following reasons:
//  - cancel write if it has not been processed (for example, to expire PUTs)
//  - a room for overload condition handling when different types need
//    different treatment (NACKing excessive PUTs).
//
//@CLASSES:
//  mqbnet::Channel    : Mechanism to interact with bmqio::Channel.
//
//@DESCRIPTION: Methods to build and write PUT, PUSH, ACK, CONFIRM, and control
// messages.  'mqbnet::Channel' buffers data in high watermark and resumes
// writing when reaching low watermark.  It allows for canceling data by
// checking 'bmqu::AtomicState' when it is provided as an argument to a write
// call.
// To meet the requirement to cancel write request, 'mqbnet::Channel' does not
// build events in high watermark.  Instead, it buffers the request data
// internally.  Event builder is called immediately before writing in low
// watermark.  'mqbnet::Channel' aggregates 4 builders - for each type of data.
// Each builder grows in size as it accumulates data and once it reaches the
// size limit, the content gets written to the channel (flushed) and the
// builder resets.  Builders get forcibly flushed by the 'flush' call.
// Therefore, data can be in one of the following:
//  1. Channel internal buffer waiting for LWM.
//  2. Inside one of 4 builders waiting to reach the size limit or LWM.
//  3. Inside BTE channel internal buffer.
// The 'control' type of messages requires that everything accumulated before
// must be successfully flushed prior to writing the message.  There is no
// 'control' builder.
// Internally, 'mqbnet::Channel' starts a new thread and unconditionally
// buffers everything it needs to write into single consumer queue.  The thread
// reads the channel state which can be
//  - e_RESET,   indicating a need to reset because of a connection change;
//  - e_READY,   low-watermark
//  - e_HWM,     high-watermark
// If the state is 'e_RESET', the thread clear the queue and transitions to
// 'e_INITIAL' . If the channel is set and the state is e_INITIAL, it
// transitions to 'e_READY'.  It the state is not e_READY, the thread waits on
//  condition variable until the state is e_READY.  If the state is 'e_READY',
// the thread pop an Item from the queue and attempts to write.
// Three public methods can change the state:
//  - 'resetChannel'    sets the state to 'e_RESET';
//  - 'setChannel'      sets the state to 'e_RESET' and waits for the internal
//                      thread to transition to 'e_READY'.  This is done to
//                      make sure, any write succeeds after 'setChannel'
//                      returns. The transition 'e_RESET' -> 'e_READY' is
//                      signaled by conditional variable.
//  - 'onWatermark'     sets the state to 'e_READY' on LWM and 'e_HWM' on HWM.

// MQB
#include <mqbi_dispatcher.h>
#include <mqbnet_channelitem.h>

// BMQ
#include <bmqp_ackeventbuilder.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_protocol.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_rejecteventbuilder.h>

#include <bmqc_monitoredqueue.h>
#include <bmqc_monitoredqueue_bdlccsingleconsumerqueue.h>
#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqma_countingallocatorstore.h>
#include <bmqu_atomicstate.h>
#include <bmqu_samethreadchecker.h>

// BDE
#include <ball_log.h>
#include <bdlb_string.h>
#include <bdlbb_blob.h>
#include <bdlcc_singleconsumerqueue.h>
#include <bdlma_concurrentpoolallocator.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace mqbnet {

// =============
// class Channel
// =============

/// Mechanism to handle high watermark events by buffering blobs.
/// This is different from ChannelPool buffer for the following reasons:
///  - timeout can be associated with each item (to expire PUTs)
///  - applying application logic to pending items as in the overload
///    case when different types of item need different treatment
///    (NACKing excessive PUTs).
class Channel {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBNET.CHANNEL");

  private:
    // PRIVATE TYPES
    typedef bmqc::MonitoredQueue<
        bdlcc::SingleConsumerQueue<bslma::ManagedPtr<ChannelItem> > >
        ItemQueue;

    typedef bmqp::BlobPoolUtil::BlobSpPoolSp BlobSpPoolSp;

    /// Builder holding a single pre-built event blob.  There is no builder
    /// for such events, so this simulates a one-time one over the item it
    /// refers to.  It must not outlive that item.  Used for control, cluster
    /// state, storage, elector and receipt events.
    struct BlobBuilder {
        const ChannelBlobItem& d_item;
        int                    d_messageCount;

        explicit BlobBuilder(const ChannelBlobItem& item);

        ~BlobBuilder();

        // MANIPULATORS
        void reset();

        // ACCESSORS
        size_t             eventSize() const;
        size_t             messageCount() const;
        const bdlbb::Blob& blob() const;
    };

    enum Mode {
        /// Call 'popFront' on the buffer.
        /// This puts internal thread into sleep until there are any events.
        e_BLOCK,

        /// Call 'tryPopFront' on the buffer until this buffer is exhausted,
        /// and transition to the next state.
        e_FLUSH_BUFFER,

        /// Flush all builders to IO, and circle back to `e_BLOCK` state.
        e_IDLE
    };

    struct Stats {
        /// Number of `d_numItems` counters; one per `bmqp::EventType::Enum`
        /// value, indexed by that value.
        static const int k_MAX_ITEM_TYPE =
            bmqp::EventType::k_HIGHEST_SUPPORTED_EVENT_TYPE + 1;

        bsls::AtomicUint d_numItems[k_MAX_ITEM_TYPE];

        bsls::AtomicUint d_numItemsTotal;

        bsls::AtomicUint64 d_numBytes;

        // CLASS METHODS

        /// @brief Return the number of bytes the specified `item` accounts
        /// for.
        ///
        /// @param item The item to measure.
        ///
        /// @return The size of the item.
        static size_t getItemSize(const ChannelItem& item);

        // MANIPULATORS

        /// @brief Account for the specified `item` being buffered.
        ///
        /// @param item The item added to the buffer.
        void onAddItem(const ChannelItem& item);

        /// @brief Stop accounting for the specified `item`.
        ///
        /// @param item The item removed from the buffer.
        void onRemoveItem(const ChannelItem& item);

        Stats();

        void reset();
    };

  public:
    // PUBLIC TYPES
    enum EnumState {
        /// Need resetting because of a connection change
        e_RESET = 0,
        /// Between 'Channel::close; and 'resetChannel'
        e_CLOSE = 1,
        e_READY = 2,
        /// LWM
        e_LWM = 3,
        /// HWM
        e_HWM = 4
    };

  private:
    // CONSTANTS
    static const int k_NAGLE_PACKET_SIZE = 1024 * 1024;  // 1MB;

    // DATA
    /// Allocator store to spawn new allocators for sub-components
    bmqma::CountingAllocatorStore d_allocators;

    /// Counting allocator
    bslma::Allocator* d_allocator_p;

    BlobSpPoolSp d_blobSpPool_sp;

    bmqp::PutEventBuilder d_putBuilder;

    bmqp::PushEventBuilder d_pushBuilder;

    bmqp::AckEventBuilder d_ackBuilder;

    bmqp::ConfirmEventBuilder d_confirmBuilder;

    bmqp::RejectEventBuilder d_rejectBuilder;

    /// Pools of items, one per item type.
    bdlma::ConcurrentPoolAllocator d_putItemPool;
    bdlma::ConcurrentPoolAllocator d_explicitPayloadPushItemPool;
    bdlma::ConcurrentPoolAllocator d_implicitPayloadPushItemPool;
    bdlma::ConcurrentPoolAllocator d_ackItemPool;
    bdlma::ConcurrentPoolAllocator d_confirmItemPool;
    bdlma::ConcurrentPoolAllocator d_rejectItemPool;
    bdlma::ConcurrentPoolAllocator d_blobItemPool;

    /// Item enqueued to wake up the writing thread.  It carries no data, so
    /// the same one serves every wake up.
    ChannelWakeUpItem d_wakeUpItem;

    ItemQueue d_buffer;

    bslmt::ThreadUtil::Handle d_threadHandle;

    bslmt::Condition d_stateCondition;

    mutable bslmt::Mutex d_mutex;

    bsls::AtomicBool d_isStopped;

    bsls::AtomicInt d_state;

    bdlcc::SingleConsumerQueue<bmqc::MonitoredQueueState::Enum> d_queueStates;
    // 'ItemQueue' threshold events get
    // processed in the 'threadFn' (instead of
    // 'onBufferStateChange'.  This container
    // keeps events to be processed.

    bsl::weak_ptr<bmqio::Channel> d_channel_wp;
    // Channel associated to this node,
    // if any

    bsl::string d_description;
    // URL of the channel when set

    const bsl::string d_name;
    // Name of the cluster node which owns this
    // channel.

    bmqu::SameThreadChecker d_internalThreadChecker;
    // Mechanism to check if a method is called
    // in the internal thread.
    Stats d_stats;

    /// Indicates graceful shutdown.  Drain the buffer if possible and then
    /// close the channel.
    bsls::AtomicBool d_isStopping;

  private:
    // NOT IMPLEMENTED
    Channel(const Channel&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    Channel& operator=(const Channel&) BSLS_CPP11_DELETED;

    /// Reset active buffer and builders.  This must be called from the
    /// writing thread only.
    void reset();

    /// Flush the specified `builder` to the specified `channel`.  Return
    /// result category.
    template <class Builder>
    bmqio::StatusCategory::Enum
    flushBuilder(Builder&                               builder,
                 const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Flush all internal builders to the specified `channel`.  Return
    /// result category.
    bmqio::StatusCategory::Enum
    flushAll(const bsl::shared_ptr<bmqio::Channel>& channel);

    /// Enqueue the specified `item`.
    bmqt::GenericResult::Enum enqueue(bslma::ManagedPtr<ChannelItem>& item);

    /// Pack the specified `item` using builder corresponding to the `item`
    /// type.  Flush the builder if necessary.  Return result and load
    /// boolean value into the specified `isConsumed` indicating if the data
    /// has ended up in the  builder / got written to the channel or not.
    bmqt::GenericResult::Enum
    writeBufferedItem(bool*                                  isConsumed,
                      const bsl::shared_ptr<bmqio::Channel>& channel,
                      const bsl::string&                     description,
                      const ChannelItem&                     item);

    /// Pack the specified `args` using the specified `builder` and flush
    /// the `builder` to the specified `channel` if necessary.  Update the
    /// specified `state`.  Return result and load boolean value into the
    /// specified `isConsumed` indicating if the data has ended up in the
    /// builder / got written to the channel or not.
    template <typename Builder, typename Args>
    bmqt::GenericResult::Enum
    writeImmediate(bool*                                     isConsumed,
                   const bsl::shared_ptr<bmqio::Channel>&    channel,
                   const bsl::string&                        description,
                   Builder&                                  builder,
                   const Args&                               args,
                   const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// Overloads to pack events data using corresponding builder.
    bmqt::EventBuilderResult::Enum pack(bmqp::PutEventBuilder& builder,
                                        const ChannelPutItem&  item);
    bmqt::EventBuilderResult::Enum
    pack(bmqp::PushEventBuilder&               builder,
         const ChannelExplicitPayloadPushItem& item);
    bmqt::EventBuilderResult::Enum
                                   pack(bmqp::PushEventBuilder&               builder,
                                        const ChannelImplicitPayloadPushItem& item);
    bmqt::EventBuilderResult::Enum pack(bmqp::AckEventBuilder& builder,
                                        const ChannelAckItem&  item);
    bmqt::EventBuilderResult::Enum pack(bmqp::ConfirmEventBuilder& builder,
                                        const ChannelConfirmItem&  item);
    bmqt::EventBuilderResult::Enum pack(bmqp::RejectEventBuilder& builder,
                                        const ChannelRejectItem&  item);
    bmqt::EventBuilderResult::Enum pack(BlobBuilder&           builder,
                                        const ChannelBlobItem& item);

    /// Dedicated thread does all writing.
    void threadFn();

    /// Callback invoked within the d_buffer when the state of the queue
    /// changes. Currently logs the state of the buffer.
    void onBufferStateChange(bmqc::MonitoredQueueState::Enum state);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Channel, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `allocator`.
    Channel(bdlbb::BlobBufferFactory* blobBufferFactory,
            const bsl::string&        name,
            bslma::Allocator*         allocator);

    ~Channel();

    // MANIPULATORS

    /// Set the channel associated to this node to the specified `value`.
    void setChannel(const bsl::weak_ptr<bmqio::Channel>& value);

    /// Reset the channel associated to this node.  The specified
    /// `closedChannel` identifies the channel being closed.  Return `false` if
    /// the node already has a different (newer) channel and ignore the reset.
    /// Return `true` otherwise.
    bool resetChannel(const bsl::shared_ptr<bmqio::Channel>& closedChannel);

    /// Start draining the channel associated to this node, if any.
    void requestToStop();

    /// Stop the channel thread once all draining is done.
    void stop();

    /// Write PUT message using the specified `ph`, `data`, and `state`.
    /// Return e_SUCCESS even if the channel is in HWM.
    bmqt::GenericResult::Enum
    writePut(const bmqp::PutHeader&                    ph,
             const bsl::shared_ptr<bdlbb::Blob>&       data,
             const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// Write `explicit` PUSH message using the specified `payload`,
    /// `queueId`, `msgId`, `flags`, `compressionType`, `subQueueInfos`, and
    /// `state`.  Return e_SUCCESS even if the channel is in HWM.
    bmqt::GenericResult::Enum
    writePush(const bsl::shared_ptr<bdlbb::Blob>&       payload,
              int                                       queueId,
              const bmqt::MessageGUID&                  msgId,
              int                                       flags,
              bmqt::CompressionAlgorithmType::Enum      compressionType,
              const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
              const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
              const bsl::shared_ptr<bmqu::AtomicState>& state =
                  bsl::shared_ptr<bmqu::AtomicState>());

    /// Write `implicit` PUSH message using the specified `subQueueInfos`,
    /// `queueId`, `msgId`, `flags`, `compressionType`, and `state`.  Return
    /// e_SUCCESS even if the channel is in HWM.
    bmqt::GenericResult::Enum
    writePush(int                                       queueId,
              const bmqt::MessageGUID&                  msgId,
              int                                       flags,
              bmqt::CompressionAlgorithmType::Enum      compressionType,
              const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
              const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
              const bsl::shared_ptr<bmqu::AtomicState>& state =
                  bsl::shared_ptr<bmqu::AtomicState>());

    /// Write ACK message using the specified `status`, `correlationId`,
    /// `guid`, `queueId`, and `state`.  Return e_SUCCESS even if the
    /// channel is in HWM.
    bmqt::GenericResult::Enum
    writeAck(int                                       status,
             int                                       correlationId,
             const bmqt::MessageGUID&                  guid,
             int                                       queueId,
             const bsl::shared_ptr<bmqu::AtomicState>& state =
                 bsl::shared_ptr<bmqu::AtomicState>());

    /// Write CONFIRM message using the specified `queueId`, `subQueueId`,
    /// `guid`, and `state`.  Return e_SUCCESS even if the channel is in
    /// HWM.
    bmqt::GenericResult::Enum
    writeConfirm(int                                       queueId,
                 int                                       subQueueId,
                 const bmqt::MessageGUID&                  guid,
                 const bsl::shared_ptr<bmqu::AtomicState>& state =
                     bsl::shared_ptr<bmqu::AtomicState>());

    bmqt::GenericResult::Enum
    writeReject(int                                       queueId,
                int                                       subQueueId,
                const bmqt::MessageGUID&                  guid,
                const bsl::shared_ptr<bmqu::AtomicState>& state =
                    bsl::shared_ptr<bmqu::AtomicState>());

    /// Send the specified `data` using the specified `state`.  The
    /// specified `type` controls whether to flush everything accumulated
    /// prior to sending (the default behavior) and whether to keep the
    /// `data` and `state` until `Flush` is called at which point send
    /// everything accumulated before the `Flush` call (as in the case of
    /// Replication Receipt).  Return e_SUCCESS even if the channel is in
    /// High WaterMark.
    bmqt::GenericResult::Enum
    writeBlob(const bsl::shared_ptr<bdlbb::Blob>&       data,
              bmqp::EventType::Enum                     type,
              const bsl::shared_ptr<bmqu::AtomicState>& state = 0);

    /// Wake up the internal thread if it's waiting for a new item to write.
    /// This leads to flushing of the inner message builders.
    void wakeUp();

    /// Notify the channel when a watermark of the specified `type` is being
    /// reached.
    void onWatermark(bmqio::ChannelWatermarkType::Enum type);

    // ACCESSORS
    bool isAvailable() const;

    /// Return associated bmqio channel or empty shared_ptr.
    const bsl::shared_ptr<bmqio::Channel> channel() const;

    unsigned int numItems() const;

    unsigned int numItems(bmqp::EventType::Enum type) const;

    bsls::Types::Uint64 numBytes() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class Channel::BlobBuilder
// --------------------------

inline Channel::BlobBuilder::BlobBuilder(const ChannelBlobItem& item)
: d_item(item)
, d_messageCount(1)
{
    // NOTHING
}

inline Channel::BlobBuilder::~BlobBuilder()
{
    // NOTHING
}

inline const bdlbb::Blob& Channel::BlobBuilder::blob() const
{
    return *d_item.data();
}

inline size_t Channel::BlobBuilder::messageCount() const
{
    return d_messageCount;
}

inline void Channel::BlobBuilder::reset()
{
    d_messageCount = 0;
}

// --------------------
// class Channel::Stats
// --------------------

inline Channel::Stats::Stats()
{
    reset();
}

inline size_t Channel::Stats::getItemSize(const ChannelItem& item)
{
    switch (item.type()) {
    case ChannelItemType::e_PUT: {
        const ChannelPutItem* put = item.the<ChannelPutItem>();

        return sizeof(bmqp::PutHeader) + put->data()->length();  // RETURN
    }
    case ChannelItemType::e_EXPLICIT_PAYLOAD_PUSH: {
        const ChannelExplicitPayloadPushItem* push =
            item.the<ChannelExplicitPayloadPushItem>();

        return sizeof(bmqp::PushHeader) + push->data()->length();  // RETURN
    }
    case ChannelItemType::e_IMPLICIT_PAYLOAD_PUSH: {
        const ChannelImplicitPayloadPushItem* push =
            item.the<ChannelImplicitPayloadPushItem>();

        return sizeof(bmqp::PushHeader) +
               push->subQueueInfos().size() *
                   bmqp::Protocol::k_WORD_SIZE;  // RETURN
    }
    case ChannelItemType::e_ACK: {
        return sizeof(bmqp::AckMessage);  // RETURN
    }
    case ChannelItemType::e_CONFIRM: {
        return sizeof(bmqp::ConfirmMessage);  // RETURN
    }
    case ChannelItemType::e_REJECT: {
        return sizeof(bmqp::RejectMessage);  // RETURN
    }
    case ChannelItemType::e_BLOB: {
        return item.the<ChannelBlobItem>()->data()->length();  // RETURN
    }
    case ChannelItemType::e_WAKE_UP:
    default: {
        return 0;  // RETURN
    }
    }
}

inline void Channel::Stats::onAddItem(const ChannelItem& item)
{
    ++d_numItemsTotal;
    d_numBytes += getItemSize(item);

    ++d_numItems[item.eventType()];
}

inline void Channel::Stats::onRemoveItem(const ChannelItem& item)
{
    const size_t size = getItemSize(item);

    BSLS_ASSERT_SAFE(d_numItemsTotal > 0);
    --d_numItemsTotal;
    BSLS_ASSERT_SAFE(d_numBytes >= size);
    d_numBytes -= size;
    --d_numItems[item.eventType()];
}

// -------------
// class Channel
// -------------

template <>
inline bmqio::StatusCategory::Enum Channel::flushBuilder<Channel::BlobBuilder>(
    BlobBuilder&                           builder,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    bmqio::StatusCategory::Enum rc = bmqio::StatusCategory::e_SUCCESS;

    const bmqp::EventType::Enum type = builder.d_item.eventType();

    if (type == bmqp::EventType::e_CONTROL ||
        type == bmqp::EventType::e_CLUSTER_STATE) {
        // 'Control' event requires that everything accumulated so far is
        // flushed first.
        rc = flushAll(channel);
    }
    else if (type == bmqp::EventType::e_STORAGE) {
        // Flush all (previously) accumulated PUSH data.  Otherwise, PUSH data
        // lag behind replication data.
        rc = flushBuilder(d_pushBuilder, channel);
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqio::StatusCategory::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc;  // RETURN
    }

    bmqio::Status st;

    BSLS_ASSERT_SAFE(builder.messageCount());

    channel->write(&st, builder.blob());

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            st.category() == bmqio::StatusCategory::e_SUCCESS)) {
        builder.reset();
    }

    return st.category();
}

template <class Builder>
inline bmqio::StatusCategory::Enum
Channel::flushBuilder(Builder&                               builder,
                      const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // This is the last 'write' method in the call hierarchy before actually
    // calling bmqio.

    bmqio::Status st;

    if (builder.messageCount()) {
        channel->write(&st, *builder.blob());

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
                st.category() == bmqio::StatusCategory::e_SUCCESS)) {
            builder.reset();
        }
    }
    return st.category();
}

inline bmqt::GenericResult::Enum
Channel::enqueue(bslma::ManagedPtr<ChannelItem>& item)
{
    // This method is called by public write methods.

    if (!isAvailable()) {
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    d_stats.onAddItem(*item);

    d_buffer.pushBack(bslmf::MovableRefUtil::move(item));

    return bmqt::GenericResult::e_SUCCESS;
}

// ACCESSORS
inline bool Channel::isAvailable() const
{
    return d_state != e_RESET && d_state != e_CLOSE && !d_isStopping;
}

inline const bsl::shared_ptr<bmqio::Channel> Channel::channel() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
    return d_channel_wp.lock();
}

inline unsigned int Channel::numItems() const
{
    return d_stats.d_numItemsTotal;
}

inline unsigned int Channel::numItems(bmqp::EventType::Enum type) const
{
    return d_stats.d_numItems[type];
}

inline bsls::Types::Uint64 Channel::numBytes() const
{
    return d_stats.d_numBytes;
}

}  // close package namespace
}  // close enterprise namespace

#endif
