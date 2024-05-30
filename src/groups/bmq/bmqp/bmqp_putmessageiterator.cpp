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

// bmqp_putmessageiterator.cpp                                        -*-C++-*-
#include <bmqp_putmessageiterator.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_crc32c.h>
#include <bmqp_messageproperties.h>
#include <bmqp_optionsview.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocolutil.h>

// MWC
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// ------------------------
// class PutMessageIterator
// ------------------------

// PRIVATE MANIPULATORS
void PutMessageIterator::copyFrom(const PutMessageIterator& src)
{
    d_blobIter                   = src.d_blobIter;
    d_applicationDataSize        = src.d_applicationDataSize;
    d_lazyMessagePayloadSize     = src.d_lazyMessagePayloadSize;
    d_lazyMessagePayloadPosition = src.d_lazyMessagePayloadPosition;
    d_messagePropertiesSize      = src.d_messagePropertiesSize;
    d_applicationDataPosition    = src.d_applicationDataPosition;
    d_advanceLength              = src.d_advanceLength;
    d_optionsSize                = src.d_optionsSize;
    d_optionsPosition            = src.d_optionsPosition;
    d_decompressFlag             = src.d_decompressFlag;
    d_applicationData            = src.d_applicationData;
    d_isDecompressingOldMPs      = src.d_isDecompressingOldMPs;
    d_header                     = src.d_header;

    d_optionsView.reset();
}

// ACCESSORS
void PutMessageIterator::initCachedOptionsView() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(hasOptions());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_optionsView.isNull())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Load options iterator
        int rc = loadOptionsView(&d_optionsView.makeValue());
        BSLS_ASSERT_SAFE(rc == 0);
        (void)rc;  // compiler happiness
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(!d_optionsView.isNull());
}

int PutMessageIterator::compressedApplicationDataSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    // Above, `msgLenPadded` is length of entire PUT message, including PUT
    // header, option headers, option data, compressed message properties
    // and message payload.
    const int msgLenPadded = d_header.messageWords() * Protocol::k_WORD_SIZE;

    mwcu::BlobPosition lastBytePos;
    int                rc = mwcu::BlobUtil::findOffsetSafe(&lastBytePos,
                                            *d_blobIter.blob(),
                                            d_blobIter.position(),
                                            msgLenPadded - 1);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // This should not happen, because we checked for 'isValid' (that
        // ensured the blob and the header were consistent).  But in case
        // BSLS_ASSERT is not enabled, 'gracefully' handle this error condition
        return -1;  // RETURN
    }

    char lastByte = d_blobIter.blob()
                        ->buffer(lastBytePos.buffer())
                        .data()[lastBytePos.byte()];

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !ProtocolUtil::isValidWordPaddingByte(lastByte))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // If the 'lastByte' is not a padding byte then message is malformed
        return -1;  // RETURN
    }

    const int appDataLenPadded = (d_header.messageWords() -
                                  d_header.optionsWords() -
                                  d_header.headerWords()) *
                                 Protocol::k_WORD_SIZE;

    return appDataLenPadded - lastByte;
}

// ACCESSORS
int PutMessageIterator::applicationDataSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (d_applicationDataSize == -1) {
        d_applicationDataSize = compressedApplicationDataSize();
    }
    return d_applicationDataSize;
}

int PutMessageIterator::loadApplicationDataPosition(
    mwcu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_applicationDataPosition != mwcu::BlobPosition());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
    };

    *position = d_applicationDataPosition;
    return rc_SUCCESS;
}

