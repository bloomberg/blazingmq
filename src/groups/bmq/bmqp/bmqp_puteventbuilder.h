// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqp_puteventbuilder.h                                             -*-C++-*-
#ifndef INCLUDED_BMQP_PUTEVENTBUILDER
#define INCLUDED_BMQP_PUTEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'PUT' event.
//
//@CLASSES:
//  bmqp::PutEventBuilder: mechanism to build a BlazingMQ PUT event.
//
//@DESCRIPTION: 'bmqp::PutEventBuilder' provides a mechanism to build a
// 'PutEvent'. Such event starts by an 'EventHeader', followed by one or many
// repetitions of a pair of 'PutHeader' + message payload.  A 'PutEventBuilder'
// can be reused to build multiple Events, by calling the 'reset()' method on
// it.
//
/// Padding
///-------
// Each message added to the PutEvent is padded, so that multiple messages can
// be added in the same event, without impacting the alignment of the headers.
//
/// Thread Safety
///-------------
// NOT thread safe
//
/// Usage
///-----
//..
//  bdlbb::PooledBlobBufferFactory bufferFactory(1024, d_allocator_p);
//  bmqp::PutEventBuilder builder(&bufferFactory, d_allocator_p);
//
//  // Append multiple messages
//  // (Error handling omitted below for brevity)
//  builder.startMessage();
//  builder.setMessagePayload();
//  builder.packMessage();
//
//  // Repeat above steps if adding more messages to this event is desired.
//
//  const bdlbb::Blob& eventBlob = builder.blob();
//  // Send the blob ...
//
//  // We can reset the builder to reuse it; note that this invalidates the
//  // 'eventBlob' retrieved above
//  builder.reset();
//..
//

// BMQ

#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace bmqp {

// =====================
// class PutEventBuilder
// =====================

/// Mechanism to build a BlazingMQ PUT event
class PutEventBuilder {
  public:
    // TYPES
    typedef bdlb::NullableValue<bmqp::Protocol::MsgGroupId> NullableMsgGroupId;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;
    // blob being built by this
    // PutEventBuilder.
    // This has been done mutable to be able to
    // skip writing the length until the blob
    // is retrieved.

    bool d_msgStarted;
    // has startMessage been called

    const bdlbb::Blob* d_blobPayload_p;
    // pointer to a blob holding the
    // payload of the current message
    //(if any)

    const char* d_rawPayload_p;
    // raw pointer to raw data buffer
    // holding the payload of the
    // current message (if any)

    int d_rawPayloadLength;
    // if d_rawPayload_p is not null,
    // this represents the size of that
    // buffer

    const MessageProperties* d_properties_p;
    // Pointer to message properties of
    // the current message (if any)

    int d_flags;
    // Flags for PutHeader of current
    // message.

    bmqt::MessageGUID d_messageGUID;
    // GUID of the current message.

    NullableMsgGroupId d_msgGroupId;
    // Optional Group Id of the current
    // message.

    int d_msgCount;
    // number of messages currently in
    // the event

    unsigned int d_crc32c;
    // CRC-32C of the current message's
    // payload (user either sets it
    // explicitly or indirectly by
    // invoking 'packMessage').

    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
    // Compression Algorithm Type of the
    // current message's payload (the
    // user sets it explicitly)

    double d_lastPackedMessageCompressionRatio;
    // Compression ratio of the last
    // packed message, or -1 if no
    // message has yet been packed.
    // Note that if message was not
    // compressed, this ratio will be 1.

    MessagePropertiesInfo d_messagePropertiesInfo;

