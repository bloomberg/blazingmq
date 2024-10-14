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

// bmqp_pushmessageiterator.cpp                                       -*-C++-*-
#include <bmqp_pushmessageiterator.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_messageproperties.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocolutil.h>

#include <bmqu_blobobjectproxy.h>
#include <bmqu_memoutstream.h>

// BSL
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// -------------------------
// class PushMessageIterator
// -------------------------

// PRIVATE MANIPULATORS
void PushMessageIterator::copyFrom(const PushMessageIterator& src)
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
    d_header                     = src.d_header;

    d_optionsView.reset();
}

void PushMessageIterator::initCachedOptionsView() const
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

    BSLS_ASSERT_SAFE(!d_optionsView.isNull());
}

// ACCESSORS
int PushMessageIterator::loadOptions(bdlbb::Blob* blob) const
{
    BSLS_ASSERT_SAFE(isValid());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasOptions())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return 0;  // RETURN
    }

    return bmqu::BlobUtil::appendToBlob(blob,
                                        *d_blobIter.blob(),
                                        d_optionsPosition,
                                        d_optionsSize);
}

int PushMessageIterator::compressedApplicationDataSize() const
{
    // Above, `msgLenPadded` is length of entire PUSH message, including
    // PUSH header, option headers, option data, compressed message
    // properties and message payload.
    const int msgLenPadded = d_header.messageWords() * Protocol::k_WORD_SIZE;

    bmqu::BlobPosition lastBytePos;
    int                rc = bmqu::BlobUtil::findOffsetSafe(&lastBytePos,
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

    const int appDataLenPadded = (d_header.messageWords() -
                                  d_header.optionsWords() -
                                  d_header.headerWords()) *
                                 Protocol::k_WORD_SIZE;

    return appDataLenPadded - lastByte;
}

// ACCESSORS
int PushMessageIterator::applicationDataSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    if (isApplicationDataImplicit()) {
        return 0;  // RETURN
    }

    if (!d_decompressFlag) {
        if (d_applicationDataSize == -1) {
            d_applicationDataSize = compressedApplicationDataSize();
        }
    }

    return d_applicationDataSize;
}

int PushMessageIterator::loadApplicationDataPosition(
    bmqu::BlobPosition* position) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(position);
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_applicationDataPosition != bmqu::BlobPosition());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_IMPLICIT_APP_DATA = -1  // App payload is implicit
    };

    if (isApplicationDataImplicit()) {
        return rc_IMPLICIT_APP_DATA;  // RETURN
    }

    *position = d_applicationDataPosition;
    return rc_SUCCESS;
}

int PushMessageIterator::loadApplicationData(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_IMPLICIT_APP_DATA = -1  // App payload is implicit
        ,
        rc_INVALID_APP_DATA_OFFSET = -2  // The appData offset isn't within
                                         // range of the blob data
        ,
        rc_INVALID_APP_DATA_LENGTH = -3  // The appData size from the header
                                         // isn't with range of the blob data
    };

    if (isApplicationDataImplicit()) {
        return rc_IMPLICIT_APP_DATA;  // RETURN
    }

    BSLS_ASSERT_SAFE(d_applicationDataPosition != bmqu::BlobPosition());

    if (d_decompressFlag) {
        bmqu::BlobUtil::appendToBlob(blob,
                                     d_applicationData,
                                     bmqu::BlobPosition());
        return rc_SUCCESS;  // RETURN
    }

    int rc = bmqu::BlobUtil::appendToBlob(blob,
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

int PushMessageIterator::loadMessageProperties(bdlbb::Blob* blob) const
{
    // NOTE: This method returns success even if there are no properties
    //       associated with the current message.  This *must* *not* be changed
    //       because 'bmqa::Message::loadProperties' directly relies on this
    //       behavior.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    enum RcEnum {
        rc_SUCCESS                  = 0,
        rc_IMPLICIT_APP_DATA        = -1,
        rc_INVALID_MSG_PROPS_OFFSET = -2
    };

    if (isApplicationDataImplicit()) {
        return rc_IMPLICIT_APP_DATA;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasMessageProperties())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        blob->removeAll();
        return rc_SUCCESS;  // RETURN
    }

    // Message properties are present.

    BSLS_ASSERT_SAFE(0 < d_messagePropertiesSize);

    int rc = bmqu::BlobUtil::appendToBlob(blob,
                                          d_applicationData,
                                          bmqu::BlobPosition(),
                                          d_messagePropertiesSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return (rc * 10 + rc_INVALID_MSG_PROPS_OFFSET);  // RETURN
    }

    return rc_SUCCESS;
}

