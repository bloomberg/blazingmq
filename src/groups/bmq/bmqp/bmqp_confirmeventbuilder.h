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

// bmqp_confirmeventbuilder.h                                         -*-C++-*-
#ifndef INCLUDED_BMQP_CONFIRMEVENTBUILDER
#define INCLUDED_BMQP_CONFIRMEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'CONFIRM' event.
//
//@CLASSES:
//  bmqp::ConfirmEventBuilder: mechanism to build a BlazingMQ CONFIRM event.
//
//@DESCRIPTION: 'bmqp::ConfirmEventBuilder' provides a mechanism to build a
// 'ConfirmEvent'. Such event starts by an 'EventHeader', followed by an
// 'ConfirmHeader' followed by one or multiple consecutive 'ConfirmMessage'.
// An 'ConfirmEventBuilder' can be reused to build multiple Events, by calling
// the 'reset()' method on it.
//
/// Padding
///-------
// ConfirmEvent messages are not meant to be sent in batch and are therefore
// not padded to be aligned; however the individual 'ConfirmMessage' composing
// it are (naturally) aligned.
//
/// Thread Safety
///-------------
// NOT thread safe
//
/// Usage
///-----
//..
//  bdlbb::PooledBlobBufferFactory   bufferFactory(1024, s_allocator_p);
//  bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
//        bmqp::BlobPoolUtil::createBlobPool(&bufferFactory, s_allocator_p));
//  bmqp::ConfirmEventBuilder builder(blobSpPool.get(), d_allocator_p);
//
//  // Append multiple messages, from same or different queue
//  builder.appendMessage(k_QUEUEID1, k_SUBQUEUEID1, bmqt::MessageGUID());
//  builder.appendMessage(k_QUEUEID2, k_SUBQUEUEID2, bmqt::MessageGUID());
//
//  const bsl::shared_ptr<bdlbb::Blob>& eventBlob = builder.blob();
//  // Send the blob ...
//
//  // We can reset the builder to reuse it; note that this invalidates the
//  // 'eventBlob' shared pointer reference retrieved above.  To keep the
//  // bdlbb::Blob valid the shared pointer should be copied, and the copy
//  // should be passed and kept in IO components.
//  builder.reset();
//
//..

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// =========================
// class ConfirmEventBuilder
// =========================

/// Mechanism to build a BlazingMQ CONFIRM event
class ConfirmEventBuilder {
  public:
    // TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

  private:
    // DATA
    /// Blob pool to use.  Held, not owned.
    BlobSpPool* d_blobSpPool_p;

    /// Blob being built by this object.
    /// `mutable` to skip writing the length until the blob is retrieved.
    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;

    /// Empty blob to be returned when no messages were added to this builder.
    bsl::shared_ptr<bdlbb::Blob> d_emptyBlob_sp;

    int d_msgCount;              // number of messages currently in the
                                 // event

  private:
    // NOT IMPLEMENTED
    ConfirmEventBuilder(const ConfirmEventBuilder&) BSLS_KEYWORD_DELETED;

    /// Copy constructor and assignment operator not implemented
    ConfirmEventBuilder&
    operator=(const ConfirmEventBuilder&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConfirmEventBuilder,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `ConfirmEventBuilder` using the specified `blobSpPool_p`
    /// and `allocator` for the blob.  We require BlobSpPool to build Blobs
    /// with set BlobBufferFactory since we might want to expand the built Blob
    /// dynamically.
    ConfirmEventBuilder(BlobSpPool* blobSpPool_p, bslma::Allocator* allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `ConfirmEvent`.  Note that calling reset invalidates the
    /// content of the blob returned by the `blob()` method.
    void reset();

    /// Append a ConfirmMessage for the specified `queueId`, `subQueueId`,
    /// and `guid` to the event being built.  Return 0 if the message was
    /// successfully added, or a non-zero code if it failed (due to event
    /// being full).
    bmqt::EventBuilderResult::Enum
    appendMessage(int queueId, int subQueueId, const bmqt::MessageGUID& guid);

    // ACCESSORS

    /// Return the number of messages currently in the event being built.
    int messageCount() const;

    /// Return the maximum number of messages that can be added to this
    /// event, with respect to protocol limitations.
    int maxMessageCount() const;

    /// Return the current size of the event being built.  If no messages
    /// were added, this will return 0.
    int eventSize() const;

    /// Return a reference to the shared pointer to the built Blob.  If no
    /// messages were added, the Blob object under this reference will be
    /// empty.
    /// Note that this accessor exposes an internal shared pointer object, and
    /// it is the user's responsibility to make a copy of it if it needs to be
    /// passed and kept in another thread while this builder object is used.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class ConfirmEventBuilder
// -------------------------

inline int ConfirmEventBuilder::messageCount() const
{
    return d_msgCount;
}

inline int ConfirmEventBuilder::maxMessageCount() const
{
    static const int res = (EventHeader::k_MAX_SIZE_SOFT -
                            sizeof(EventHeader) - sizeof(ConfirmHeader)) /
                           sizeof(ConfirmMessage);
    return res;
}

inline int ConfirmEventBuilder::eventSize() const
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
