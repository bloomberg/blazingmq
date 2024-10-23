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

// bmqp_schemaeventbuilder.h                                          -*-C++-*-
#ifndef INCLUDED_BMQP_SCHEMAEVENTBUILDER
#define INCLUDED_BMQP_SCHEMAEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ schema event.
//
//@CLASSES:
//  bmqp::SchemaEventBuilder: Mechanism to build a BlazingMQ schema event.
//
//@DESCRIPTION: 'bmqp::SchemaEventBuilder' provides a mechanism to encode a
// 'bmqp_ctrlmsg' message into a properly formatted bmqp::Event blob.  Messages
// are encoded using one of the encodings in 'bmqp::EncodingType::Enum. Schema
// Event messages are used to send infrequent messages between a BlazingMQ
// client and the broker, or between brokers.
//
/// Padding
///-------
// Schema Event messages are not meant to be sent in batch and therefore do not
// require padding to be added; however, due to the way data is read from the
// channel pool blob, padding is added so that the next message read from the
// channel will start aligned.
//
/// Thread Safety
///-------------
// NOT thread safe
//
/// Usage
///-----
//..
//  bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
//
//  bmqp_ctrlmsg::ControlMessage message(&localAllocator);
//  message.choice().makeXXX();
//  [...]
//
//  bmqp::SchemaEventBuilder builder(d_bufferFactory_p,
//                                   d_allocator_p,
//                                   bmqp::EncodingType::e_JSON);
//
//  // Encode the message in the control event
//  int rc = builder.setMessage(message, bmqp::EventType::e_CONTROL);
//  if (rc != 0) {
//      // Failed to encode the message
//  }
//
//  // Retrieve the encoding full event message
//  const bdlbb::Blob& blob = builder.blob();
//
//  // Send the event ...
//..

// BMQ

#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlbb_blob.h>
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// ========================
// class SchemaEventBuilder
// ========================

/// Mechanism to build a BlazingMQ schema event
class SchemaEventBuilder {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQP.SCHEMAEVENTBUILDER");

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use by this object.

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;
    // blob containing the event under construction.
    // This has been done mutable to be able to skip
    // writing the length until the blob is retrieved.
    EncodingType::Enum d_encodingType;
    // Encoding type for encoding the message