int PutMessageIterator::loadApplicationData(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_APP_DATA_OFFSET = -1  // The appData offset isn't within
                                         // range of the blob data
        ,
        rc_INVALID_APP_DATA_LENGTH = -2  // The appData size from the header
                                         // isn't with range of the blob data
    };

    if (d_applicationDataSize > -1) {
        mwcu::BlobUtil::appendToBlob(blob,
                                     d_applicationData,
                                     mwcu::BlobPosition());
        return rc_SUCCESS;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_applicationDataPosition != mwcu::BlobPosition());
    int rc = mwcu::BlobUtil::appendToBlob(blob,
                                          *d_blobIter.blob(),
                                          d_applicationDataPosition,
                                          applicationDataSize());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // This can only fail if the [appDataPos, appDataPos +
        // applicationDataSize()] doesn't fall within the d_blobIter.blob(),
        // which should not happen because we checked for 'isValid' (that
        // ensured the blob and the header were consistent). But in case
        // BSLS_ASSERT is not enabled, 'gracefully' handle this error condition
        return (rc * 10 + rc_INVALID_APP_DATA_LENGTH);  // RETURN
    }

    return rc_SUCCESS;
}

int PutMessageIterator::loadOptions(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_OPTIONS_DATA = -1  // The options size from the header isn't
                                      // valid
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasOptions())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_SUCCESS;  // RETURN
    }

    const int rc = mwcu::BlobUtil::appendToBlob(blob,
                                                *d_blobIter.blob(),
                                                d_optionsPosition,
                                                d_optionsSize);

    if (rc != 0) {
        return (rc * 10 + rc_INVALID_OPTIONS_DATA);  // RETURN
    }

    return rc_SUCCESS;
}

int PutMessageIterator::loadOptionsView(OptionsView* view) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return view->reset(d_blobIter.blob(), d_optionsPosition, d_optionsSize);
}

int PutMessageIterator::loadMessagePropertiesPosition(
    mwcu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum { rc_SUCCESS = 0, rc_NO_MSG_PROPERTIES = -1 };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasMessageProperties())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        *position = mwcu::BlobPosition();
        return rc_NO_MSG_PROPERTIES;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_applicationDataPosition != mwcu::BlobPosition());
    BSLS_ASSERT_SAFE(0 < d_messagePropertiesSize);

    *position = d_applicationDataPosition;
    return rc_SUCCESS;
}

int PutMessageIterator::loadMessageProperties(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag || d_isDecompressingOldMPs);

    enum RcEnum { rc_SUCCESS = 0, rc_INVALID_MSG_PROPS_OFFSET = -1 };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasMessageProperties())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        blob->removeAll();
        return rc_SUCCESS;  // RETURN
    }

    // Message properties are present.
    BSLS_ASSERT_SAFE(d_applicationDataPosition != mwcu::BlobPosition());
    BSLS_ASSERT_SAFE(0 < d_messagePropertiesSize);

    int rc = mwcu::BlobUtil::appendToBlob(blob,
                                          d_applicationData,
                                          mwcu::BlobPosition(),
                                          d_messagePropertiesSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return (rc * 10 + rc_INVALID_MSG_PROPS_OFFSET);  // RETURN
    }

    return rc_SUCCESS;
}

int PutMessageIterator::loadMessageProperties(
    MessageProperties* properties) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag || d_isDecompressingOldMPs);

    enum RcEnum {
        rc_SUCCESS          = 0,
        rc_BLOB_FAILURE     = -1,
        rc_STREAMIN_FAILURE = -2
    };

    if (!hasMessageProperties()) {
        // Clear out and return success.  'bmqa::Message' relies on this
        // contract.
        properties->clear();
        return rc_SUCCESS;  // RETURN
    }

    bdlbb::Blob propertiesBlob(d_allocator_p);
    int         rc = loadMessageProperties(&propertiesBlob);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 != rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_BLOB_FAILURE + 100 * rc;  // RETURN
    }

    rc = properties->streamIn(propertiesBlob,
                              MessagePropertiesInfo::hasSchema(d_header));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 != rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_STREAMIN_FAILURE + 100 * rc;  // RETURN
    }

    return rc_SUCCESS;
}

int PutMessageIterator::messagePayloadSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    if (d_lazyMessagePayloadSize != -1) {
        return d_lazyMessagePayloadSize;  // RETURN
    }

    d_lazyMessagePayloadSize = applicationDataSize() - messagePropertiesSize();
    return d_lazyMessagePayloadSize;
}

