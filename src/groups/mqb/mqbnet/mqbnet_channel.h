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

// mqbnet_channel.h                                                   -*-C++-*-
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
//  - e_INITIAL, not connected
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
#include <bdlma_concurrentpool.h>
#include <bsl_deque.h>
#include <bslma_allocator.h>
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
    struct Item;

    typedef bmqc::MonitoredQueue<
        bdlcc::SingleConsumerQueue<bslma::ManagedPtr<Item> > >
        ItemQueue;

    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    /// Const references to everything needed to writeBufferedItem PUT.
    /// This is template parameter to generalized writing code.
    struct PutArgs {
        const bmqp::PutHeader&              d_putHeader;
        const bsl::shared_ptr<bdlbb::Blob>& d_data_sp;

        PutArgs(Item& item);

        ~PutArgs();
    };

    /// Const references to generic PUSH data.
    struct PushArgsBase {
        const int                                  d_queueId;
        const bmqt::MessageGUID&                   d_msgId;
        const int                                  d_flags;
        const bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
        const bmqp::MessagePropertiesInfo          d_messagePropertiesInfo;

        PushArgsBase(
            int                                  queueId,
            const bmqt::MessageGUID&             msgId,
            int                                  flags,
            bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
            const bmqp::MessagePropertiesInfo&   messagePropertiesInfo);

        ~PushArgsBase();
    };

    /// Const references to everything needed to writeBufferedItem PUSH with
    /// data.
    /// This is template parameter to generalized writing code.
    struct ExplicitPushArgs : PushArgsBase {
        const bsl::shared_ptr<bdlbb::Blob>& d_data_sp;

        const bmqp::Protocol::SubQueueInfosArray& d_subQueueInfos;

        ExplicitPushArgs(Item& item);

        ~ExplicitPushArgs();
    };

    /// Const references to everything needed to writeBufferedItem PUSH
    /// without data.
    /// This is template parameter to generalized writing code.
    struct ImplicitPushArgs : PushArgsBase {
        const bmqp::Protocol::SubQueueInfosArray& d_subQueueInfos;

        ImplicitPushArgs(Item& item);

        ~ImplicitPushArgs();
    };
    /// Const references to everything needed to writeBufferedItem ACK.
    /// This is template parameter to generalized writing code.
    struct AckArgs {
        const int                d_status;
        const int                d_correlationId;
        const bmqt::MessageGUID& d_guid;
        const int                d_queueId;

        AckArgs(Item& item);

        ~AckArgs();
    };

    /// Const references to everything needed to writeBufferedItem CONFIRM.
    /// This is template parameter to generalized writing code.
    struct ConfirmArgs {
        const int                d_queueId;
        const int                d_subQueueId;
        const bmqt::MessageGUID& d_guid;

        ConfirmArgs(Item& item);

        ~ConfirmArgs();
    };

    /// Const references to everything needed to writeBufferedItem REJECT.
    /// This is template parameter to generalized writing code.
    struct RejectArgs {
        const int                d_queueId;
        const int                d_subQueueId;
        const bmqt::MessageGUID& d_guid;

        RejectArgs(Item& item);

        ~RejectArgs();
    };

    /// Const references to everything needed to writeBufferedItem
    /// bdlbb::Blob.
    /// This is template parameter to generalized writing code.
    /// This struct also simulates a (one-time) Builder since there is no
    /// `control` builder.
    struct ControlArgs {
        bmqp::EventType::Enum d_type;
        const bsl::shared_ptr<bdlbb::Blob>& d_data_sp;
        int                   d_messageCount;

        ControlArgs(Item& item);

        ~ControlArgs();

        // MANIPULATORS
        void reset();

        // ACCESSORS
        size_t             eventSize() const;
        size_t             messageCount() const;
        const bdlbb::Blob& blob() const;
    };

    /// VST representing event data buffered (due to high watermark).
    struct Item {
        // discriminator
        bmqp::EventType::Enum d_type;

        // union of event data copies
        const bmqp::PutHeader                      d_putHeader;
        const bsl::shared_ptr<bdlbb::Blob>         d_data_sp;
        const int                                  d_queueId;
        const int                                  d_subQueueId;
        const bmqt::MessageGUID                    d_msgId;
        const int                                  d_flags;
        const bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
        const bmqp::MessagePropertiesInfo          d_messagePropertiesInfo;
        const bmqp::Protocol::SubQueueInfosArray   d_subQueueInfos;
        const int                                  d_correlationId;
        const int                                  d_status;

        const bsl::shared_ptr<bmqu::AtomicState> d_state;

        size_t d_numBytes;

        Item(bslma::Allocator* allocator);

        Item(const bmqp::PutHeader&                    ph,
             const bsl::shared_ptr<bdlbb::Blob>&       data,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bslma::Allocator*                         allocator);

        Item(int                                  queueId,
             const bmqt::MessageGUID&             msgId,
             int                                  flags,
             bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
             const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
             const bsl::shared_ptr<bdlbb::Blob>&  payload,
             const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bslma::Allocator*                         allocator);

        Item(int                                  queueId,
             const bmqt::MessageGUID&             msgId,
             int                                  flags,
             bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
             const bmqp::MessagePropertiesInfo&   messagePropertiesInfo,
             const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bslma::Allocator*                         allocator);

        Item(int                                       status,
             int                                       correlationId,
             const bmqt::MessageGUID&                  msgId,
             int                                       queueId,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bslma::Allocator*                         allocator);

        Item(int                                       queueId,
             int                                       subQueueId,
             const bmqt::MessageGUID&                  guid,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bmqp::EventType::Enum                     type,
             bslma::Allocator*                         allocator);

        Item(const bsl::shared_ptr<bdlbb::Blob>&       data,
             bmqp::EventType::Enum                     type,
             const bsl::shared_ptr<bmqu::AtomicState>& state,
             bslma::Allocator*                         allocator);

        ~Item();

        const bsl::shared_ptr<bdlbb::Blob>& data();

        bool isSecondary();
    };

    enum Mode {
        e_BLOCK,     // call 'popFront' on the primary buffer
        e_PRIMARY,   // call 'tryPopFront' on the primary buffer, if it is
                     // empty, then transition to the next state
        e_IDLE,      // flush all builders, and transition to the next state
        e_SECONDARY  // call 'tryPopFront' on the secondary buffer until it
                     // gets empty, then transition 'to e_BLOCK' state

    };

    struct Stats {
        static const int k_MAX_ITEM_TYPE =
            bmqp::EventType::e_REPLICATION_RECEIPT + 1;

        bsls::AtomicUint d_numItems[k_MAX_ITEM_TYPE];

        bsls::AtomicUint d_numItemsTotal;

        bsls::AtomicUint64 d_numBytes;

        void addItem(bmqp::EventType::Enum, size_t size);

        void removeItem(bmqp::EventType::Enum, size_t size);

        Stats();

        void reset();
    };

  public:
    // PUBLIC TYPES
    enum EnumState {
        e_INITIAL = 0  // Not connected
        ,
        e_RESET = 1  // Need resetting because of a connection change
        ,
        e_CLOSE = 2  // Between 'Channel::close; and 'resetChannel'
        ,
        e_READY = 3,
        e_LWM   = 4  // LWM
        ,
        e_HWM = 5  // HWM
    };

  private:
    // CONSTANTS
    static const int k_NAGLE_PACKET_SIZE = 1024 * 1024;  // 1MB;

    // DATA
    /// Allocator store to spawn new allocators for sub-components
    bmqma::CountingAllocatorStore d_allocators;

    /// Counting allocator
    bslma::Allocator* d_allocator_p;

    BlobSpPool d_blobSpPool;

    bmqp::PutEventBuilder d_putBuilder;

    bmqp::PushEventBuilder d_pushBuilder;

    bmqp::AckEventBuilder d_ackBuilder;

    bmqp::ConfirmEventBuilder d_confirmBuilder;

    bmqp::RejectEventBuilder d_rejectBuilder;

    bdlma::ConcurrentPool d_itemPool;
    // Pool of 'Item' objects.

    ItemQueue d_buffer;
    ItemQueue d_secondaryBuffer;

    bslmt::ThreadUtil::Handle d_threadHandle;

    bslmt::Condition d_stateCondition;

    mutable bslmt::Mutex d_mutex;

    bsls::AtomicBool d_doStop;

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
    bmqt::GenericResult::Enum enqueue(bslma::ManagedPtr<Item>& item);

    /// Pack the specified `item` using builder corresponding to the `item`
    /// type.  Flush the builder if necessary.  Return result and load
    /// boolean value into the specified `isConsumed` indicating if the data
    /// has ended up in the  builder / got written to the channel or not.
    bmqt::GenericResult::Enum
    writeBufferedItem(bool*                                  isConsumed,
                      const bsl::shared_ptr<bmqio::Channel>& channel,
                      const bsl::string&                     description,
                      Item&                                  item);

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
                                        const PutArgs&         args);
    bmqt::EventBuilderResult::Enum pack(bmqp::PushEventBuilder& builder,
                                        const ExplicitPushArgs& args);
    bmqt::EventBuilderResult::Enum pack(bmqp::PushEventBuilder& builder,
                                        const ImplicitPushArgs& args);
    bmqt::EventBuilderResult::Enum pack(bmqp::AckEventBuilder& builder,
                                        const AckArgs&         args);
    bmqt::EventBuilderResult::Enum pack(bmqp::ConfirmEventBuilder& builder,
                                        const ConfirmArgs&         args);
    bmqt::EventBuilderResult::Enum pack(bmqp::RejectEventBuilder& builder,
                                        const RejectArgs&         args);
    bmqt::EventBuilderResult::Enum pack(ControlArgs&       builder,
                                        const ControlArgs& args);

    /// Dedicated thread does all writing.
    void threadFn();

    /// Callback invoked within the d_buffer when the state of the queue
    /// changes. Currently logs the state of the buffer.
    void onBufferStateChange(bmqc::MonitoredQueueState::Enum state);

  private:
    // PRIVATE CLASS METHODS
    static void deleteItem(void* item, void* cookie);

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

    /// Reset the channel associated to this node.
    void resetChannel();

    /// Close the channel associated to this node, if any.
    void closeChannel();

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

    /// Write everything unless there is a thread actively writing already.
    void flush();

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

