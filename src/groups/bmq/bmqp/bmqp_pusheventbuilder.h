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

// bmqp_pusheventbuilder.h                                            -*-C++-*-
#ifndef INCLUDED_BMQP_PUSHEVENTBUILDER
#define INCLUDED_BMQP_PUSHEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'PUSH' event.
//
//@CLASSES:
//  bmqp::PushEventBuilder: mechanism to build a BlazingMQ PUSH event.
//
//@DESCRIPTION: 'bmqp::PushEventBuilder' provides a mechanism to build a
// 'PushEvent'. Such event starts by an 'EventHeader', followed by one or many
// repetitions of a pair of 'PushHeader' + message payload.  A
// 'PushEventBuilder' can be reused to build multiple Events, by calling the
// 'reset()' method on it.
//
/// Padding
///-------
// Each message added to the PushEvent is padded, so that multiple messages can
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
//  bmqp::PushEventBuilder builder(&bufferFactory, d_allocator_p);
//
//  // Append multiple messages
//  builder.packMessage("hello", 5, 0, bmqt::MessageGUID());
//  builder.packMessage(myBlob, 0, bmqt::MessageGUID());
//
//  const bdlbb::Blob& eventBlob = builder.blob();
//  // Send the blob ...
//
//  // We can reset the builder to reuse it; note that this invalidates the
//  // 'eventBlob' retrieved above
//  builder.reset();
//..

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>

namespace BloombergLP {

namespace bmqp {

// ======================
// class PushEventBuilder
// ======================

/// Mechanism to build a BlazingMQ PUSH event
class PushEventBuilder {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // DATA
    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

    /// Blob pool to use.  Held, not owned.
    BlobSpPool* d_blobSpPool_p;

    /// Blob being built by this object.
    /// `mutable` to skip writing the length until the blob is retrieved.
    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    int d_msgCount;
    // number of messages currently in
    // the event.

    OptionUtil::OptionsBox d_options;
    // Provides information and operations
    // associated with the options of the
    // current (to-be-packed) message.

    bmqu::BlobObjectProxy<PushHeader> d_currPushHeader;
    // Push Header associated with the
    // current (to-be-packed) message.

  private:
    // PRIVATE MANIPULATORS

    /// Add a message to the event being built, having the specified
    /// `queueId` and `msgId` and `payload` and the specified `flags` and
    /// the specified `compressionAlgorithmType`.  Use the specified
    /// `propertiesLogic` to encode MessageProperties related flag and
    /// Schema Id.
    /// Return 0 on success, or a meaningful non-zero error code otherwise.
    /// In case of failure, this method has no effect on the underlying
    /// event blob.
    bmqt::EventBuilderResult::Enum packMessageImp(
        const bdlbb::Blob&                   payload,
        int                                  queueId,
        const bmqt::MessageGUID&             msgId,
        int                                  flags,
        bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
        const MessagePropertiesInfo&         messagePropertiesInfo);

    /// Check if `PushHeader` has been written for current event.  If it
    /// hasn't, add the `PushHeader`.  The behavior is undefined unless
    /// there's enough space reserved on the underlying event blob.
    void ensurePushHeader();

    /// Erase from the event being built the PushHeader and options that
    /// have been added to the current (to-be-packed) message, if these were
    /// written to the underlying blob.  Return 0 on success, and non-zero
    /// on error.
    int eraseCurrentMessage();