int PutMessageIterator::loadMessagePayloadPosition(
    mwcu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_INVALID_MSG_PROPS_OFFSET = -1,
        rc_INVALID_PAYLOAD_OFFSET   = -2
    };

    if (d_lazyMessagePayloadPosition != mwcu::BlobPosition()) {
        *position = d_lazyMessagePayloadPosition;
        return rc_SUCCESS;  // RETURN
    }

    int rc = mwcu::BlobUtil::findOffsetSafe(&d_lazyMessagePayloadPosition,
                                            d_applicationData,
                                            mwcu::BlobPosition(),
                                            d_messagePropertiesSize);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_lazyMessagePayloadPosition = mwcu::BlobPosition();
        return (rc * 10 + rc_INVALID_PAYLOAD_OFFSET);  // RETURN
    }

    *position = d_lazyMessagePayloadPosition;
    return rc_SUCCESS;
}

int PutMessageIterator::loadMessagePayload(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    enum RcEnum {
        rc_SUCCESS                = 0,
        rc_INVALID_PAYLOAD_OFFSET = -1,
        rc_INVALID_PAYLOAD_LENGTH = -2
    };

    if (d_lazyMessagePayloadPosition == mwcu::BlobPosition()) {
        // This could be because 'loadMessagePayloadPosition' wasn't called or
        // because it failed.  If its later, calling it will fail again, and an
        // appropriate error will be returned.

        mwcu::BlobPosition payloadPos;
        int                rc = loadMessagePayloadPosition(&payloadPos);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return rc * 10 + rc_INVALID_PAYLOAD_OFFSET;  // RETURN
        }

        BSLS_ASSERT_SAFE(d_lazyMessagePayloadPosition == payloadPos);
    }

    int rc = mwcu::BlobUtil::appendToBlob(blob,
                                          d_applicationData,
                                          d_lazyMessagePayloadPosition,
                                          messagePayloadSize());
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // This can only fail if the [payloadPos, payloadPos + msgPayloadSize]
        // doesn't fall within the d_blobIter.blob().
        return (rc * 10 + rc_INVALID_PAYLOAD_LENGTH);  // RETURN
    }

    return rc_SUCCESS;
}

bool PutMessageIterator::extractMsgGroupId(
    bmqp::Protocol::MsgGroupId* msgGroupId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(msgGroupId);
    BSLS_ASSERT_SAFE(isValid());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasOptions())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    // Load options view
    initCachedOptionsView();

    BSLS_ASSERT_SAFE(!d_optionsView.isNull());
    OptionsView& optionsView = d_optionsView.value();
    BSLS_ASSERT_SAFE(optionsView.isValid());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            optionsView.find(OptionType::e_MSG_GROUP_ID) ==
            optionsView.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return false;  // RETURN
    }

    // Load Group Id
    const int rc = optionsView.loadMsgGroupIdOption(msgGroupId);
    BSLS_ASSERT_SAFE(rc == 0);

    return (rc == 0);
}

