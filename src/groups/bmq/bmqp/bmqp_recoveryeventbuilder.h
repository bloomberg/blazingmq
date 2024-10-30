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
//  bdlbb::PooledBlobBufferFactory bufferFactory(1024, d_allocator_p);
//  bmqp::RecoveryEventBuilder builder(&bufferFactory, d_allocator_p);
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

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return an empty
    /// blob, i.e., a blob with length == 0.
    const bdlbb::Blob& blob() const;

    /// Return a shared pointer to the built Blob.  If no messages were added,
    /// this will return an empty shared pointer.
    /// Note that a shared pointer is returned by value, so the user holds to
    /// the copy of a pointer.  The Blob in that copy will be valid even if we
    /// `reset` this builder and modify the internal shared pointer.
    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;
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