  private:
    // NOT IMPLEMENTED
    SchemaEventBuilder(const SchemaEventBuilder&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    SchemaEventBuilder&
    operator=(const SchemaEventBuilder&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SchemaEventBuilder,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of a `SchemaEventBuilder` using the specified
    /// `bufferFactory` and `allocator` for the internal blob data member,
    /// and the optionally specified `encodingType` (default to `ber`) for
    /// encoding the message.
    SchemaEventBuilder(
        bdlbb::BlobBufferFactory* bufferFactory,
        bslma::Allocator*         allocator,
        bmqp::EncodingType::Enum  encodingType = bmqp::EncodingType::e_BER);

    // MANIPULATORS

    /// Clear the blob owned by this SchemaEventBuilder so that it can be
    /// used to build a new event.
    void reset();

    /// Encode the templated specified `message` of the specified event
    /// `type` in this SchemaEvent, and return 0 on success or a non-zero
    /// result on error.  The behaviour is undefined unless `type` is
    /// `CONTROL` or `ELECTOR`.
    template <class TYPE>
    int setMessage(const TYPE& message, EventType::Enum type);

    // ACCESSORS

    /// Return the fully formatted blob corresponding to the message built.
    /// Note that if `setMessage` has not been called on this
    /// SchemaEventBuilder, or if `reset` has been called since, the blob
    /// returned will be an empty one.
    const bdlbb::Blob& blob() const;

    /// Return the fully formatted blob corresponding to the message built.
    /// Note that if `setMessage` has not been called on this
    /// SchemaEventBuilder, or if `reset` has been called since, the blob
    /// returned will be an empty one.
    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;
};

// =============================
// struct SchemaEventBuilderUtil
// =============================

/// This class provides utility functions for SchemaEventBuilder.
struct SchemaEventBuilderUtil {
    // CLASS METHODS

    /// Based on the specified `remoteFeatureSet` provided by the remote
    /// peer, return the best encoding type supported for communicating
    /// with the peer.
    static EncodingType::Enum
    bestEncodingSupported(const bsl::string& remoteFeatureSet);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class SchemaEventBuilder
// ------------------------

inline SchemaEventBuilder::SchemaEventBuilder(
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator,
    bmqp::EncodingType::Enum  encodingType)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_bufferFactory_p(bufferFactory)
, d_blob_sp(new(*d_allocator_p) bdlbb::Blob(bufferFactory, d_allocator_p),
            d_allocator_p)
, d_encodingType(encodingType)
{
    // NOTHING
}

inline void SchemaEventBuilder::reset()
{
    d_blob_sp.reset(new (*d_allocator_p)
                        bdlbb::Blob(d_bufferFactory_p, d_allocator_p),
                    d_allocator_p);
}

template <class TYPE>
int SchemaEventBuilder::setMessage(const TYPE& message, EventType::Enum type)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_blob_sp->length() == 0);  // Ensure the blob is empty
    BSLS_ASSERT_SAFE(d_encodingType != EncodingType::e_UNKNOWN);
    BSLS_ASSERT_SAFE(type == EventType::e_CONTROL ||
                     (type == EventType::e_ELECTOR &&
                      d_encodingType == EncodingType::e_BER));
    // Currently, we only support BER encoding for elector messages

    // NOTE: Since SchemaEventBuilder owns the blob and we know its empty, we
    //       have guarantee that buffer(0) will contain the entire header
    //       (unless the bufferFactory has blobs of ridiculously small size,
    //       which we assert against after growing the blob).

    // Insert EventHeader
    //
    // Ensure blob has enough space for an EventHeader.  Use placement new to
    // create the object directly in the blob buffer, while still calling it's
    // constructor (to memset memory and initialize some fields)
    d_blob_sp->setLength(sizeof(EventHeader));
    BSLS_ASSERT_SAFE(d_blob_sp->numDataBuffers() == 1 &&
                     "The buffers allocated by the supplied bufferFactory "
                     "are too small");

    EventHeader* eventHeader = new (d_blob_sp->buffer(0).data())
        EventHeader(type);

    // Specify the encoding type in the EventHeader for control messages
    if (type == EventType::e_CONTROL) {
        EventHeaderUtil::setControlEventEncodingType(eventHeader,
                                                     d_encodingType);
    }

    // Append appropriate encoding of 'message' to the blob
    mwcu::MemOutStream os(d_allocator_p);
    int                rc = ProtocolUtil::encodeMessage(os,
                                         d_blob_sp.get(),
                                         message,
                                         d_encodingType,
                                         d_allocator_p);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!os.isEmpty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_DEBUG << os.str();
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Failed to encode
        return rc;  // RETURN
    }

    // Make sure the event is padded
    ProtocolUtil::appendPadding(d_blob_sp.get(), d_blob_sp->length());

    // Fix packet's length in header now that we know it ..
    eventHeader->setLength(d_blob_sp->length());

    // Guard against too big events
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_blob_sp->length() >
                                              EventHeader::k_MAX_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        reset();
        return -1;  // RETURN
    }

    return 0;
}

inline const bdlbb::Blob& SchemaEventBuilder::blob() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_blob_sp->length() <= EventHeader::k_MAX_SIZE_SOFT);
    BSLS_ASSERT_SAFE(d_blob_sp->length() % 4 == 0);

    return *d_blob_sp;
}

inline bsl::shared_ptr<bdlbb::Blob> SchemaEventBuilder::blob_sp() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_blob_sp->length() <= EventHeader::k_MAX_SIZE_SOFT);
    BSLS_ASSERT_SAFE(d_blob_sp->length() % 4 == 0);

    return d_blob_sp;
}

}  // close package namespace
}  // close enterprise namespace

#endif
