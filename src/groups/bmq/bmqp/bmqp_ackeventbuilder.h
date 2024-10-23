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

// bmqp_ackeventbuilder.h                                             -*-C++-*-
#ifndef INCLUDED_BMQP_ACKEVENTBUILDER
#define INCLUDED_BMQP_ACKEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'ACK' event.
//
//@CLASSES:
//  bmqp::AckEventBuilder: mechanism to build a BlazingMQ ACK event.
//
//@DESCRIPTION: 'bmqp::AckEventBuilder' provides a mechanism to build a
// 'AckEvent'. Such event starts by an 'EventHeader', followed by a 'AckHeader'
// followed by one or multiple consecutive 'AckMessage'.  An 'AckEventBuilder'
// can be reused to build multiple Events, by calling the 'reset()' method on
// it.
//
/// Padding
///-------
// AckEvent messages are not meant to be sent in batch and are therefore not
// padded to be aligned; however the individual 'AckMessage' composing it are
// (naturally) aligned.
//
/// Thread Safety
///-------------
// NOT thread safe
//
/// Usage
///-----
//..
//  bdlbb::BlobPooledBlobBufferFactory bufferFactory(1024, d_allocator_p);
//  bmqp::AckEventBuilder builder(&bufferFactory, d_allocator_p);
//
//  // Append multiple messages
//  builder.appendMessage(0, 1, bmqt::MessageGUID(), 1);
//  builder.appendMessage(-1, 2, bmqt::MessageGUID(), 1);
//
//  const bdlbb::Blob& eventBlob = builder.blob();
//  // Send the blob ...
//
//  // We can reset the builder to reuse it; note that this invalidates the
//  // 'eventBlob' retrieved above
//  builder.reset();
//..

// BMQ

#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

namespace bmqp {

// =====================
// class AckEventBuilder
// =====================

/// Mechanism to build a BlazingMQ ACK event
class AckEventBuilder BSLS_CPP11_FINAL {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    /// Blob being built by this object.
    /// `mutable` to skip writing the length until the blob is retrieved.
    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    int d_msgCount;              // number of messages currently in the
                                 // event

  private:
    // NOT IMPLEMENTED
    AckEventBuilder(const AckEventBuilder&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator not implemented
    AckEventBuilder& operator=(const AckEventBuilder&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AckEventBuilder, bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `AckEventBuilder` using the specified `bufferFactory`
    /// and `allocator` for the blob.
    AckEventBuilder(bdlbb::BlobBufferFactory* bufferFactory,
                    bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `AckEvent`.  Note that calling reset invalidates the
    /// content of the blob returned by the `blob()` method.
    void reset();

    /// Append an AckMessage for the specified `status`, `correlationId`,
    /// `guid` and `queueId` to the event being built.  Return 0 if the
    /// message was successfully added, or a non-zero code if it failed (due
    /// to event being full).
    bmqt::EventBuilderResult::Enum appendMessage(int status,
                                                 int correlationId,
                                                 const bmqt::MessageGUID& guid,
                                                 int queueId);

    // ACCESSORS

    /// Return the number of messages currently in the event being built.
    int messageCount() const;

    /// Return the maximum number of messages that can be added to this
    /// event, with respect to protocol limitations.
    int maxMessageCount() const;

    /// Return the current size of the event being built.  If no messages
    /// were added, this will return 0.
    int eventSize() const;

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return an empty
    /// blob, i.e., a blob with length == 0.
    const bdlbb::Blob& blob() const;

    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class AckEventBuilder
// ---------------------

inline int AckEventBuilder::messageCount() const
{
    return d_msgCount;
}

inline int AckEventBuilder::maxMessageCount() const
{
    static const int res = (EventHeader::k_MAX_SIZE_SOFT -
                            sizeof(EventHeader) - sizeof(AckHeader)) /
                           sizeof(AckMessage);
    return res;
}

inline int AckEventBuilder::eventSize() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    return d_blob_sp->length();
}

}  // close package namespace
}  // close enterprise namespace

#endif
