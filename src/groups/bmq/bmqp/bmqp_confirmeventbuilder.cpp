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

// bmqp_confirmeventbuilder.cpp                                       -*-C++-*-
#include <bmqp_confirmeventbuilder.h>

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

// -------------------------
// class ConfirmEventBuilder
// -------------------------

ConfirmEventBuilder::ConfirmEventBuilder(
    bdlbb::BlobBufferFactory* bufferFactory,
    bslma::Allocator*         allocator)
: d_blob(bufferFactory, allocator)
, d_msgCount(0)
{
    reset();
}

void ConfirmEventBuilder::reset()
{
    d_blob.removeAll();

    d_msgCount = 0;

    // NOTE: Since ConfirmEventBuilder owns the blob and we just reset it, we
    //       have guarantee that buffer(0) will contain the entire headers
    //       (unless the bufferFactory has blobs of ridiculously small size,
    //       which we assert against after growing the blob).

    // Ensure blob has enough space for an EventHeader followed by a
    // ConfirmHeader Use placement new to create the object directly in the
    // blob buffer, while still calling it's constructor (to memset memory and
    // initialize some fields).
    d_blob.setLength(sizeof(EventHeader) + sizeof(ConfirmHeader));
    BSLS_ASSERT_SAFE(d_blob.numDataBuffers() == 1 &&
                     "The buffers allocated by the supplied bufferFactory "
                     "are too small");

    // EventHeader
    new (d_blob.buffer(0).data()) EventHeader(EventType::e_CONFIRM);

    // ConfirmHeader
    new (d_blob.buffer(0).data() + sizeof(EventHeader)) ConfirmHeader();
}

bmqt::EventBuilderResult::Enum
ConfirmEventBuilder::appendMessage(int                      queueId,
                                   int                      subQueueId,
                                   const bmqt::MessageGUID& guid)
{
    // Note that this method *must* return one of the
    // 'bmqt::EventBuilderResult::Enum' values because the return value is
    // directly exposed to the client.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageCount() ==
                                              maxMessageCount())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_EVENT_TOO_BIG;  // RETURN
    }

    // Resize the blob to have space for an 'ConfirmMessage' at the end ...
    bmqu::BlobPosition offset;
    bmqu::BlobUtil::reserve(&offset, &d_blob, sizeof(ConfirmMessage));

    bmqu::BlobObjectProxy<ConfirmMessage> confirmMessage(&d_blob,
                                                         offset,
                                                         false,  // no read
                                                         true);  // write mode
    // Make sure memory is reset
    bsl::memset(static_cast<void*>(confirmMessage.object()),
                0,
                sizeof(ConfirmMessage));
    (*confirmMessage)
        .setQueueId(queueId)
        .setSubQueueId(subQueueId)
        .setMessageGUID(guid);
    confirmMessage.reset();  // i.e., flush writing to blob..

    ++d_msgCount;

    return bmqt::EventBuilderResult::e_SUCCESS;
}

const bdlbb::Blob& ConfirmEventBuilder::blob() const
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
