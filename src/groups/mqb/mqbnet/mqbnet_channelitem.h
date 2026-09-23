// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_MQBNET_CHANNELITEM
#define INCLUDED_MQBNET_CHANNELITEM

//@PURPOSE: Provide value types for the data buffered by 'mqbnet::Channel'.
//
//@CLASSES:
//  mqbnet::ChannelItemType:                Enumeration of the item types
//  mqbnet::ChannelItem:                    Base class for buffered data
//  mqbnet::ChannelPutItem:                 Buffered PUT message
//  mqbnet::ChannelExplicitPayloadPushItem: Buffered PUSH message with payload
//  mqbnet::ChannelImplicitPayloadPushItem: Buffered PUSH message, no payload
//  mqbnet::ChannelAckItem:                 Buffered ACK message
//  mqbnet::ChannelConfirmItem:             Buffered CONFIRM message
//  mqbnet::ChannelRejectItem:              Buffered REJECT message
//  mqbnet::ChannelBlobItem:                Buffered pre-built event blob
//  mqbnet::ChannelWakeUpItem:              Placeholder carrying no data
//
//@DESCRIPTION: 'mqbnet::Channel' buffers the data it cannot write yet.  Each
// buffered element is a 'mqbnet::ChannelItem'.  The base class holds what
// every element needs - the cancelation state - and each derived class holds
// the fields of one kind of message.
//
// 'mqbnet::ChannelItem' offers:
//: o 'type': the concrete class holding the data
//: o 'eventType': the wire type of the event the data belongs to
//: o 'the': the item as its concrete class, checked against 'type'
//: o 'state': the state used to cancel the item before it is written

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>
#include <bmqu_atomicstate.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mqbnet {

// ======================
// struct ChannelItemType
// ======================

/// Concrete types of `mqbnet::ChannelItem`.
struct ChannelItemType {
    // TYPES
    enum Enum {
        /// `mqbnet::ChannelPutItem`
        e_PUT,

        /// `mqbnet::ChannelExplicitPayloadPushItem`
        e_EXPLICIT_PAYLOAD_PUSH,

        /// `mqbnet::ChannelImplicitPayloadPushItem`
        e_IMPLICIT_PAYLOAD_PUSH,

        /// `mqbnet::ChannelAckItem`
        e_ACK,

        /// `mqbnet::ChannelConfirmItem`
        e_CONFIRM,

        /// `mqbnet::ChannelRejectItem`
        e_REJECT,

        /// `mqbnet::ChannelBlobItem`
        e_BLOB,

        /// `mqbnet::ChannelWakeUpItem`
        e_WAKE_UP
    };
};

// =================
// class ChannelItem
// =================

/// Base class for an element buffered by `mqbnet::Channel`.
class ChannelItem {
  private:
    // DATA

    /// State used to cancel this item before it is written.
    const bsl::shared_ptr<bmqu::AtomicState> d_state_sp;

  private:
    // NOT IMPLEMENTED
    ChannelItem(const ChannelItem&) BSLS_KEYWORD_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    ChannelItem& operator=(const ChannelItem&) BSLS_KEYWORD_DELETED;

  protected:
    // CREATORS

    /// @brief Create an item using the specified `state`.
    ///
    /// @param state State used to cancel this item, may be empty.
    explicit ChannelItem(const bsl::shared_ptr<bmqu::AtomicState>& state);

  public:
    // CREATORS

    /// @brief Destructor.
    virtual ~ChannelItem();

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    virtual ChannelItemType::Enum type() const = 0;

    /// @brief Return the type of the event the data of this item belongs to.
    virtual bmqp::EventType::Enum eventType() const = 0;

    /// @brief Return the state used to cancel this item, if any.
    const bsl::shared_ptr<bmqu::AtomicState>& state() const;

    /// @brief Return this item as the specified `ITEM_TYPE`.
    ///
    /// The behavior is undefined unless this item is of `ITEM_TYPE`.
    ///
    /// @return A pointer to this item.
    template <class ITEM_TYPE>
    const ITEM_TYPE* the() const;
};

// ====================
// class ChannelPutItem
// ====================