  private:
    // NOT IMPLEMENTED
    PutEventBuilder(const PutEventBuilder&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator not implemented
    PutEventBuilder& operator=(const PutEventBuilder&) BSLS_CPP11_DELETED;

  private:
    // CLASS LEVEL METHODS

    /// Reset flags and message guid of PutEventBuilder instance pointed by
    /// the specified `ptr`.
    static void resetFields(void* ptr);

    // PRIVATE MANIPULATORS
    bmqt::EventBuilderResult::Enum
    packMessageInternal(const bdlbb::Blob& appData, int queueId);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PutEventBuilder, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `PutEventBuilder` using the specified `bufferFactory`
    /// and `allocator` for the blob.
    PutEventBuilder(bdlbb::BlobBufferFactory* bufferFactory,
                    bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `PutEvent`.  Note that calling reset invalidates the
    /// content of the blob returned by the `blob()` method.  Return 0 on
    /// success, or non-zero on error.
    int reset();

    /// Reset the current message being built and start a new one.
    void startMessage();

    /// Set the specified `flag` in the flags bit-mask of the PutHeader for
    /// the current message and return a reference offering modifiable
    /// access to this object.
    PutEventBuilder& setFlag(PutHeaderFlags::Enum flag);

    /// Set the flags for PutHeader of the current message to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    PutEventBuilder& setFlags(int value);

    /// Set the Group Id of the current message to the specified `value` and
    /// return a reference offering modifiable access to this object.
    PutEventBuilder& setMsgGroupId(const bmqp::Protocol::MsgGroupId& value);

    /// Set the message guid of the current message to the specified `value`
    /// and return a reference offering modifiable access to this object.
    PutEventBuilder& setMessageGUID(const bmqt::MessageGUID& value);

    /// Set the payload of the current message to the blob in the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.
    PutEventBuilder& setMessagePayload(const bdlbb::Blob* value);

    /// Set the payload of the current message to the specified `data` of
    /// the specified `length` and return a reference offering modifiable
    /// access to this object.
    PutEventBuilder& setMessagePayload(const char* data, int length);

    /// Set the properties of the current message to the specified `value`
    /// and return a reference offering modifiable access to this object.
    PutEventBuilder& setMessageProperties(const MessageProperties* value);

    /// Set the CRC-32C of the current message's payload to the specified
    /// `value` and return a reference offering modifiable access to this
    /// object.  Note that this builder sets CRC on its own when
    /// `packMessage` is invoked.
    PutEventBuilder& setCrc32c(unsigned int value);

    /// Set the Compression algorithm type of the current message to the
    /// specified `value` and return a reference offering modifiable access
    /// to this object.
    PutEventBuilder&
    setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::Enum value);

    /// Set the knowledge about MessageProperties presence and their Schema
    /// Id in the current message to the specified `value` and return a
    /// reference offering modifiable access to this object.
    PutEventBuilder&
    setMessagePropertiesInfo(const MessagePropertiesInfo& value);

    /// Clear out the properties, if any, of the current message and return
    /// a reference offering modifiable access to this object.
    PutEventBuilder& clearMessageProperties();

    /// Clear out the Group Id, if any, of the current message and return a
    /// reference offering modifiable access to this object.
    PutEventBuilder& clearMsgGroupId();

    /// Add the current message to the underlying event blob with the
    /// specified `queueId` as the destination queue.  Return zero on
    /// success, and a meaningful non-zero error code otherwise.  In case of
    /// failure, this method has no effect on the underlying event blob; but
    /// the guid, compressionAlgorithmType and flags, if set, are reset.
    /// Behavior is undefined unless `startMessage` was called at least
    /// once.  Note that usage of this method in BlazingMQ backend must be
    /// carefully reviewed since this method compresses payload.
    bmqt::EventBuilderResult::Enum packMessage(int queueId);

    /// Temporary; shall remove after 2nd roll out of "new style" brokers.
    bmqt::EventBuilderResult::Enum packMessageInOldStyle(int queueId);

    /// Add the current message to the underlying event blob with the
    /// specified `queueId` as the destination queue.  Return zero on
    /// success, and a meaningful non-zero error code otherwise.  In case of
    /// failure, this method has no effect on the underlying event blob; but
    /// the guid, compressionAlgorithmType and flags, if set, are reset.
    /// Behavior is undefined unless `startMessage` was called at least
    /// once.  Behaviour is also undefined unless `d_crc32`,
    /// `d_compressionAlgorithmType`, `d_blobPayload_p`, `d_messageGUID` and
    /// `d_flags` are set before.  Note that this method *does* *not*
    /// compress the payload and will append it as is instead.
    bmqt::EventBuilderResult::Enum packMessageRaw(int queueId);

    // ACCESSORS

    /// Return the flags that were last set.  Note that flags are reset
    /// after every call to packMessage().
    int flags() const;

    /// Return the Group Id that was last set.  Note that Group Id is reset
    /// after every call to packMessage().
    const NullableMsgGroupId& msgGroupId() const;

    /// Return the message guid that was last set.  Note that an unset guid
    /// is a valid return value.  Also note that guid is reset after every
    /// call to packMessage().
    const bmqt::MessageGUID& messageGUID() const;

    /// Return the size in bytes of the event under construction.
    int eventSize() const;

    /// Return the size in bytes of the current unpacked message, or 0 if
    /// there are none (i.e., `start` and/or `setMessagePayload` were not
    /// called).  Note that returned value *does* *not* include the length
    /// of the associated properties.
    int unpackedMessageSize() const;

    /// Return the number of messages added.  Note that resetting the
    /// builder sets this number to zero.
    int messageCount() const;

    /// Return CRC32C of the current message.  Note that if `setCrc32c` has
    /// not been invoked, then zero will be returned.
    unsigned int crc32c() const;

    /// Return Compression algorithm type of the current message.  Note that
    /// if `setCompressionAlgorithmType` has not been invoked, then
    /// bmqt::CompressionAlgorithmType::e_NONE will be returned.
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType() const;

    /// Return the compression ratio of the last packed message, or -1 if no
    /// message was yet packed.  Note that compression ratio is computed by
    /// dividing the original message size, by its compressed one.  If the
    /// message was not compressed, a value of 1 is returned.
    double lastPackedMesageCompressionRatio() const;

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return a blob
    /// composed only of an `EventHeader`.
    const bdlbb::Blob& blob() const;

    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;

    const bmqp::MessageProperties* messageProperties() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class PutEventBuilder
// ---------------------

inline PutEventBuilder& PutEventBuilder::setFlag(PutHeaderFlags::Enum flag)
{
    PutHeaderFlagUtil::setFlag(&d_flags, flag);

    return *this;
}

inline PutEventBuilder& PutEventBuilder::setFlags(int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_flags = value;

    return *this;
}

inline PutEventBuilder&
PutEventBuilder::setMsgGroupId(const bmqp::Protocol::MsgGroupId& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_msgGroupId = value;

    return *this;
}

inline PutEventBuilder&
PutEventBuilder::setMessageGUID(const bmqt::MessageGUID& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_messageGUID = value;

    return *this;
}

inline PutEventBuilder&
PutEventBuilder::setMessagePayload(const bdlbb::Blob* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_blobPayload_p    = value;
    d_rawPayload_p     = 0;
    d_rawPayloadLength = 0;

    return *this;
}

inline PutEventBuilder& PutEventBuilder::setMessagePayload(const char* data,
                                                           int         length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= length);
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_blobPayload_p    = 0;
    d_rawPayload_p     = data;
    d_rawPayloadLength = length;

    return *this;
}

inline PutEventBuilder&
PutEventBuilder::setMessageProperties(const MessageProperties* value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value);