int PushMessageIterator::loadMessageProperties(
    MessageProperties* properties) const
{
    // NOTE: This method returns success even if there are no properties
    //       associated with the current message.  This *must* *not* be changed
    //       because 'bmqa::Message::loadProperties' directly relies on this
    //       behavior.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    enum RcEnum {
        rc_SUCCESS           = 0,
        rc_IMPLICIT_APP_DATA = -1,
        rc_BLOB_FAILURE      = -2,
        rc_STREAMIN_FAILURE  = -3
    };

    if (isApplicationDataImplicit()) {
        return rc_IMPLICIT_APP_DATA;  // RETURN
    }

    if (!hasMessageProperties()) {
        // Clear out and return success.  'bmqa::Message' relies on this
        // contract.
        properties->clear();
        return rc_SUCCESS;  // RETURN
    }

    bdlbb::Blob propertiesBlob;
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

int PushMessageIterator::messagePayloadSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    if (d_lazyMessagePayloadSize != -1) {
        return d_lazyMessagePayloadSize;  // RETURN
    }

    // There is no need to check 'isApplicationDataImplicit' flag here, it is
    // simply deferred to 'applicationDataSize' and 'messagePropertiesSize'
    // routines which will both return zero if app payload is implicit.

    d_lazyMessagePayloadSize = applicationDataSize() - messagePropertiesSize();
    return d_lazyMessagePayloadSize;
}

int PushMessageIterator::loadMessagePayloadPosition() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS                  = 0,
        rc_IMPLICIT_APP_DATA        = -1,
        rc_INVALID_MSG_PROPS_OFFSET = -2,
        rc_INVALID_PAYLOAD_OFFSET   = -3
    };

    // Message properties are present.

    BSLS_ASSERT_SAFE(0 < d_messagePropertiesSize);

    int rc = bmqu::BlobUtil::findOffsetSafe(&d_lazyMessagePayloadPosition,
                                            d_applicationData,
                                            bmqu::BlobPosition(),
                                            d_messagePropertiesSize);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_lazyMessagePayloadPosition = bmqu::BlobPosition();
        return (rc * 10 + rc_INVALID_PAYLOAD_OFFSET);  // RETURN
    }

    return rc_SUCCESS;
}