/// Buffered PUT message.
class ChannelPutItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_PUT;

  private:
    // DATA

    /// Header of the message.
    const bmqp::PutHeader d_putHeader;

    /// Payload of the message.
    const bsl::shared_ptr<bdlbb::Blob> d_data_sp;

  public:
    // CREATORS

    /// @brief Create an item holding the specified `putHeader` and `data`.
    ///
    /// @param putHeader Header of the message.
    /// @param data      Payload of the message, must not be empty.
    /// @param state     State used to cancel this item, may be empty.
    explicit ChannelPutItem(const bmqp::PutHeader&              putHeader,
                            const bsl::shared_ptr<bdlbb::Blob>& data,
                            const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// @brief Destructor.
    ~ChannelPutItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the header of the message.
    const bmqp::PutHeader& putHeader() const;

    /// @brief Return the payload of the message.
    const bsl::shared_ptr<bdlbb::Blob>& data() const;
};

// ====================================
// class ChannelExplicitPayloadPushItem
// ====================================

/// Buffered PUSH message carrying its payload.  The broker sends the payload
/// when the receiver has no other way to get it, as for a queue in the
/// at-most-once mode where nothing is replicated.
class ChannelExplicitPayloadPushItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE =
        ChannelItemType::e_EXPLICIT_PAYLOAD_PUSH;

  private:
    // DATA

    /// Payload of the message.
    const bsl::shared_ptr<bdlbb::Blob> d_data_sp;

    /// Id of the queue the message belongs to.
    const int d_queueId;

    /// Id of the message.
    const bmqt::MessageGUID d_msgId;

    /// Flags of the message.
    const int d_flags;

    /// Compression of the payload.
    const bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;

    /// Message properties of the message.
    const bmqp::MessagePropertiesInfo d_messagePropertiesInfo;

    /// Subscriptions the message is sent for.
    const bmqp::Protocol::SubQueueInfosArray d_subQueueInfos;

  public:
    // CREATORS

    /// @brief Create an item holding a message with the specified `payload`.
    ///
    /// This type does not declare the `bslma::UsesBslmaAllocator` trait, so
    /// the memory of the item itself and the memory of its fields come from
    /// different allocators.  The item can then be allocated from a pool
    /// holding blocks of the size of this type only.
    ///
    /// @param queueId                  Id of the queue the message belongs to.
    /// @param msgId                    Id of the message.
    /// @param flags                    Flags of the message.
    /// @param compressionAlgorithmType Compression of the payload.
    /// @param messagePropertiesInfo    Message properties of the message.
    /// @param payload                  Payload, must not be empty.
    /// @param subQueueInfos            Subscriptions to send the message for.
    /// @param state                    State used to cancel this item, may be
    ///                                 empty.
    /// @param allocator                Allocator to supply memory.
    explicit ChannelExplicitPayloadPushItem(
        int                                       queueId,
        const bmqt::MessageGUID&                  msgId,
        int                                       flags,
        bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
        const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
        const bsl::shared_ptr<bdlbb::Blob>&       payload,
        const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
        const bsl::shared_ptr<bmqu::AtomicState>& state,
        bslma::Allocator*                         allocator);

    /// @brief Destructor.
    ~ChannelExplicitPayloadPushItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the payload of the message.
    const bsl::shared_ptr<bdlbb::Blob>& data() const;

    /// @brief Return the id of the queue the message belongs to.
    int queueId() const;

    /// @brief Return the id of the message.
    const bmqt::MessageGUID& msgId() const;

    /// @brief Return the flags of the message.
    int flags() const;

    /// @brief Return the compression of the payload.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// @brief Return the message properties of the message.
    const bmqp::MessagePropertiesInfo& messagePropertiesInfo() const;

    /// @brief Return the subscriptions the message is sent for.
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos() const;
};

// ====================================
// class ChannelImplicitPayloadPushItem
// ====================================

