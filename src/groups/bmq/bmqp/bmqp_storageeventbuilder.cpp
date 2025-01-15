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

// bmqp_storageeventbuilder.cpp                                       -*-C++-*-
#include <bmqp_storageeventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>

#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bsl_cstring.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// -------------------------
// class StorageEventBuilder
// -------------------------

// PRIVATE MANIPULATORS
bmqt::EventBuilderResult::Enum StorageEventBuilder::packMessageImp(
    StorageMessageType::Enum messageType,
    unsigned int             partitionId,
    int                      flags,
    unsigned int             journalOffsetWords,
    const bdlbb::BlobBuffer& journalRecordBuffer,
    const bdlbb::BlobBuffer& payloadBuffer)
{
    unsigned int totalLength = journalRecordBuffer.size() +
                               payloadBuffer.size();

    // No padding needed for storage messages

    // Validate payload is not too big
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            totalLength > StorageHeader::k_MAX_PAYLOAD_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG;  // RETURN
    }

    // Ensure event has enough space left for this message
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            EventHeader::k_MAX_SIZE_SOFT <
            static_cast<int>(eventSize() + sizeof(StorageHeader) +
                             totalLength))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    // Add the StorageHeader
    bmqu::BlobPosition offset;
    bmqu::BlobUtil::reserve(&offset, d_blob_sp.get(), sizeof(StorageHeader));

    bmqu::BlobObjectProxy<StorageHeader> storageHeader(d_blob_sp.get(),
                                                       offset,
                                                       false,  // no read
                                                       true);  // write mode
    // Make sure memory is reset
    bsl::memset(static_cast<void*>(storageHeader.object()),
                0,
                sizeof(bmqp::StorageHeader));

    int headerWords = sizeof(bmqp::StorageHeader) / Protocol::k_WORD_SIZE;
    (*storageHeader)
        .setHeaderWords(headerWords)
        .setFlags(flags)
        .setStorageProtocolVersion(d_storageProtocolVersion)
        .setMessageWords(headerWords + (totalLength / Protocol::k_WORD_SIZE))
        .setMessageType(messageType)
        .setPartitionId(partitionId)
        .setJournalOffsetWords(journalOffsetWords);

    storageHeader.reset();  // i.e., flush writing to blob..

    d_blob_sp->appendDataBuffer(journalRecordBuffer);

    if (StorageMessageType::e_DATA == messageType ||
        StorageMessageType::e_QLIST == messageType) {
        d_blob_sp->appendDataBuffer(payloadBuffer);
    }

    ++d_msgCount;
    return bmqt::EventBuilderResult::e_SUCCESS;
}

// CREATORS
StorageEventBuilder::StorageEventBuilder(int storageProtocolVersion,
                                         EventType::Enum   eventType,
                                         BlobSpPool*       blobSpPool_p,
                                         bslma::Allocator* allocator)
: d_blobSpPool_p(blobSpPool_p)
, d_storageProtocolVersion(storageProtocolVersion)
, d_eventType(eventType)
, d_blob_sp(0, allocator)       // initialized in `reset()`
, d_emptyBlob_sp(0, allocator)  // initialized later in constructor
, d_msgCount(0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blobSpPool_p);
    BSLS_ASSERT_SAFE(EventType::e_STORAGE == eventType ||
                     EventType::e_PARTITION_SYNC == eventType);

    d_emptyBlob_sp = blobSpPool_p->getObject();

    // Assume that items built with the given `blobSpPool_p` either all have or
    // all don't have buffer factory, and check it once for `d_emptyBlob_sp`.
    // We require this since we do `Blob::setLength`:
    BSLS_ASSERT_SAFE(
        NULL != d_emptyBlob_sp->factory() &&
        "Passed BlobSpPool must build Blobs with set BlobBufferFactory");

    reset();
}

// MANIPULATORS
void StorageEventBuilder::reset()
{
    d_blob_sp  = d_blobSpPool_p->getObject();
    d_msgCount = 0;

    // NOTE: Since StorageEventBuilder owns the blob and we just reset it, we
    //       have guarantee that buffer(0) will contain the entire header
    //       (unless the bufferFactory has blobs of ridiculously small size !!)

    // Ensure blob has enough space for an EventHeader
    //
    // Use placement new to create the object directly in the blob buffer,
    // while still calling it's constructor (to memset memory and initialize
    // some fields)
    d_blob_sp->setLength(sizeof(EventHeader));
    new (d_blob_sp->buffer(0).data()) EventHeader(d_eventType);
}

bmqt::EventBuilderResult::Enum
StorageEventBuilder::packMessageRaw(const bdlbb::Blob&        blob,
                                    const bmqu::BlobPosition& startPos,
                                    int                       length)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < length);

    if (0 == blob.length()) {
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !bmqu::BlobUtil::isValidPos(blob, startPos))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_UNKNOWN;  // RETURN
    }

    // Ensure event has enough space left for this raw message.  We don't
    // include the length of 'StorageHeader' because this is a raw message, and
    // it is assumed that it already contains it.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            EventHeader::k_MAX_SIZE_SOFT <
            static_cast<int>(eventSize() + blob.length()))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            0 != bmqu::BlobUtil::appendToBlob(d_blob_sp.get(),
                                              blob,
                                              startPos,
                                              length))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_UNKNOWN;  // RETURN
    }

    ++d_msgCount;
    return bmqt::EventBuilderResult::e_SUCCESS;
}

const bsl::shared_ptr<bdlbb::Blob>& StorageEventBuilder::blob() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_blob_sp->length() <= EventHeader::k_MAX_SIZE_SOFT);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return d_emptyBlob_sp;  // RETURN
    }

    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return d_blob_sp;
}

}  // close package namespace
}  // close enterprise namespace
