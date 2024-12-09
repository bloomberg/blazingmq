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

// bmqp_recoveryeventbuilder.h                                        -*-C++-*-
#ifndef INCLUDED_BMQP_RECOVERYEVENTBUILDER
#define INCLUDED_BMQP_RECOVERYEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'RECOVERY' event.
//
//@CLASSES:
//  bmqp::RecoveryEventBuilder: mechanism to build a BlazingMQ  RECOVERY event.
//
//@DESCRIPTION: 'bmqp::RecoveryEventBuilder' provides a mechanism to build a
// 'RecoveryEvent'. Such event starts by an 'EventHeader', followed by one or
// many repetitions of a pair of 'RecoveryHeader' + message payload.  A
// 'RecoveryEventBuilder' can be reused to build multiple Events, by calling
// the 'reset()' method on it.
//
/// Padding
///-------
// Note that messages added to the RecoveryEvent are *not* padded.  This is
// unlike other event builders in bmqp.  The reason for absence of padding is
// that storage messages are already padded when written to storage files.  If
// this ever changes, this component will need to be updated accordingly.
//
/// Thread Safety
///-------------
// NOT thread safe
//
/// Usage
///-----
//..
//  bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
//  bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
//        bmqp::BlobPoolUtil::createBlobPool(&bufferFactory, s_allocator_p));
//  bmqp::RecoveryEventBuilder builder(&blobSpPool, d_allocator_p);
//
//  // Append multiple messages
//  builder.appendMessage(0, 1, bmqt::MessageGUID(), 1);
//  builder.appendMessage(-1, 2, bmqt::MessageGUID(), 1);
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
//

// BMQ
#include <bmqp_blobpoolutil.h>
#include <bmqp_protocol.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqp {

// ==========================
// class RecoveryEventBuilder
// ==========================

/// Mechanism to build a BlazingMQ RECOVERY event
class RecoveryEventBuilder BSLS_CPP11_FINAL {
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

    int d_msgCount;  // number of messages currently in the
                     // event

  private:
    // NOT IMPLEMENTED
    RecoveryEventBuilder(const RecoveryEventBuilder&) BSLS_CPP11_DELETED;
    RecoveryEventBuilder&
    operator=(const RecoveryEventBuilder&) BSLS_CPP11_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(RecoveryEventBuilder,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `RecoveryEventBuilder` using the specified `blobSpPool_p`
    /// and `allocator` for the blob.  We require BlobSpPool to build Blobs
    /// with set BlobBufferFactory since we might want to expand the built Blob
    /// dynamically.
    RecoveryEventBuilder(BlobSpPool*       blobSpPool_p,
                         bslma::Allocator* allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `RecoveryEvent`.  Note that calling reset invalidates
    /// the content of the blob returned by the `blob()` method.
    void reset();

    /// Pack message i.e. add header and payload to the underlying blob
    /// being built as per the specified `partitionId`, `chunkFileType` with
    /// the specified `sequenceNumber` and using the chunks from the
    /// specified `chunkBuffer`. The specified `isFinal` flag is used to
    /// determine if the current packed message is the last message. Also,
    /// optionally specify whether Md5 calculation is unnecessary as per the
    /// specified `isSetMd5` flag.
    bmqt::EventBuilderResult::Enum
    packMessage(unsigned int                partitionId,
                RecoveryFileChunkType::Enum chunkFileType,
                unsigned int                sequenceNumber,
                const bdlbb::BlobBuffer&    chunkBuffer,
                bool                        isFinal,
                bool                        isSetMd5 = true);

    // ACCESSORS

    /// Return the current size of the event being built.
    int eventSize() const;

    /// Return the number of messages currently in the event being built.
    int messageCount() const;

    /// Return a reference to the shared pointer to the built Blob.
    /// Note that this accessor exposes an internal shared pointer object, and
    /// it is the user's responsibility to make a copy of it if it needs to be
    /// passed and kept in another thread while this builder object is used.
    const bsl::shared_ptr<bdlbb::Blob>& blob() const;
};

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class RecoveryEventBuilder
// --------------------------

// ACCESSORS
inline int RecoveryEventBuilder::eventSize() const
{
    return d_blob_sp->length();
}

inline int RecoveryEventBuilder::messageCount() const
{
    return d_msgCount;
}

}  // close package namespace
}  // close enterprise namespace

#endif