/// Buffered PUSH message without payload.  The receiver already holds the
/// data, as after replication, and looks it up by the id of the message.
class ChannelImplicitPayloadPushItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE =
        ChannelItemType::e_IMPLICIT_PAYLOAD_PUSH;

  private:
    // DATA

    /// Id of the queue the message belongs to.
    const int d_queueId;

    /// Id of the message.
    const bmqt::MessageGUID d_msgId;

    /// Flags of the message.
    const int d_flags;

    /// Compression of the payload.
    const bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;

    /// Message properties of the message.
    const bmqp::MessagePropertiesInfo d_messagePropertiesInfo;

    /// Subscriptions the message is sent for.
    const bmqp::Protocol::SubQueueInfosArray d_subQueueInfos;

  public:
    // CREATORS

    /// @brief Create an item holding a message without payload.
    ///
    /// This type does not declare the `bslma::UsesBslmaAllocator` trait, so
    /// the memory of the item itself and the memory of its fields come from
    /// different allocators.  The item can then be allocated from a pool
    /// holding blocks of the size of this type only.
    ///
    /// @param queueId                  Id of the queue the message belongs to.
    /// @param msgId                    Id of the message.
    /// @param flags                    Flags of the message.
    /// @param compressionAlgorithmType Compression of the payload.
    /// @param messagePropertiesInfo    Message properties of the message.
    /// @param subQueueInfos            Subscriptions to send the message for.
    /// @param state                    State used to cancel this item, may be
    ///                                 empty.
    /// @param allocator                Allocator to supply memory.
    explicit ChannelImplicitPayloadPushItem(
        int                                       queueId,
        const bmqt::MessageGUID&                  msgId,
        int                                       flags,
        bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
        const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
        const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
        const bsl::shared_ptr<bmqu::AtomicState>& state,
        bslma::Allocator*                         allocator);

    /// @brief Destructor.
    ~ChannelImplicitPayloadPushItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the id of the queue the message belongs to.
    int queueId() const;

    /// @brief Return the id of the message.
    const bmqt::MessageGUID& msgId() const;

    /// @brief Return the flags of the message.
    int flags() const;

    /// @brief Return the compression of the payload.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// @brief Return the message properties of the message.
    const bmqp::MessagePropertiesInfo& messagePropertiesInfo() const;

    /// @brief Return the subscriptions the message is sent for.
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos() const;
};

// ====================
// class ChannelAckItem
// ====================

/// Buffered ACK message.
class ChannelAckItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_ACK;

  private:
    // DATA

    /// Id of the message being acknowledged.
    const bmqt::MessageGUID d_guid;

    /// Status of the acknowledgment.
    const int d_status;

    /// Correlation id of the acknowledgment.
    const int d_correlationId;

    /// Id of the queue the message belongs to.
    const int d_queueId;

  public:
    // CREATORS

    /// @brief Create an item acknowledging the message with the specified
    /// `guid`.
    ///
    /// @param status        Status of the acknowledgment.
    /// @param correlationId Correlation id of the acknowledgment.
    /// @param guid          Id of the message being acknowledged.
    /// @param queueId       Id of the queue the message belongs to.
    /// @param state         State used to cancel this item, may be empty.
    explicit ChannelAckItem(int                      status,
                            int                      correlationId,
                            const bmqt::MessageGUID& guid,
                            int                      queueId,
                            const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// @brief Destructor.
    ~ChannelAckItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the id of the message being acknowledged.
    const bmqt::MessageGUID& guid() const;

    /// @brief Return the status of the acknowledgment.
    int status() const;

    /// @brief Return the correlation id of the acknowledgment.
    int correlationId() const;

    /// @brief Return the id of the queue the message belongs to.
    int queueId() const;
};

// ========================
// class ChannelConfirmItem
// ========================

/// Buffered CONFIRM message.
class ChannelConfirmItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_CONFIRM;

  private:
    // DATA

    /// Id of the message being confirmed.
    const bmqt::MessageGUID d_guid;

    /// Id of the queue the message belongs to.
    const int d_queueId;

    /// Id of the subscription the message was sent for.
    const int d_subQueueId;

  public:
    // CREATORS

    /// @brief Create an item confirming the message with the specified `guid`.
    ///
    /// @param queueId    Id of the queue the message belongs to.
    /// @param subQueueId Id of the subscription the message was sent for.
    /// @param guid       Id of the message being confirmed.
    /// @param state      State used to cancel this item, may be empty.
    explicit ChannelConfirmItem(
        int                                       queueId,
        int                                       subQueueId,
        const bmqt::MessageGUID&                  guid,
        const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// @brief Destructor.
    ~ChannelConfirmItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the id of the message being confirmed.
    const bmqt::MessageGUID& guid() const;

    /// @brief Return the id of the queue the message belongs to.
    int queueId() const;

    /// @brief Return the id of the subscription the message was sent for.
    int subQueueId() const;
};