// MANIPULATORS
int PutMessageIterator::next()
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_HAS_NEXT = 1  // There is another message
                         // after this one
        ,
        rc_AT_END = 0  // This is the last message
        ,
        rc_INVALID = -1  // The Iterator is an invalid
                         // state
        ,
        rc_NO_PUTHEADER = -2  // PutHeader is missing or
                              // incomplete
        ,
        rc_NOT_ENOUGH_BYTES = -3  // The number of bytes in the
                                  // blob is less than the
                                  // header size OR payload size
                                  // declared in the header
        ,
        rc_INVALID_APPLICATION_DATA_OFFSET = -4,
        rc_PARSING_ERROR                   = -5,
        rc_INVALID_OPTIONS_OFFSET          = -6,
        rc_INVALID_ADVANCE_LENGTH          = -7,
        rc_INVALID_APPLICATION_DATA_SIZE   = -8,
        rc_INVALID_MESSAGE_PROPERTIES      = -9
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID;  // RETURN
    }

    // Reset the cached payload size & position since we are moving to the next
    // message
    d_applicationDataSize        = -1;
    d_lazyMessagePayloadSize     = -1;
    d_lazyMessagePayloadPosition = mwcu::BlobPosition();
    d_messagePropertiesSize      = 0;
    d_applicationDataPosition    = mwcu::BlobPosition();
    d_optionsSize                = 0;
    d_optionsPosition            = mwcu::BlobPosition();
    d_optionsView.reset();
    d_applicationData.removeAll();

    if (d_advanceLength != 0 && !d_blobIter.advance(d_advanceLength)) {
        return rc_AT_END;  // RETURN
    }

    // Read PutHeader, supporting protocol evolution by reading as many bytes
    // as the header declares (and not as many as the size of the struct)
    mwcu::BlobObjectProxy<PutHeader> header(d_blobIter.blob(),
                                            d_blobIter.position(),
                                            -PutHeader::k_MIN_HEADER_SIZE,
                                            true,
                                            false);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // We couldn't read a PutHeader.. set the state of this iterator to
        // invalid
        d_advanceLength = -1;
        return rc_NO_PUTHEADER;  // RETURN
    }

    const int headerSize = header->headerWords() * Protocol::k_WORD_SIZE;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(headerSize <
                                              PutHeader::k_MIN_HEADER_SIZE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Header is declaring less bytes than expected, probably this is
        // because header is malformed
        d_advanceLength = -1;
        return rc_NO_PUTHEADER;  // RETURN
    }

    header.resize(headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_advanceLength = -1;
        return rc_NO_PUTHEADER;  // RETURN
    }

    d_header = *header;

    // Update 'advanceLength' for the next message.
    d_advanceLength = d_header.messageWords() * Protocol::k_WORD_SIZE;

    // Validation: must ensure that 'd_advanceLength > 0' after update, or
    // iteration process via 'next()' might be infinite
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_advanceLength == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
        d_advanceLength = -1;
        return rc_INVALID_ADVANCE_LENGTH;  // RETURN
    }

    // Validation: make sure blob has enough data as indicated by PutHeader
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_blobIter.remaining() <
                                              d_advanceLength)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
        d_advanceLength = -1;
        return rc_NOT_ENOUGH_BYTES;  // RETURN
    }

    const bool loadOptionsPosition = OptionUtil::loadOptionsPosition(
        &d_optionsSize,
        &d_optionsPosition,
        *d_blobIter.blob(),
        header.position(),
        d_header);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!loadOptionsPosition)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Will happen if we could not find position of options area or if the
        // range [optionsPosition, optionsPosition + optionsSize] does not fall
        // within the blob.
        d_advanceLength = -1;

        return rc_INVALID_OPTIONS_OFFSET;  // RETURN
    }

    // load the 'd_applicationDataPosition' instance variable with the start of
    // the application data offset.
    int dataOffset = (d_header.headerWords() + d_header.optionsWords()) *
                     bmqp::Protocol::k_WORD_SIZE;
    int rc = mwcu::BlobUtil::findOffsetSafe(&d_applicationDataPosition,
                                            *d_blobIter.blob(),
                                            header.position(),
                                            dataOffset);
    // 'd_applicationDataPosition' is 'sizeof(EventHeader) + dataOffset'
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_applicationDataPosition = mwcu::BlobPosition();
        d_advanceLength           = -1;
        return (rc * 10 + rc_INVALID_APPLICATION_DATA_OFFSET);  // RETURN
    }

    const int  flags = d_header.flags();
    const bool haveMPs =
        PutHeaderFlagUtil::isSet(flags, PutHeaderFlags::e_MESSAGE_PROPERTIES);
    const bool haveNewMPs = MessagePropertiesInfo::hasSchema(d_header);

    // Validation: if new message properties schema is set, the flag
    // e_MESSAGE_PROPERTIES must also be set.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!haveMPs && haveNewMPs)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_advanceLength = -1;
        return rc_INVALID_MESSAGE_PROPERTIES;  // RETURN
    }

    const int length = compressedApplicationDataSize();

    // Validation: 'length' is an unpadded application data size, with
    // substracted size of the padding bytes section.
    //: o Negative 'length' is a sign that packet is malformed.
    //: o Zero 'length' is not supported.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(length <= 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_advanceLength = -1;
        return rc_INVALID_APPLICATION_DATA_SIZE;  // RETURN
    }

    bool decompressFlag = d_decompressFlag;
    // 'd_decompressFlag' can be set explicitly to force decompression

    if (haveMPs && !haveNewMPs && d_isDecompressingOldMPs) {
        // Force decompression.  Note that MPs are encoded in the old style
        // (with lengths, not offsets).  That style does not work with schema.
        // So, leave the schema absence. The only effect is de-compressing.
        decompressFlag = true;
        d_header.setCompressionAlgorithmType(
            bmqt::CompressionAlgorithmType::e_NONE);
        // No need to write the header back to the blob since future events
        // will use 'header()' as their input (not the blob).
    }

    bmqt::CompressionAlgorithmType::Enum cat =
        d_header.compressionAlgorithmType();

    rc = ProtocolUtil::parse(0,  // do not separate MPs from data
                             &d_messagePropertiesSize,
                             &d_applicationData,
                             *d_blobIter.blob(),
                             length,
                             decompressFlag,
                             d_applicationDataPosition,
                             haveMPs,
                             haveNewMPs,
                             cat,
                             d_bufferFactory_p,
                             d_allocator_p);

    if (rc < 0) {
        d_applicationDataPosition = mwcu::BlobPosition();
        d_advanceLength           = -1;
        return rc * 100 + rc_PARSING_ERROR;  // RETURN
    }
    else if (rc == 0) {
        d_applicationDataSize = d_applicationData.length();

        if (haveMPs && !haveNewMPs && d_isDecompressingOldMPs) {
            // Force decompressed.  Need to recalculate CRC32

            d_header.setCrc32c(Crc32c::calculate(d_applicationData));

            // No need to write the header back to the blob since future events
            // will use 'header()' as their input (not the blob).
        }
    }  // else no data is available (compressed)

    return rc_HAS_NEXT;
}