// -----------------------
// struct Channel::PutArgs
// -----------------------

inline Channel::PutArgs::~PutArgs()
{
    // NOTHING
}

inline Channel::PutArgs::PutArgs(Item& item)
: d_putHeader(item.d_putHeader)
, d_data_sp(item.d_data_sp)
{
    // NOTHING
}

// ------------------------
// struct Channel::PushArgs
// ------------------------

inline Channel::PushArgsBase::PushArgsBase(
    int                                  queueId,
    const bmqt::MessageGUID&             msgId,
    int                                  flags,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    const bmqp::MessagePropertiesInfo&   messagePropertiesInfo)
: d_queueId(queueId)
, d_msgId(msgId)
, d_flags(flags)
, d_compressionAlgorithmType(compressionAlgorithmType)
, d_messagePropertiesInfo(messagePropertiesInfo)
{
    // NOTHING
}

inline Channel::PushArgsBase::~PushArgsBase()
{
    // NOTHING
}

// --------------------------------
// struct Channel::ExplicitPushArgs
// --------------------------------

inline Channel::ExplicitPushArgs::ExplicitPushArgs(Item& item)
: PushArgsBase(item.d_queueId,
               item.d_msgId,
               item.d_flags,
               item.d_compressionAlgorithmType,
               item.d_messagePropertiesInfo)