// =======================
// class ChannelRejectItem
// =======================

/// Buffered REJECT message.
class ChannelRejectItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_REJECT;

  private:
    // DATA

    /// Id of the message being rejected.
    const bmqt::MessageGUID d_guid;

    /// Id of the queue the message belongs to.
    const int d_queueId;

    /// Id of the subscription the message was sent for.
    const int d_subQueueId;

  public:
    // CREATORS

    /// @brief Create an item rejecting the message with the specified `guid`.
    ///
    /// @param queueId    Id of the queue the message belongs to.
    /// @param subQueueId Id of the subscription the message was sent for.
    /// @param guid       Id of the message being rejected.
    /// @param state      State used to cancel this item, may be empty.
    explicit ChannelRejectItem(
        int                                       queueId,
        int                                       subQueueId,
        const bmqt::MessageGUID&                  guid,
        const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// @brief Destructor.
    ~ChannelRejectItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the id of the message being rejected.
    const bmqt::MessageGUID& guid() const;

    /// @brief Return the id of the queue the message belongs to.
    int queueId() const;

    /// @brief Return the id of the subscription the message was sent for.
    int subQueueId() const;
};

// =====================
// class ChannelBlobItem
// =====================

/// Buffered event blob that is written as built by the caller.  This holds
/// every event type for which `mqbnet::Channel` keeps no builder.
class ChannelBlobItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_BLOB;

  private:
    // DATA

    /// Contents of the event.
    const bsl::shared_ptr<bdlbb::Blob> d_data_sp;

    /// Type of the event.
    const bmqp::EventType::Enum d_eventType;

  public:
    // CREATORS

    /// @brief Create an item holding the specified `data` of the specified
    /// `eventType`.
    ///
    /// @param data      Contents of the event, must not be empty.
    /// @param eventType Type of the event.
    /// @param state     State used to cancel this item, may be empty.
    explicit ChannelBlobItem(const bsl::shared_ptr<bdlbb::Blob>& data,
                             bmqp::EventType::Enum               eventType,
                             const bsl::shared_ptr<bmqu::AtomicState>& state);

    /// @brief Destructor.
    ~ChannelBlobItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the contents of the event.
    const bsl::shared_ptr<bdlbb::Blob>& data() const;
};

// =======================
// class ChannelWakeUpItem
// =======================

/// Placeholder carrying no data, used to wake up the writing thread.
class ChannelWakeUpItem BSLS_KEYWORD_FINAL : public ChannelItem {
  public:
    // CLASS DATA

    /// The concrete type of this class.
    static const ChannelItemType::Enum k_TYPE = ChannelItemType::e_WAKE_UP;

    // CREATORS

    /// @brief Create an item carrying no data.
    explicit ChannelWakeUpItem();

    /// @brief Destructor.
    ~ChannelWakeUpItem() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// @brief Return the concrete type of this item.
    ChannelItemType::Enum type() const BSLS_KEYWORD_OVERRIDE;

    /// @brief Return the type of the event the data of this item belongs to.
    bmqp::EventType::Enum eventType() const BSLS_KEYWORD_OVERRIDE;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class ChannelItem
// -----------------

inline ChannelItem::ChannelItem(
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: d_state_sp(state)
{
    // NOTHING
}

inline const bsl::shared_ptr<bmqu::AtomicState>& ChannelItem::state() const
{
    return d_state_sp;
}

template <class ITEM_TYPE>
inline const ITEM_TYPE* ChannelItem::the() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(ITEM_TYPE::k_TYPE == type());

    return static_cast<const ITEM_TYPE*>(this);
}

// --------------------
// class ChannelPutItem
// --------------------

inline ChannelPutItem::ChannelPutItem(
    const bmqp::PutHeader&                    putHeader,
    const bsl::shared_ptr<bdlbb::Blob>&       data,
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: ChannelItem(state)
, d_putHeader(putHeader)
, d_data_sp(data)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_data_sp);
}