  private:
    // NOT IMPLEMENTED
    PushEventBuilder(const PushEventBuilder&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator not implemented
    PushEventBuilder& operator=(const PushEventBuilder&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PushEventBuilder, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `PushEventBuilder` using the specified `blobSpPool_p` and
    /// `allocator` for the blob.  We require BlobSpPool to build Blobs with
    /// set BlobBufferFactory since we might want to expand the built Blob
    /// dynamically.
    PushEventBuilder(BlobSpPool* blobSpPool_p, bslma::Allocator* allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `PushEvent`.  Note that calling reset invalidates the
    /// content of the blob returned by the `blob()` method.  Return 0 on
    /// success, or non-zero on error.
    int reset();

    /// Add a message to the event being built, having the specified
    /// `queueId`, `msgId`, `payload`, `flags` and
    /// `compressionAlgorithmType`.  Use the specified `propertiesLogic` to
    /// encode MessageProperties related flag and Schema Id.
    /// Return 0 on success, or a meaningfulnon-zero error code otherwise.
    /// In case of failure, this method has no effect on the underlying
    /// event blob.
    bmqt::EventBuilderResult::Enum
    packMessage(const bdlbb::Blob&                   payload,
                int                                  queueId,
                const bmqt::MessageGUID&             msgId,
                int                                  flags,
                bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
                const MessagePropertiesInfo&         messagePropertiesInfo =
                    bmqp::MessagePropertiesInfo());

    /// Add a message to the event being built, having the specified
    /// `queueId`, `msgId`, `flags` and `compressionAlgorithmType`.  Note
    /// that since no payload is specified, in addition to `flags`, the
    /// `implicit payload` flag will also be set.  Use the specified
    /// `propertiesLogic` to encode MessageProperties related flag and
    /// Schema Id.
    /// Return 0 on success, or a meaningful non-zero error code otherwise.
    /// In case of failure, this method has no effect on the underlying
    /// event blob.
    bmqt::EventBuilderResult::Enum
    packMessage(int                                  queueId,
                const bmqt::MessageGUID&             msgId,
                int                                  flags,
                bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
                const MessagePropertiesInfo&         messagePropertiesInfo =
                    bmqp::MessagePropertiesInfo());

    /// Add a message to the event being built, having the specified
    /// `payload` and `header`.  Return 0 on success, or a meaningful
    /// non-zero error code otherwise.  In case of failure, this method has
    /// no effect on the underlying event blob.
    bmqt::EventBuilderResult::Enum packMessage(const bdlbb::Blob& payload,
                                               const PushHeader&  header);

    /// Add a SubQueueId option having the specified `subQueueIds` to the
    /// current (to-be-packed) message in the event being built.  Return 0
    /// on success, or a meaningful non-zero error code otherwise.  In case
    /// of failure, this method has no effect on the underlying event blob.
    bmqt::EventBuilderResult::Enum
    addSubQueueIdsOption(const Protocol::SubQueueIdsArrayOld& subQueueIds);

    /// Add a SubQueueInfo option having the specified `subQueueInfos` to
    /// the current (to-be-packed) message in the event being built.  If the
    /// specified `packRdaCounter` is true, also pack the remaining delivery
    /// attempts counter (`rdaCounter`).  If `packRdaCounter` is true and
    /// `subQueueInfos` contains only the default SubQueueId, pack a special
    /// option header carrying information about the RDA counter.  Return 0
    /// on success, or a meaningful non-zero error code otherwise.  In case
    /// of failure, this method has no effect on the underlying event blob.
    bmqt::EventBuilderResult::Enum
    addSubQueueInfosOption(const Protocol::SubQueueInfosArray& subQueueInfos,
                           bool packRdaCounter = true);

    /// Add a Group Id option having the specified `msgGroupId` to the
    /// current (to-be-packed) message in the event being built.  Return 0
    /// on success, or a meaningful non-zero error code otherwise.  In case
    /// of failure, this method has no effect on the underlying event blob.
    bmqt::EventBuilderResult::Enum
    addMsgGroupIdOption(const Protocol::MsgGroupId& msgGroupId);

    // ACCESSORS

    /// Return the current size of the event being built.  Note that this
    /// size excludes the PushHeader and options of a message that has not
    /// been packed with a call to one of the `packMessage` methods.
    int eventSize() const;

    /// Return the number of messages currently in the event being built.
    int messageCount() const;

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return a blob
    /// composed only of an `EventHeader`.
    const bdlbb::Blob& blob() const;

    /// Return a shared pointer to the built Blob.  If no messages were added,
    /// this will return an empty shared pointer.
    /// Note that a shared pointer is returned by value, so the user holds to
    /// the copy of a pointer.  The Blob in that copy will be valid even if we
    /// `reset` this builder and modify the internal shared pointer.
    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// class PushEventBuilder
// ----------------------

// PRIVATE MANIPULATORS
inline int PushEventBuilder::eraseCurrentMessage()
{
    // PRECONDITIONS
    const int optionsSize = d_options.size();

#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
    bool hasNoOptions    = optionsSize == 0 && !d_currPushHeader.isSet();
    bool hasOptions      = optionsSize > 0 && d_currPushHeader.isSet();
    bool isValidBlobSize = d_blob_sp->length() >
                           (static_cast<int>(sizeof(PushHeader)) +
                            optionsSize);

    BSLS_ASSERT_SAFE(hasNoOptions || (hasOptions && isValidBlobSize));
#endif

    if (optionsSize > 0) {
        // We previously wrote a PushHeader and options to the current message.
        d_currPushHeader.reset();
        // Flush any buffered changes if necessary, and make this object
        // not refer to any valid blob object.

        d_blob_sp->setLength(d_blob_sp->length() - sizeof(PushHeader) -
                             optionsSize);

        d_options.reset();
    }

    return 0;
}

// MANIPULATORS
inline bmqt::EventBuilderResult::Enum
PushEventBuilder::packMessage(const bdlbb::Blob& payload,
                              const PushHeader&  header)
{
    return packMessageImp(payload,
                          header.queueId(),
                          header.messageGUID(),
                          header.flags(),
                          header.compressionAlgorithmType(),
                          MessagePropertiesInfo(header));
}

inline bmqt::EventBuilderResult::Enum PushEventBuilder::packMessage(
    const bdlbb::Blob&                   payload,
    int                                  queueId,
    const bmqt::MessageGUID&             msgId,
    int                                  flags,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    const MessagePropertiesInfo&         messagePropertiesInfo)
{
    return packMessageImp(payload,
                          queueId,
                          msgId,
                          flags,
                          compressionAlgorithmType,
                          messagePropertiesInfo);
}

inline bmqt::EventBuilderResult::Enum PushEventBuilder::packMessage(
    int                                  queueId,
    const bmqt::MessageGUID&             msgId,
    int                                  flags,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    const MessagePropertiesInfo&         messagePropertiesInfo)
{
    const bdlbb::Blob emptyBlob;
    PushHeaderFlagUtil::setFlag(&flags, PushHeaderFlags::e_IMPLICIT_PAYLOAD);
    return packMessageImp(emptyBlob,
                          queueId,
                          msgId,
                          flags,
                          compressionAlgorithmType,
                          messagePropertiesInfo);
}

// ACCESSORS
inline int PushEventBuilder::eventSize() const
{
    // PRECONDITIONS
    const int optionsCount = d_options.optionsCount();
    BSLS_ASSERT_SAFE((optionsCount > 0 && d_currPushHeader.isSet()) ||
                     (optionsCount == 0 && !d_currPushHeader.isSet()));

    if (optionsCount > 0) {
        return d_blob_sp->length() - sizeof(PushHeader) - d_options.size();
        // RETURN
    }
    return d_blob_sp->length();
}

inline int PushEventBuilder::messageCount() const
{
    return d_msgCount;
}

}  // close package namespace
}  // close enterprise namespace

#endif