, d_data_sp(item.d_data_sp)
, d_subQueueInfos(item.d_subQueueInfos)
{
    // NOTHING
}

inline Channel::ExplicitPushArgs::~ExplicitPushArgs()
{
    // NOTHING
}
// --------------------------------
// struct Channel::ImplicitPushArgs
// --------------------------------

inline Channel::ImplicitPushArgs::ImplicitPushArgs(Item& item)
: PushArgsBase(item.d_queueId,
               item.d_msgId,
               item.d_flags,
               item.d_compressionAlgorithmType,
               item.d_messagePropertiesInfo)
, d_subQueueInfos(item.d_subQueueInfos)
{
    // NOTHING
}

inline Channel::ImplicitPushArgs::~ImplicitPushArgs()
{
    // NOTHING
}

// --------------------------
// class Channel::AckArgs
// --------------------------

inline Channel::AckArgs::AckArgs(Item& item)
: d_status(item.d_status)
, d_correlationId(item.d_correlationId)
, d_guid(item.d_msgId)
, d_queueId(item.d_queueId)
{
    // NOTHING
}

inline Channel::AckArgs::~AckArgs()
{
    // NOTHING
}

// --------------------------
// class Channel::ConfirmArgs
// --------------------------

inline Channel::ConfirmArgs::ConfirmArgs(Item& item)
: d_queueId(item.d_queueId)
, d_subQueueId(item.d_subQueueId)
, d_guid(item.d_msgId)
{
    // NOTHING
}

inline Channel::ConfirmArgs::~ConfirmArgs()
{
    // NOTHING
}