inline ChannelItemType::Enum ChannelPutItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelPutItem::eventType() const
{
    return bmqp::EventType::e_PUT;
}

inline const bmqp::PutHeader& ChannelPutItem::putHeader() const
{
    return d_putHeader;
}

inline const bsl::shared_ptr<bdlbb::Blob>& ChannelPutItem::data() const
{
    return d_data_sp;
}

// ------------------------------------
// class ChannelExplicitPayloadPushItem
// ------------------------------------

inline ChannelExplicitPayloadPushItem::ChannelExplicitPayloadPushItem(
    int                                       queueId,
    const bmqt::MessageGUID&                  msgId,
    int                                       flags,
    bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
    const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
    const bsl::shared_ptr<bdlbb::Blob>&       payload,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bsl::shared_ptr<bmqu::AtomicState>& state,
    bslma::Allocator*                         allocator)
: ChannelItem(state)
, d_data_sp(payload)
, d_queueId(queueId)
, d_msgId(msgId)
, d_flags(flags)
, d_compressionAlgorithmType(compressionAlgorithmType)
, d_messagePropertiesInfo(messagePropertiesInfo)
, d_subQueueInfos(subQueueInfos, allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_data_sp);
}

inline ChannelItemType::Enum ChannelExplicitPayloadPushItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelExplicitPayloadPushItem::eventType() const
{
    return bmqp::EventType::e_PUSH;
}

inline const bsl::shared_ptr<bdlbb::Blob>&
ChannelExplicitPayloadPushItem::data() const
{
    return d_data_sp;
}

inline int ChannelExplicitPayloadPushItem::queueId() const
{
    return d_queueId;
}

inline const bmqt::MessageGUID& ChannelExplicitPayloadPushItem::msgId() const
{
    return d_msgId;
}

inline int ChannelExplicitPayloadPushItem::flags() const
{
    return d_flags;
}

inline bmqt::CompressionAlgorithmType::Enum
ChannelExplicitPayloadPushItem::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

inline const bmqp::MessagePropertiesInfo&
ChannelExplicitPayloadPushItem::messagePropertiesInfo() const
{
    return d_messagePropertiesInfo;
}

inline const bmqp::Protocol::SubQueueInfosArray&
ChannelExplicitPayloadPushItem::subQueueInfos() const
{
    return d_subQueueInfos;
}

// ------------------------------------
// class ChannelImplicitPayloadPushItem
// ------------------------------------

inline ChannelImplicitPayloadPushItem::ChannelImplicitPayloadPushItem(
    int                                       queueId,
    const bmqt::MessageGUID&                  msgId,
    int                                       flags,
    bmqt::CompressionAlgorithmType::Enum      compressionAlgorithmType,
    const bmqp::MessagePropertiesInfo&        messagePropertiesInfo,
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos,
    const bsl::shared_ptr<bmqu::AtomicState>& state,
    bslma::Allocator*                         allocator)
: ChannelItem(state)
, d_queueId(queueId)
, d_msgId(msgId)
, d_flags(flags)
, d_compressionAlgorithmType(compressionAlgorithmType)
, d_messagePropertiesInfo(messagePropertiesInfo)
, d_subQueueInfos(subQueueInfos, allocator)
{
    // NOTHING
}

inline ChannelItemType::Enum ChannelImplicitPayloadPushItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelImplicitPayloadPushItem::eventType() const
{
    return bmqp::EventType::e_PUSH;
}

inline int ChannelImplicitPayloadPushItem::queueId() const
{
    return d_queueId;
}

inline const bmqt::MessageGUID& ChannelImplicitPayloadPushItem::msgId() const
{
    return d_msgId;
}

inline int ChannelImplicitPayloadPushItem::flags() const
{
    return d_flags;
}

inline bmqt::CompressionAlgorithmType::Enum
ChannelImplicitPayloadPushItem::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