int PutMessageIterator::reset(const bdlbb::Blob* blob,
                              const EventHeader& eventHeader,
                              bool               decompressFlag)
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

    clear();

    d_decompressFlag = decompressFlag;
    d_blobIter.reset(blob, mwcu::BlobPosition(), blob->length(), true);

    bool rc = d_blobIter.advance(eventHeader.headerWords() *
                                 Protocol::k_WORD_SIZE);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the iterator to invalid state
        d_advanceLength = -1;
        return rc_INVALID_EVENTHEADER;  // RETURN
    }

    // Below code snippet is needed so that 'next()' works seamlessly during
    // first invocation too, by reading the header from current position in
    // blob
    d_advanceLength = 0;

    return rc_SUCCESS;
}

int PutMessageIterator::reset(const bdlbb::Blob*        blob,
                              const PutMessageIterator& other)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(blob);

    copyFrom(other);
    d_blobIter.reset(blob,
                     other.d_blobIter.position(),
                     other.d_blobIter.remaining(),
                     true);
    return 0;
}

void PutMessageIterator::dumpBlob(bsl::ostream& stream)
{
    static const int k_MAX_BYTES_DUMP = 128;

    // For now, print only the beginning of the blob.. we may later on print
    // also the bytes around the current position
    if (d_blobIter.blob()) {
        stream << mwcu::BlobStartHexDumper(d_blobIter.blob(),
                                           k_MAX_BYTES_DUMP);
    }
    else {
        stream << "** no blob **";
    }
}

}  // close package namespace
}  // close enterprise namespace