// -------------------------
// class Channel::RejectArgs
// -------------------------

inline Channel::RejectArgs::RejectArgs(Item& item)
: d_queueId(item.d_queueId)
, d_subQueueId(item.d_subQueueId)
, d_guid(item.d_msgId)
{
    // NOTHING
}

inline Channel::RejectArgs::~RejectArgs()
{
    // NOTHING
}

// --------------------------
// class Channel::ControlArgs
// --------------------------

inline Channel::ControlArgs::ControlArgs(Item& item)
: d_type(item.d_type)
, d_data_sp(item.d_data_sp)
, d_messageCount(1)
{
    // NOTHING
}

inline Channel::ControlArgs::~ControlArgs()
{
    // NOTHING
}

inline const bdlbb::Blob& Channel::ControlArgs::blob() const
{
    return *d_data_sp;
}

inline size_t Channel::ControlArgs::messageCount() const
{
    return d_messageCount;
}

inline void Channel::ControlArgs::reset()
{
    d_messageCount = 0;
}

// -------------------
// class Channel::Item
// -------------------

// CREATORS
inline Channel::Item::Item(bslma::Allocator* allocator)
: d_type(bmqp::EventType::e_UNDEFINED)
, d_putHeader()
, d_data_sp(0, allocator)
, d_queueId(0)
, d_subQueueId(0)
, d_msgId()
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_subQueueInfos(allocator)
, d_correlationId(0)
, d_status(0)
, d_state(0)
, d_numBytes(0)
{
    // NOTHING
}

inline Channel::Item::Item(const bmqp::PutHeader&                    ph,
                           const bsl::shared_ptr<bdlbb::Blob>&       data,
                           const bsl::shared_ptr<bmqu::AtomicState>& state,
                           bslma::Allocator*                         allocator)
: d_type(bmqp::EventType::e_PUT)
, d_putHeader(ph)
, d_data_sp(data)
, d_queueId(0)
, d_subQueueId(0)
, d_msgId()
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_messagePropertiesInfo()
, d_subQueueInfos(allocator)
, d_correlationId(0)
, d_status(0)
, d_state(state)
, d_numBytes(sizeof(bmqp::PutHeader) + data->length())
{
    // NOTHING
}

inline Channel::Item::Item(
    int                                       queueId,
    const bmqt::MessageGUID&                  msgId,
    int                                       flags,
    bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
    const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
    const bsl::shared_ptr<bdlbb::Blob>&       payload,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bsl::shared_ptr<bmqu::AtomicState>& state,
    bslma::Allocator*                         allocator)
: d_type(bmqp::EventType::e_PUSH)
, d_putHeader()
, d_data_sp(payload)
, d_queueId(queueId)
, d_subQueueId(0)
, d_msgId(msgId)
, d_flags(flags)
, d_compressionAlgorithmType(compressionAlgorithmType)
, d_messagePropertiesInfo(messagePropertiesInfo)
, d_subQueueInfos(subQueueInfos, allocator)
, d_correlationId(0)
, d_status(0)
, d_state(state)
, d_numBytes(sizeof(bmqp::PushHeader) + payload->length())
{
    // NOTHING
}

inline Channel::Item::Item(
    int                                       queueId,
    const bmqt::MessageGUID&                  msgId,
    int                                       flags,
    bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
    const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bsl::shared_ptr<bmqu::AtomicState>& state,
    bslma::Allocator*                         allocator)
: d_type(bmqp::EventType::e_PUSH)
, d_putHeader()
, d_data_sp(0, allocator)
, d_queueId(queueId)
, d_subQueueId(0)
, d_msgId(msgId)
, d_flags(flags)
, d_compressionAlgorithmType(compressionAlgorithmType)
, d_messagePropertiesInfo(messagePropertiesInfo)
, d_subQueueInfos(subQueueInfos, allocator)
, d_correlationId(0)
, d_status(0)
, d_state(state)
, d_numBytes(sizeof(bmqp::PushHeader) +
             subQueueInfos.size() * bmqp::Protocol::k_WORD_SIZE)
{
    // NOTHING
}

inline Channel::Item::Item(int                      status,
                           int                      correlationId,
                           const bmqt::MessageGUID& guid,
                           int                      queueId,
                           const bsl::shared_ptr<bmqu::AtomicState>& state,
                           bslma::Allocator*                         allocator)
