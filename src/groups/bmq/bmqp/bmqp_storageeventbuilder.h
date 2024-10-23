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

// bmqp_storageeventbuilder.h                                         -*-C++-*-
#ifndef INCLUDED_BMQP_STORAGEEVENTBUILDER
#define INCLUDED_BMQP_STORAGEEVENTBUILDER

//@PURPOSE: Provide a mechanism to build a BlazingMQ 'STORAGE' event.
//
//@CLASSES:
//  bmqp::StorageEventBuilder: mechanism to build a BlazingMQ STORAGE event.
//
//@DESCRIPTION: 'bmqp::StorageEventBuilder' provides a mechanism to build a
// 'StorageEvent'. Such event starts by an 'EventHeader', followed by one or
// many repetitions of a pair of 'StorageHeader' + message payload.  A
// 'StorageEventBuilder' can be reused to build multiple Events, by calling the
// 'reset()' method on it.
//
/// Padding
///-------
// Note that messages added to the StorageEvent are *not* padded.  This is
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
//  bmqp::StorageEventBuilder builder(&bufferFactory, d_allocator_p);
//
//  // Append multiple messages
//  builder.packMessage(...);
//  builder.packMessage(...;
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

#include <bmqp_protocol.h>
#include <bmqt_messageguid.h>
#include <bmqt_resultcode.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_cpp11.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqp {

// =========================
// class StorageEventBuilder
// =========================

/// Mechanism to build a BlazingMQ STORAGE event
class StorageEventBuilder BSLS_CPP11_FINAL {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;

    bdlbb::BlobBufferFactory* d_bufferFactory_p;

    int d_storageProtocolVersion;
    // file storage protocol version

    EventType::Enum d_eventType;
    // Event type, either 'e_STORAGE' or 'e_PARTITION_SYNC'

    mutable bsl::shared_ptr<bdlbb::Blob> d_blob_sp;
    // blob being built by this PushEventBuilder.
    // This has been done mutable to be able to skip
    // writing the length until the blob is retrieved.

    int d_msgCount;
    // number of messages currently in the event

