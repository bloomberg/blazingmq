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

// bmqp_ackeventbuilder.cpp                                           -*-C++-*-
#include <bmqp_ackeventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocolutil.h>

#include <bmqu_blob.h>
#include <bmqu_blobobjectproxy.h>

// BDE
#include <bsl_cstring.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// ---------------------
// class AckEventBuilder
// ---------------------

AckEventBuilder::AckEventBuilder(bdlbb::BlobBufferFactory* bufferFactory,
                                 bslma::Allocator*         allocator)
: d_blob(bufferFactory, allocator)
, d_msgCount(0)
{
    reset();
}

void AckEventBuilder::reset()
{
    d_blob.removeAll();
    d_msgCount = 0;

    // NOTE: Since AckEventBuilder owns the blob and we just reset it, we have
    //       guarantee that buffer(0) will contain the entire headers (unless
    //       the bufferFactory has blobs of ridiculously small size, which we
    //       assert against after growing the blob).

    // Ensure blob has enough space for an EventHeader followed by a AckHeader.
    // Use placement new to create the object directly in the blob buffer,
    // while still calling it's constructor (to memset memory and initialize
    // some fields).
    d_blob.setLength(sizeof(EventHeader) + sizeof(AckHeader));
    BSLS_ASSERT_SAFE(d_blob.numDataBuffers() == 1 &&
                     "The buffers allocated by the supplied bufferFactory "
                     "are too small");

    // EventHeader
    new (d_blob.buffer(0).data()) EventHeader(EventType::e_ACK);

    // AckHeader
    new (d_blob.buffer(0).data() + sizeof(EventHeader)) AckHeader();
}

bmqt::EventBuilderResult::Enum
AckEventBuilder::appendMessage(int                      status,
                               int                      correlationId,
                               const bmqt::MessageGUID& guid,
                               int                      queueId)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() ==
                                              maxMessageCount())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    // Resize the blob to have space for an 'AckMessage' at the end ...
    bmqu::BlobPosition offset;
    bmqu::BlobUtil::reserve(&offset, &d_blob, sizeof(AckMessage));

    bmqu::BlobObjectProxy<AckMessage> ackMessage(&d_blob,
                                                 offset,
                                                 false,  // no read
                                                 true);  // write mode
    // Make sure memory is reset
    bsl::memset(static_cast<void*>(ackMessage.object()),
                0,
                sizeof(AckMessage));
    (*ackMessage)
        .setStatus(status)
        .setCorrelationId(correlationId)
        .setMessageGUID(guid)
        .setQueueId(queueId);
    ackMessage.reset();  // i.e., flush writing to blob.

    ++d_msgCount;

    return bmqt::EventBuilderResult::e_SUCCESS;
}

const bdlbb::Blob& AckEventBuilder::blob() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_blob.length() <= EventHeader::k_MAX_SIZE_SOFT);

    // Empty event
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return ProtocolUtil::emptyBlob();  // RETURN
    }

    // Fix packet's length in header now that we know it.  Following is valid
    // (see comment in reset).
    EventHeader& eh = *reinterpret_cast<EventHeader*>(d_blob.buffer(0).data());
    eh.setLength(d_blob.length());

    return d_blob;
}

}  // close package namespace
}  // close enterprise namespace