: d_type(bmqp::EventType::e_ACK)
, d_putHeader()
, d_data_sp(0, allocator)
, d_queueId(queueId)
, d_subQueueId(0)
, d_msgId(guid)
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_messagePropertiesInfo()
, d_subQueueInfos(allocator)
, d_correlationId(correlationId)
, d_status(status)
, d_state(state)
, d_numBytes(sizeof(bmqp::AckMessage))
{
    // NOTHING
}

inline Channel::Item::Item(int                      queueId,
                           int                      subQueueId,
                           const bmqt::MessageGUID& guid,
                           const bsl::shared_ptr<bmqu::AtomicState>& state,
                           bmqp::EventType::Enum                     type,
                           bslma::Allocator*                         allocator)
: d_type(type)
, d_putHeader()
, d_data_sp(0, allocator)
, d_queueId(queueId)
, d_subQueueId(subQueueId)
, d_msgId(guid)
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_messagePropertiesInfo()
, d_subQueueInfos(allocator)
, d_correlationId(0)
, d_status(0)
, d_state(state)
, d_numBytes(0)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(type ==
                                            bmqp::EventType::e_CONFIRM)) {
        d_numBytes = sizeof(bmqp::ConfirmMessage);
    }
    else if (type == bmqp::EventType::e_REJECT) {
        d_numBytes = sizeof(bmqp::RejectMessage);
    }
    else {
        BSLS_ASSERT_SAFE(false && "Unsupported EventType.");
    }
}

inline Channel::Item::Item(const bsl::shared_ptr<bdlbb::Blob>&       data,
                           bmqp::EventType::Enum                     type,
                           const bsl::shared_ptr<bmqu::AtomicState>& state,
                           bslma::Allocator*                         allocator)
: d_type(type)
, d_putHeader()
, d_data_sp(data)
, d_queueId(0)
, d_subQueueId(0)
, d_msgId()
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_messagePropertiesInfo()
, d_subQueueInfos(allocator)
, d_correlationId(0)
, d_status(0)
, d_state(state)
, d_numBytes(data->length())
{
    // NOTHING
}

inline Channel::Item::~Item()
{
    // NOTHING
}

inline const bsl::shared_ptr<bdlbb::Blob>& Channel::Item::data()
{
    return d_data_sp;
}

inline bool Channel::Item::isSecondary()
{
    return d_type == bmqp::EventType::e_ACK;
}

// --------------------
// class Channel::Stats
// --------------------

inline Channel::Stats::Stats()
{
    reset();
}

inline void Channel::Stats::addItem(bmqp::EventType::Enum type, size_t size)
{
    ++d_numItemsTotal;
    d_numBytes += size;

    ++d_numItems[type];
}

inline void Channel::Stats::removeItem(bmqp::EventType::Enum type, size_t size)
{
    BSLS_ASSERT_SAFE(d_numItemsTotal > 0);
    --d_numItemsTotal;
    BSLS_ASSERT_SAFE(d_numBytes >= size);
    d_numBytes -= size;
    --d_numItems[type];
}

// -------------
// class Channel
// -------------

template <>
inline bmqio::StatusCategory::Enum Channel::flushBuilder<Channel::ControlArgs>(
    ControlArgs&                           args,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    bmqio::StatusCategory::Enum rc = bmqio::StatusCategory::e_SUCCESS;

    if (args.d_type == bmqp::EventType::e_CONTROL ||
        args.d_type == bmqp::EventType::e_CLUSTER_STATE) {
        // 'Control' event requires that everything accumulated so far is
        // flushed first.
        rc = flushAll(channel);
    }
    else if (args.d_type == bmqp::EventType::e_STORAGE) {
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

    BSLS_ASSERT_SAFE(args.messageCount());

    channel->write(&st, args.blob());

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            st.category() == bmqio::StatusCategory::e_SUCCESS)) {
        args.reset();
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
        channel->write(&st, builder.blob());

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
                st.category() == bmqio::StatusCategory::e_SUCCESS)) {
            builder.reset();
        }
    }
    return st.category();
}

inline bmqt::GenericResult::Enum
Channel::enqueue(bslma::ManagedPtr<Item>& item)
{
    // This method is called by public write methods.

    if (!isAvailable()) {
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    d_stats.addItem(item->d_type, item->d_numBytes);

    d_buffer.pushBack(bslmf::MovableRefUtil::move(item));

    return bmqt::GenericResult::e_SUCCESS;
}

// ACCESSORS
inline bool Channel::isAvailable() const
{
    return d_state != e_INITIAL && d_state != e_RESET && d_state != e_CLOSE;
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
