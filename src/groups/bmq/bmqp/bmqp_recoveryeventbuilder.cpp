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

// bmqp_recoveryeventbuilder.cpp                                      -*-C++-*-
#include <bmqp_recoveryeventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>

#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlde_md5.h>
#include <bsl_cstring.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// --------------------------
// class RecoveryEventBuilder
// --------------------------

// CREATORS
RecoveryEventBuilder::RecoveryEventBuilder(BlobSpPool*       blobSpPool_p,
                                           bslma::Allocator* allocator)
: d_blobSpPool_p(blobSpPool_p)
, d_blob_sp(0, allocator)  // initialized in `reset()`
, d_msgCount(0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blobSpPool_p);

    reset();
}

// MANIPULATORS
void RecoveryEventBuilder::reset()
{
    d_blob_sp = d_blobSpPool_p->getObject();

    // The following prerequisite is necessary since we do `Blob::setLength`:
    BSLS_ASSERT_SAFE(
        NULL != d_blob_sp->factory() &&
        "Passed BlobSpPool must build Blobs with set BlobBufferFactory");

    d_msgCount = 0;

    // NOTE: Since RecoveryEventBuilder owns the blob and we just reset it, we
    //       have guarantee that buffer(0) will contain the entire header
    //       (unless the bufferFactory has blobs of ridiculously small size !!)

    // Ensure blob has enough space for an EventHeader
    //
    // Use placement new to create the object directly in the blob buffer,
    // while still calling it's constructor (to memset memory and initialize
    // some fields)
    d_blob_sp->setLength(sizeof(EventHeader));
    new (d_blob_sp->buffer(0).data()) EventHeader(EventType::e_RECOVERY);
}

bmqt::EventBuilderResult::Enum
RecoveryEventBuilder::packMessage(unsigned int                partitionId,
                                  RecoveryFileChunkType::Enum chunkFileType,
                                  unsigned int                sequenceNumber,
                                  const bdlbb::BlobBuffer&    chunkBuffer,
                                  bool                        isFinal,
                                  bool                        isSetMd5)
{
    // Validate payload is not too big (no padding needed for the chunk).  Note
    // that chunkBuffer could be zero length.

    unsigned int payloadLen = static_cast<unsigned int>(chunkBuffer.size());
    BSLS_ASSERT_SAFE(0 == payloadLen % Protocol::k_WORD_SIZE);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            payloadLen > RecoveryHeader::k_MAX_PAYLOAD_SIZE_SOFT)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG;  // RETURN
    }

    // Ensure event has enough space left for this message.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            EventHeader::k_MAX_SIZE_SOFT <
            static_cast<int>(eventSize() + sizeof(RecoveryHeader) +
                             payloadLen))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    // Add RecoveryHeader
    bmqu::BlobPosition offset;
    bmqu::BlobUtil::reserve(&offset, d_blob_sp.get(), sizeof(RecoveryHeader));

    bmqu::BlobObjectProxy<RecoveryHeader> recoveryHeader(d_blob_sp.get(),
                                                         offset,
                                                         false,  // no read
                                                         true);  // write mode
    // Make sure memory is reset
    bsl::memset(static_cast<void*>(recoveryHeader.object()),
                0,
                sizeof(bmqp::RecoveryHeader));

    int headerWords = sizeof(bmqp::RecoveryHeader) / Protocol::k_WORD_SIZE;
    (*recoveryHeader)
        .setHeaderWords(headerWords)
        .setMessageWords(headerWords + (payloadLen / Protocol::k_WORD_SIZE))
        .setFileChunkType(chunkFileType)
        .setPartitionId(partitionId)
        .setChunkSequenceNumber(sequenceNumber);

    if (0 != payloadLen && isSetMd5) {
        bdlde::Md5::Md5Digest md5Digest;
        bdlde::Md5 md5Hasher(chunkBuffer.data(), chunkBuffer.size());
        md5Hasher.loadDigest(&md5Digest);

        (*recoveryHeader).setMd5Digest(md5Digest.buffer());
    }

    if (isFinal) {
        recoveryHeader->setFinalChunkBit();
    }

    recoveryHeader.reset();  // i.e., flush writing to blob..

    if (0 != payloadLen) {
        // Per bdlbb::Blob contract, specifying a buffer of zero length in
        // 'appendDataBuffer' is undefined.
        d_blob_sp->appendDataBuffer(chunkBuffer);
    }

    ++d_msgCount;
    return bmqt::EventBuilderResult::e_SUCCESS;
}

const bdlbb::Blob& RecoveryEventBuilder::blob() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return ProtocolUtil::emptyBlob();  // RETURN
    }

    // Fix packet's length in header now that we know it ..  Following is valid
    // (see comment in reset)
    EventHeader& eh = *reinterpret_cast<EventHeader*>(
        d_blob_sp->buffer(0).data());
    eh.setLength(d_blob_sp->length());

    return *d_blob_sp;
}

bsl::shared_ptr<bdlbb::Blob> RecoveryEventBuilder::blob_sp() const
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bsl::shared_ptr<bdlbb::Blob>();  // RETURN
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