inline const bmqp::MessagePropertiesInfo&
ChannelImplicitPayloadPushItem::messagePropertiesInfo() const
{
    return d_messagePropertiesInfo;
}

inline const bmqp::Protocol::SubQueueInfosArray&
ChannelImplicitPayloadPushItem::subQueueInfos() const
{
    return d_subQueueInfos;
}

// --------------------
// class ChannelAckItem
// --------------------

inline ChannelAckItem::ChannelAckItem(
    int                                       status,
    int                                       correlationId,
    const bmqt::MessageGUID&                  guid,
    int                                       queueId,
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: ChannelItem(state)
, d_guid(guid)
, d_status(status)
, d_correlationId(correlationId)
, d_queueId(queueId)
{
    // NOTHING
}

inline ChannelItemType::Enum ChannelAckItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelAckItem::eventType() const
{
    return bmqp::EventType::e_ACK;
}

inline const bmqt::MessageGUID& ChannelAckItem::guid() const
{
    return d_guid;
}

inline int ChannelAckItem::status() const
{
    return d_status;
}

inline int ChannelAckItem::correlationId() const
{
    return d_correlationId;
}

inline int ChannelAckItem::queueId() const
{
    return d_queueId;
}

// ------------------------
// class ChannelConfirmItem
// ------------------------

inline ChannelConfirmItem::ChannelConfirmItem(
    int                                       queueId,
    int                                       subQueueId,
    const bmqt::MessageGUID&                  guid,
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: ChannelItem(state)
, d_guid(guid)
, d_queueId(queueId)
, d_subQueueId(subQueueId)
{
    // NOTHING
}

inline ChannelItemType::Enum ChannelConfirmItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelConfirmItem::eventType() const
{
    return bmqp::EventType::e_CONFIRM;
}

inline const bmqt::MessageGUID& ChannelConfirmItem::guid() const
{
    return d_guid;
}

inline int ChannelConfirmItem::queueId() const
{
    return d_queueId;
}

inline int ChannelConfirmItem::subQueueId() const
{
    return d_subQueueId;
}

// -----------------------
// class ChannelRejectItem
// -----------------------

inline ChannelRejectItem::ChannelRejectItem(
    int                                       queueId,
    int                                       subQueueId,
    const bmqt::MessageGUID&                  guid,
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: ChannelItem(state)
, d_guid(guid)
, d_queueId(queueId)
, d_subQueueId(subQueueId)
{
    // NOTHING
}

inline ChannelItemType::Enum ChannelRejectItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelRejectItem::eventType() const
{
    return bmqp::EventType::e_REJECT;
}

inline const bmqt::MessageGUID& ChannelRejectItem::guid() const
{
    return d_guid;
}

inline int ChannelRejectItem::queueId() const
{
    return d_queueId;
}

inline int ChannelRejectItem::subQueueId() const
{
    return d_subQueueId;
}

// ---------------------
// class ChannelBlobItem
// ---------------------

inline ChannelBlobItem::ChannelBlobItem(
    const bsl::shared_ptr<bdlbb::Blob>&       data,
    bmqp::EventType::Enum                     eventType,
    const bsl::shared_ptr<bmqu::AtomicState>& state)
: ChannelItem(state)
, d_data_sp(data)
, d_eventType(eventType)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_data_sp);
}

inline ChannelItemType::Enum ChannelBlobItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelBlobItem::eventType() const
{
    return d_eventType;
}

inline const bsl::shared_ptr<bdlbb::Blob>& ChannelBlobItem::data() const
{
    return d_data_sp;
}

// -----------------------
// class ChannelWakeUpItem
// -----------------------

inline ChannelWakeUpItem::ChannelWakeUpItem()
: ChannelItem(bsl::shared_ptr<bmqu::AtomicState>())
{
    // NOTHING
}

inline ChannelItemType::Enum ChannelWakeUpItem::type() const
{
    return k_TYPE;
}

inline bmqp::EventType::Enum ChannelWakeUpItem::eventType() const
{
    return bmqp::EventType::e_UNDEFINED;
}

}  // close package namespace
}  // close enterprise namespace

#endif