    d_properties_p = value;
    return *this;
}

inline PutEventBuilder& PutEventBuilder::setCrc32c(unsigned int value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_msgStarted == true);

    d_crc32c = value;
    return *this;
}

inline PutEventBuilder& PutEventBuilder::setCompressionAlgorithmType(
    bmqt::CompressionAlgorithmType::Enum value)
{
    d_compressionAlgorithmType = value;
    return *this;
}

inline PutEventBuilder&
PutEventBuilder::setMessagePropertiesInfo(const MessagePropertiesInfo& value)
{
    d_messagePropertiesInfo = value;
    return *this;
}

inline PutEventBuilder& PutEventBuilder::clearMessageProperties()
{
    d_properties_p          = 0;
    d_messagePropertiesInfo = MessagePropertiesInfo();
    return *this;
}

inline PutEventBuilder& PutEventBuilder::clearMsgGroupId()
{
    d_msgGroupId.reset();
    return *this;
}

inline void PutEventBuilder::startMessage()
{
    d_msgStarted = true;

    // Reset message state
    d_blobPayload_p            = 0;
    d_rawPayload_p             = 0;
    d_rawPayloadLength         = 0;
    d_properties_p             = 0;
    d_flags                    = 0;
    d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
    d_messageGUID              = bmqt::MessageGUID();
    d_msgGroupId.reset();
    d_crc32c                = 0;
    d_messagePropertiesInfo = MessagePropertiesInfo();
}

// ACCESSORS
inline int PutEventBuilder::flags() const
{
    return d_flags;
}

inline const PutEventBuilder::NullableMsgGroupId&
PutEventBuilder::msgGroupId() const
{
    return d_msgGroupId;
}

inline const bmqt::MessageGUID& PutEventBuilder::messageGUID() const
{
    return d_messageGUID;
}

inline int PutEventBuilder::eventSize() const
{
    return d_blob_sp->length();
}

inline int PutEventBuilder::unpackedMessageSize() const
{
    // NOTE: If 'startMessage' or 'setMessagePayload' were not called, then
    //       'd_blobPayload_p' is guaranteed to be null, and
    //       'd_rawPayloadLength' equal to 0 - which respects the method's
    //       contract.
    return (d_blobPayload_p ? d_blobPayload_p->length() : d_rawPayloadLength);
}

inline int PutEventBuilder::messageCount() const
{
    return d_msgCount;
}

inline unsigned int PutEventBuilder::crc32c() const
{
    return d_crc32c;
}

inline bmqt::CompressionAlgorithmType::Enum
PutEventBuilder::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

inline double PutEventBuilder::lastPackedMesageCompressionRatio() const
{
    return d_lastPackedMessageCompressionRatio;
}

}  // close package namespace
}  // close enterprise namespace

#endif
