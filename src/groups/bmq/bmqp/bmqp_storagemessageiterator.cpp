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

// bmqp_storagemessageiterator.cpp                                    -*-C++-*-
#include <bmqp_storagemessageiterator.h>

#include <bmqscm_version.h>
// BSL
#include <bsl_iostream.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// ----------------------------
// class StorageMessageIterator
// ----------------------------

void StorageMessageIterator::copyFrom(const StorageMessageIterator& src)
{
    d_blobIter      = src.d_blobIter;
    d_advanceLength = src.d_advanceLength;

    if (!src.d_header.isSet()) {
        d_header.reset();
    }
    else {
        d_header.reset(src.d_header.blob(),
                       src.d_header.position(),
                       src.d_header.length(),
                       true,
                       false);
    }
}

int StorageMessageIterator::next()
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_HAS_NEXT = 1  // There is another message after this one
        ,
        rc_AT_END = 0  // This is the last message
        ,
        rc_INVALID = -1  // The Iterator is in invalid state
        ,
        rc_NO_STORAGEHEADER = -2  // StorageHeader is missing or incomplete
        ,
        rc_NOT_ENOUGH_BYTES = -3  // The number of bytes in the blob is less
                                  // than the header size OR payload size
                                  // declared in the header
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID;  // RETURN
    }

    // Skip over the previous message, so that blobIter now points to the next
    // StorageHeader (if any)
    if (d_advanceLength != 0 && !d_blobIter.advance(d_advanceLength)) {
        return rc_AT_END;  // RETURN
    }

    // Read StorageHeader, supporting protocol evolution by reading as many
    // bytes as the header declares (and not as many as the size of the struct)
    d_header.reset(d_blobIter.blob(),
                   d_blobIter.position(),
                   -StorageHeader::k_MIN_HEADER_SIZE,
                   true,
                   false);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // We couldn't read a StorageHeader.. set the state of this iterator to
        // invalid
        d_advanceLength = -1;
        return rc_NO_STORAGEHEADER;  // RETURN
    }

    const int headerSize = d_header->headerWords() * Protocol::k_WORD_SIZE;

    d_header.resize(headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_advanceLength = -1;
        return rc_NO_STORAGEHEADER;  // RETURN
    }

    const int messageSize = d_header->messageWords() * Protocol::k_WORD_SIZE;

    // Validation: make sure blob has enough data as indicated by StorageHeader
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_blobIter.remaining() <
                                              messageSize)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
        d_advanceLength = -1;
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    // Update 'advanceLength' for next message
    d_advanceLength = messageSize;

    return rc_HAS_NEXT;
}

int StorageMessageIterator::reset(const bdlbb::Blob* blob,
                                  const EventHeader& eventHeader)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_EVENTHEADER = -1  // The blob contains only an event header
                                     // (maybe not event complete); i.e., there
                                     // are no messages in it
    };

    d_blobIter.reset(blob, mwcu::BlobPosition(), blob->length(), true);

    bool rc = d_blobIter.advance(eventHeader.headerWords() *
                                 Protocol::k_WORD_SIZE);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
        d_advanceLength = -1;
        return rc_INVALID_EVENTHEADER;  // RETURN
    }

    // Below code snippet is needed so that 'next()' works seamlessly during
    // first invocation too, by reading the header from current position in
    // blob
    d_advanceLength = 0;

    return rc_SUCCESS;
}

void StorageMessageIterator::reset(const bdlbb::Blob*            blob,
                                   const StorageMessageIterator& other)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    copyFrom(other);
    d_blobIter.reset(blob,
                     other.d_blobIter.position(),
                     other.d_blobIter.remaining(),
                     true);
}

void StorageMessageIterator::dumpBlob(bsl::ostream& stream) const
{
    static const int k_MAX_BYTES_DUMP = 128;

    // For now, print only the beginning of the blob.. we may later on print
    // also the bytes around the current position
    if (d_blobIter.blob()) {
        stream << mwcu::BlobStartHexDumper(d_blobIter.blob(),
                                           k_MAX_BYTES_DUMP);
    }
    else {
        stream << "/no blob/";
    }
}

}  // close package namespace
}  // close enterprise namespace