  private:
    // NOT IMPLEMENTED
    StorageEventBuilder(const StorageEventBuilder&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator not implemented
    StorageEventBuilder&
    operator=(const StorageEventBuilder&) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS
    bmqt::EventBuilderResult::Enum
    packMessageImp(StorageMessageType::Enum messageType,
                   unsigned int             partitionId,
                   int                      flags,
                   unsigned int             journalOffsetWords,
                   const bdlbb::BlobBuffer& journalRecordBuffer,
                   const bdlbb::BlobBuffer& payloadBuffer);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StorageEventBuilder,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new `StorageEventBuilder` having the specified `eventType`
    /// and using the specified `bufferFactory` and `allocator` for the
    /// blob, and operating with the specified `storageProtocolVersion`.
    /// Behavior is undefined unless `eventType` is `e_STORAGE` or
    /// `e_PARTITION_SYNC`.
    StorageEventBuilder(int                       storageProtocolVersion,
                        EventType::Enum           eventType,
                        bdlbb::BlobBufferFactory* bufferFactory,
                        bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Reset this builder to an initial state so that it can be used to
    /// build a new `StorageEvent`.  Note that calling reset invalidates the
    /// content of the blob returned by the `blob()` method.
    void reset();

    /// Add a message to the event being built, having the specified
    /// `messageType`, `partitionId`, `flags`, `journalOffsetWords`,
    /// `journalRecordBuffer` and optionally having `payloadBuffer`.  Return
    /// 0 on success, or a meaningful non-zero error code otherwise.  In
    /// case of failure, this method has no effect on the underlying event
    /// blob.  Behavior is undefined unless lengths of `journalRecordBuffer`
    /// and `payloadBuffer` are a multiple of `bmqp::Protocol::k_WORD_SIZE`.
    bmqt::EventBuilderResult::Enum
    packMessage(StorageMessageType::Enum messageType,
                unsigned int             partitionId,
                int                      flags,
                unsigned int             journalOffsetWords,
                const bdlbb::BlobBuffer& journalRecordBuffer);
    bmqt::EventBuilderResult::Enum
    packMessage(StorageMessageType::Enum messageType,
                unsigned int             partitionId,
                int                      flags,
                unsigned int             journalOffsetWords,
                const bdlbb::BlobBuffer& journalRecordBuffer,
                const bdlbb::BlobBuffer& payloadBuffer);

    /// Add the raw storage message of the specified `length` starting at
    /// the specified `startPos` in the specified `blob` to the event being
    /// built.  Return 0 on success, or a meaningful non-zero error code
    /// otherwise.  In case of failure, this method has no effect on the
    /// underlying event blob.  Behavior is undefined unless `length` is
    /// greater than zero.  Note that this method does *not* add a
    /// `bmqp::StorageHeader` before the storage message.
    bmqt::EventBuilderResult::Enum
    packMessageRaw(const bdlbb::Blob&        blob,
                   const mwcu::BlobPosition& startPos,
                   int                       length);

    // ACCESSORS

    /// Return the storage protocol version associated with this builder.
    int storageProtocolVersion() const;

    /// Return the type of event being built by this builder.  Note that
    /// returned value is one of `e_STORAGE` or `e_PARTITION_SYNC`.
    EventType::Enum eventType() const;

    /// Return the current size of the event being built.
    int eventSize() const;

    /// Return the number of messages currently in the event being built.
    int messageCount() const;

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return an empty
    /// blob, i.e., a blob with length == 0.
    const bdlbb::Blob& blob() const;

    /// Return a reference not offering modifiable access to the blob built
    /// by this event.  If no messages were added, this will return an empty
    /// blob, i.e., a blob with length == 0.
    bsl::shared_ptr<bdlbb::Blob> blob_sp() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class StorageEventBuilder
// -------------------------

// MANIPULATORS
inline bmqt::EventBuilderResult::Enum
StorageEventBuilder::packMessage(StorageMessageType::Enum messageType,
                                 unsigned int             partitionId,
                                 int                      flags,
                                 unsigned int             journalOffsetWords,
                                 const bdlbb::BlobBuffer& journalRecordBuffer)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(StorageMessageType::e_DATA != messageType &&
                     StorageMessageType::e_UNDEFINED != messageType);
    BSLS_ASSERT_SAFE(journalRecordBuffer.buffer());
    BSLS_ASSERT_SAFE(journalRecordBuffer.size());
    BSLS_ASSERT_SAFE(0 != journalOffsetWords);
    BSLS_ASSERT_SAFE(0 == journalRecordBuffer.size() % Protocol::k_WORD_SIZE);

    return packMessageImp(messageType,
                          partitionId,
                          flags,
                          journalOffsetWords,
                          journalRecordBuffer,
                          bdlbb::BlobBuffer());
}

inline bmqt::EventBuilderResult::Enum
StorageEventBuilder::packMessage(StorageMessageType::Enum messageType,
                                 unsigned int             partitionId,
                                 int                      flags,
                                 unsigned int             journalOffsetWords,
                                 const bdlbb::BlobBuffer& journalRecordBuffer,
                                 const bdlbb::BlobBuffer& payloadBuffer)

{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(StorageMessageType::e_DATA == messageType ||
                     StorageMessageType::e_QLIST == messageType);
    BSLS_ASSERT_SAFE(journalRecordBuffer.buffer());
    BSLS_ASSERT_SAFE(journalRecordBuffer.size());
    BSLS_ASSERT_SAFE(0 == journalRecordBuffer.size() % Protocol::k_WORD_SIZE);
    BSLS_ASSERT_SAFE(payloadBuffer.buffer());
    BSLS_ASSERT_SAFE(payloadBuffer.size());
    BSLS_ASSERT_SAFE(0 != journalOffsetWords);
    BSLS_ASSERT_SAFE(0 == payloadBuffer.size() % Protocol::k_WORD_SIZE);

    return packMessageImp(messageType,
                          partitionId,
                          flags,
                          journalOffsetWords,
                          journalRecordBuffer,
                          payloadBuffer);
}

// ACCESSORS
inline int StorageEventBuilder::storageProtocolVersion() const
{
    return d_storageProtocolVersion;
}

inline EventType::Enum StorageEventBuilder::eventType() const
{
    return d_eventType;
}

inline int StorageEventBuilder::eventSize() const
{
    return d_blob_sp->length();
}

inline int StorageEventBuilder::messageCount() const
{
    return d_msgCount;
}

}  // close package namespace
}  // close enterprise namespace

#endif