int PushMessageIterator::loadMessagePayload(bdlbb::Blob* blob) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(d_decompressFlag);

    enum RcEnum {
        rc_SUCCESS                = 0,
        rc_IMPLICIT_APP_DATA      = -1,
        rc_INVALID_PAYLOAD_OFFSET = -2,
        rc_INVALID_PAYLOAD_LENGTH = -3
    };

    if (isApplicationDataImplicit()) {
        return rc_IMPLICIT_APP_DATA;  // RETURN
    }

    if (hasMessageProperties() &&
        (d_lazyMessagePayloadPosition == bmqu::BlobPosition())) {
        // This could be because 'loadMessagePayloadPosition' wasn't called or
        // because it failed.  If its later, calling it will fail again, and an
        // appropriate error will be returned.

        int rc = loadMessagePayloadPosition();
        if (0 != rc) {
            return rc * 10 + rc_INVALID_PAYLOAD_OFFSET;  // RETURN
        }

        BSLS_ASSERT_SAFE(d_lazyMessagePayloadPosition != bmqu::BlobPosition());
    }

    int rc = bmqu::BlobUtil::appendToBlob(blob,
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

void PushMessageIterator::extractQueueInfo(int*          queueId,
                                           unsigned int* subscriptionId,
                                           RdaInfo*      rdaInfo) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueId);
    BSLS_ASSERT_SAFE(subscriptionId);
    BSLS_ASSERT_SAFE(rdaInfo);
    BSLS_ASSERT_SAFE(isValid());

    *queueId        = header().queueId();
    *subscriptionId = bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;

    if (!hasOptions()) {
        *rdaInfo = RdaInfo();
        return;  // RETURN
    }

    // Load options view
    initCachedOptionsView();

    BSLS_ASSERT_SAFE(!d_optionsView.isNull());
    OptionsView& optionsView = d_optionsView.value();
    BSLS_ASSERT_SAFE(optionsView.isValid());

    if (optionsView.find(OptionType::e_SUB_QUEUE_INFOS) == optionsView.end() &&
        optionsView.find(OptionType::e_SUB_QUEUE_IDS_OLD) ==
            optionsView.end()) {
        *rdaInfo = RdaInfo();
        return;  // RETURN
    }

    // Load SubQueueInfos
    Protocol::SubQueueInfosArray subQueueInfos;

    int rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
    BSLS_ASSERT_SAFE(rc == 0);
    BSLS_ASSERT_SAFE(subQueueInfos.size() == 1);

    *subscriptionId = subQueueInfos[0].id();
    *rdaInfo        = RdaInfo(subQueueInfos[0].rdaInfo());

    (void)rc;  // Compiler happiness
}

void PushMessageIterator::extractMsgGroupId(
    bmqp::Protocol::MsgGroupId* msgGroupId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(msgGroupId);
    BSLS_ASSERT_SAFE(isValid());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasOptions())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
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
        return;  // RETURN
    }

    // Load Group Id
    int rc = optionsView.loadMsgGroupIdOption(msgGroupId);
    BSLS_ASSERT_SAFE(rc == 0);

    (void)rc;  // Compiler happiness
}

// MANIPULATORS

int PushMessageIterator::next()
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
        rc_NO_PUSH_HEADER = -2  // PushHeader is missing or
                                // incomplete
        ,
        rc_NOT_ENOUGH_BYTES = -3  // The number of bytes in the
                                  // blob is less than the
                                  // header size OR payload size
                                  // OR options size declared in
                                  // the header
        ,
        rc_INVALID_APPLICATION_DATA_OFFSET = -4,
        rc_PARSING_ERROR                   = -5,
        rc_INVALID_OPTIONS_OFFSET          = -6,
        rc_INVALID_MESSAGE_SIZE            = -7
    };

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID;  // RETURN
    }

    // Reset the cached payload size & position since we are moving to the next
    // message
    d_applicationDataSize        = -1;
    d_lazyMessagePayloadSize     = -1;
    d_lazyMessagePayloadPosition = bmqu::BlobPosition();
    d_messagePropertiesSize      = 0;
    d_applicationDataPosition    = bmqu::BlobPosition();
    d_optionsSize                = 0;
    d_optionsPosition            = bmqu::BlobPosition();
    d_optionsView.reset();
    d_applicationData.removeAll();

    if (d_advanceLength != 0 && !d_blobIter.advance(d_advanceLength)) {
        return rc_AT_END;  // RETURN
    }

    d_advanceLength = -1;

    // Read PushHeader, supporting protocol evolution by reading as many bytes
    // as the header declares (and not as many as the size of the struct)
    bmqu::BlobObjectProxy<PushHeader> header(d_blobIter.blob(),
                                             d_blobIter.position(),
                                             -PushHeader::k_MIN_HEADER_SIZE,
                                             true,
                                             false);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // We couldn't read a PushHeader.. set the state of this iterator to
        // invalid
        return rc_NO_PUSH_HEADER;  // RETURN
    }

    const int headerSize = header->headerWords() * Protocol::k_WORD_SIZE;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(headerSize <
                                              PushHeader::k_MIN_HEADER_SIZE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Header is declaring less bytes than expected, probably this is
        // because header is malformed
        return rc_NO_PUSH_HEADER;  // RETURN
    }

    header.resize(headerSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!header.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_NO_PUSH_HEADER;  // RETURN
    }

    d_header              = *header;
    const int messageSize = d_header.messageWords() * Protocol::k_WORD_SIZE;
    // Validation: must ensure that 'messageSize > 0', or iteration process
    // via 'next()' might be infinite
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(messageSize == 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
        return rc_INVALID_MESSAGE_SIZE;  // RETURN
    }

    // Validation: make sure blob has enough data as indicated by PushHeader
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_blobIter.remaining() <
                                              messageSize)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Set the state of this iterator to invalid
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
        return rc_INVALID_OPTIONS_OFFSET;  // RETURN
    }

    // load the 'd_applicationDataPosition' instance variable with the start of
    // the application data offset.

    int rc = bmqu::BlobUtil::findOffsetSafe(
        &d_applicationDataPosition,
        *d_blobIter.blob(),
        header.position(),
        (d_header.headerWords() + d_header.optionsWords()) *
            bmqp::Protocol::k_WORD_SIZE);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        d_applicationDataPosition = bmqu::BlobPosition();
        return (rc * 10 + rc_INVALID_APPLICATION_DATA_OFFSET);  // RETURN
    }

    if (PushHeaderFlagUtil::isSet(d_header.flags(),
                                  PushHeaderFlags::e_IMPLICIT_PAYLOAD)) {
        // Entire application data payload is implicit. Nothing more to
        // validate.  Update 'advanceLength' for next message.
        d_advanceLength = messageSize;
        return rc_HAS_NEXT;  // RETURN
    }

    const bool haveMPs = PushHeaderFlagUtil::isSet(
        d_header.flags(),
        PushHeaderFlags::e_MESSAGE_PROPERTIES);
    const bool haveNewMPs = MessagePropertiesInfo::hasSchema(d_header);
    int        length     = compressedApplicationDataSize();

    rc = ProtocolUtil::parse(0,  // do not separate MPs from data
                             &d_messagePropertiesSize,
                             &d_applicationData,
                             *d_blobIter.blob(),
                             length,
                             d_decompressFlag,
                             d_applicationDataPosition,
                             haveMPs,
                             haveNewMPs,
                             d_header.compressionAlgorithmType(),
                             d_bufferFactory_p,
                             d_allocator_p);

    if (rc < 0) {
        return rc * 100 + rc_PARSING_ERROR;  // RETURN
    }
    else if (rc == 0) {
        d_applicationDataSize = d_applicationData.length();
    }  // else no data is available (compressed)

    d_advanceLength = messageSize;

    // Do not 'd_header.setCompressionAlgorithmType(
    // bmqt::CompressionAlgorithmType::e_NONE) since callers of
    // 'PushMessageIterator::header()' expect to see the original header.
    // The callers of 'PushMessageIterator::PushMessageIterator' requesting
    // decompression ('d_decompressFlag == true'), are responsible of
    // recognizing the change of CompressionAlgorithmType.

    return rc_HAS_NEXT;
}

int PushMessageIterator::reset(const bdlbb::Blob* blob,
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
    d_blobIter.reset(blob, bmqu::BlobPosition(), blob->length(), true);

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

int PushMessageIterator::reset(const bdlbb::Blob*         blob,
                               const PushMessageIterator& other)
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

void PushMessageIterator::dumpBlob(bsl::ostream& stream)
{
    static const int k_MAX_BYTES_DUMP = 128;

    // For now, print only the beginning of the blob.. we may later on print
    // also the bytes around the current position
    if (d_blobIter.blob()) {
        stream << bmqu::BlobStartHexDumper(d_blobIter.blob(),
                                           k_MAX_BYTES_DUMP);
    }
    else {
        stream << "** no blob **";
    }
}

}  // close package namespace
}  // close enterprise namespace
