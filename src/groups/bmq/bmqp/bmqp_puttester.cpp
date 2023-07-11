// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_puttester.cpp                                                 -*-C++-*-
#include <bmqp_puttester.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocol.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// ----------------------
// struct PutTester::Data
// ----------------------
// CREATORS
PutTester::Data::Data(bdlbb::BlobBufferFactory* bufferFactory,
                      bslma::Allocator*         allocator)
: d_payload(bufferFactory, allocator)
, d_properties(bufferFactory, allocator)
, d_appData(bufferFactory, allocator)
, d_msgGroupId(allocator)
, d_queueId(-1)
, d_corrId(-1)
, d_msgLen(-1)
, d_propLen(-1)
, d_compressedAppData(bufferFactory, allocator)
{
    // NOTHING
}

PutTester::Data::Data(const Data& other, bslma::Allocator* allocator)
: d_payload(other.d_payload, allocator)
, d_properties(other.d_properties, allocator)
, d_appData(other.d_appData, allocator)
, d_msgGroupId(other.d_msgGroupId, allocator)
, d_queueId(other.d_queueId)
, d_corrId(other.d_corrId)
, d_msgLen(other.d_msgLen)
, d_propLen(other.d_propLen)
, d_compressedAppData(other.d_compressedAppData, allocator)
{
    // NOTHING
}

// ----------------
// struct PutTester
// ----------------

// CLASS METHODS
void PutTester::populateBlob(bdlbb::Blob*              blob,
                             bmqp::EventHeader*        eh,
                             bsl::vector<Data>*        vec,
                             size_t                    numMsgs,
                             bdlbb::BlobBufferFactory* bufferFactory,
                             bool                      zeroLengthMsgs,
                             bslma::Allocator*         allocator,
                             bmqt::CompressionAlgorithmType::Enum cat)
{
    int seed        = bsl::numeric_limits<int>::max();
    int eventLength = sizeof(bmqp::EventHeader);

    eh->setType(bmqp::EventType::e_PUT);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    for (size_t i = 0; i < numMsgs; ++i) {
        Data        data(bufferFactory, allocator);
        bdlbb::Blob properties(bufferFactory, allocator);
        bdlbb::Blob payload(bufferFactory, allocator);
        int         blobSize     = bdlb::Random::generate15(&seed);
        int         propAreaSize = blobSize / 2 +
                           sizeof(bmqp::MessagePropertiesHeader);

        // Append properties.  Must have a valid
        // 'bmqp::MessagePropertiesHeader'.

        int padding = 0;
        int paddedPropsNumWords =
            bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding, propAreaSize);

        bmqp::MessagePropertiesHeader mph;
        mph.setHeaderSize(sizeof(mph));
        mph.setMessagePropertyHeaderSize(sizeof(bmqp::MessagePropertyHeader));
        mph.setNumProperties(12);  // This field doesn't matter
        mph.setMessagePropertiesAreaWords(paddedPropsNumWords);
        bdlbb::BlobUtil::append(&properties,
                                reinterpret_cast<const char*>(&mph),
                                sizeof(mph));

        bsl::vector<char> props(propAreaSize - sizeof(mph), 'x', allocator);
        bdlbb::BlobUtil::append(&properties, props.data(), props.size());
        bmqp::ProtocolUtil::appendPadding(&properties, propAreaSize);
        // Message properties area is word aligned.

        if (zeroLengthMsgs && 0 == i % 5) {
            // If zero-length msgs can be added, add one every 5th msg.
            blobSize = 0;
        }

        bsl::vector<char> blobData(blobSize, 'a', allocator);
        bdlbb::BlobUtil::append(&payload, blobData.data(), blobSize);

        padding = 0;
        int paddedPayloadNumWords =
            bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding, blobSize);

        data.d_payload    = payload;
        data.d_properties = properties;
        bdlbb::BlobUtil::append(&properties, payload);
        data.d_appData = properties;

        mwcu::MemOutStream error(allocator);
        int                paddedCompressedAppDataNumWords = -1;
        if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
            bdlbb::Blob compressed(bufferFactory, allocator);
            int         rc = bmqp::Compression::compress(&compressed,
                                                 bufferFactory,
                                                 cat,
                                                 data.d_appData,
                                                 &error,
                                                 allocator);
            BSLS_ASSERT_OPT(rc == 0);

            {
                // Sanity check for 'compressed'

                bdlbb::Blob dummy(bufferFactory, allocator);
                bmqp::Compression::decompress(&dummy,
                                              bufferFactory,
                                              cat,
                                              compressed,
                                              &error,
                                              allocator);
                int compare = bdlbb::BlobUtil::compare(dummy, data.d_appData);
                BSLS_ASSERT_SAFE(compare == 0);
            }

            padding                  = 0;
            data.d_compressedAppData = compressed;
            paddedCompressedAppDataNumWords =
                bmqp::ProtocolUtil::calcNumWordsAndPadding(
                    &padding,
                    data.d_compressedAppData.length());
            BSLS_ASSERT_SAFE(paddedCompressedAppDataNumWords > 0);
        }

        data.d_queueId = blobSize;
        data.d_corrId  = propAreaSize;
        data.d_msgLen  = blobSize;
        data.d_propLen = paddedPropsNumWords * bmqp::Protocol::k_WORD_SIZE;
        // Recall that properties' length retrieved from the iterator
        // includes padding.
        const bool hasMsgGroupId = (i % 3);
        if (hasMsgGroupId) {
            // Don't add Group Id, once every 3 iterations
            mwcu::MemOutStream oss(allocator);
            oss << "gid:" << i;
            data.d_msgGroupId.makeValue(oss.str());
        }
        const NullableMsgGroupId& msgGroupId = data.d_msgGroupId;

        typedef bmqp::OptionUtil::OptionMeta OptionMeta;
        const OptionMeta                     msgGroupIdOption =
            hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                bmqp::OptionType::e_MSG_GROUP_ID,
                                msgGroupId.value().length())
                                              : OptionMeta::forNullOption();

        const int optionsWords = hasMsgGroupId ? (msgGroupIdOption.size() /
                                                  bmqp::Protocol::k_WORD_SIZE)
                                               : 0;

        // PutHeader
        bmqp::PutHeader ph;
        ph.setOptionsWords(optionsWords);
        ph.setHeaderWords(sizeof(bmqp::PutHeader) /
                          bmqp::Protocol::k_WORD_SIZE);
        ph.setQueueId(data.d_queueId);
        ph.setCorrelationId(data.d_corrId);
        ph.setCompressionAlgorithmType(cat);
        if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
            BSLS_ASSERT_SAFE(paddedCompressedAppDataNumWords > 0);
            ph.setMessageWords(ph.headerWords() + optionsWords +
                               paddedCompressedAppDataNumWords);
        }
        else {
            ph.setMessageWords(ph.headerWords() + optionsWords +
                               paddedPropsNumWords + paddedPayloadNumWords);
        }

        // Set the 'e_ACK_REQUESTED' and 'e_MESSAGE_PROPERTIES' flags.

        int flags = 0;
        bmqp::PutHeaderFlagUtil::setFlag(
            &flags,
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        bmqp::PutHeaderFlagUtil::setFlag(
            &flags,
            bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);
        ph.setFlags(flags);

        eventLength += (ph.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&ph),
                                ph.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        if (hasMsgGroupId) {
            BSLS_ASSERT_SAFE(!msgGroupId.isNull());
            bmqp::OptionUtil::OptionsBox options;
            options.add(blob, msgGroupId.value().data(), msgGroupIdOption);
            eventLength += msgGroupIdOption.size();
        }

        if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
            bdlbb::BlobUtil::append(blob, data.d_compressedAppData);
            bmqp::ProtocolUtil::appendPadding(
                blob,
                data.d_compressedAppData.length());
        }
        else {
            bdlbb::BlobUtil::append(blob,
                                    reinterpret_cast<const char*>(&mph),
                                    sizeof(mph));
            bdlbb::BlobUtil::append(blob, props.data(), props.size());

            bmqp::ProtocolUtil::appendPadding(blob, propAreaSize);
            // Message properties area is word aligned.

            bdlbb::BlobUtil::append(blob, blobData.data(), blobSize);

            bmqp::ProtocolUtil::appendPadding(blob, blobSize);
        }
        vec->push_back(data);
    }

    // set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

void PutTester::populateBlob(bdlbb::Blob*             blob,
                             bmqp::EventHeader*       eh,
                             bdlbb::Blob*             eb,
                             int*                     ebLen,
                             mwcu::BlobPosition*      headerPosition,
                             mwcu::BlobPosition*      payloadPosition,
                             int                      queueId,
                             const bmqt::MessageGUID& messageGUID,
                             bmqt::CompressionAlgorithmType::Enum cat,
                             bdlbb::BlobBufferFactory* bufferFactory,
                             bslma::Allocator*         allocator)
{
    // Payload is 36 bytes.  Per BlazingMQ protocol, it will require 4 bytes of
    // padding (ie 1 word)
    const char* payload = "abcdefghijklmnopqrstuvwxyz1234567890";  // 36

    *ebLen = bsl::strlen(payload);

    bdlbb::BlobUtil::append(eb, payload, *ebLen);
    mwcu::MemOutStream error(allocator);
    bdlbb::Blob        compressedBlob(bufferFactory, allocator);
    if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
        int payloadLength = bsl::strlen(payload);
        bmqp::Compression::compress(&compressedBlob,
                                    bufferFactory,
                                    cat,
                                    payload,
                                    payloadLength,
                                    &error,
                                    allocator);
        int padding  = 0;
        int numWords = bmqp::ProtocolUtil::calcNumWordsAndPadding(
            &padding,
            compressedBlob.length());
        BSLS_ASSERT_SAFE(numWords >= 0);
        // Adding padding per BlazingMQ protocol
        bsl::string paddingCompressedBlob(padding, '\0');
        for (int index = 0; index < padding; index++) {
            paddingCompressedBlob[index] = padding;
        }
        bdlbb::BlobUtil::append(&compressedBlob,
                                paddingCompressedBlob.data(),
                                padding);
    }

    int payloadLenWords = bsl::strlen(payload) / bmqp::Protocol::k_WORD_SIZE;

    // 1 word of padding
    payloadLenWords += 1;
    if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
        payloadLenWords = compressedBlob.length() /
                          bmqp::Protocol::k_WORD_SIZE;
    }

    // PutHeader
    bmqp::PutHeader ph;
    ph.setFlags(0);
    ph.setOptionsWords(0);
    ph.setHeaderWords(sizeof(bmqp::PutHeader) / bmqp::Protocol::k_WORD_SIZE);
    ph.setQueueId(queueId);
    ph.setMessageGUID(messageGUID);
    ph.setMessageWords(ph.headerWords() + payloadLenWords);
    ph.setCompressionAlgorithmType(cat);

    // Set the 'e_ACK_REQUESTED' flag
    int flags = ph.flags();
    bmqp::PutHeaderFlagUtil::setFlag(&flags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);
    ph.setFlags(flags);

    int eventLength = sizeof(bmqp::EventHeader) +
                      ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

    // EventHeader
    eh->setLength(eventLength);
    eh->setType(bmqp::EventType::e_PUT);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Capture PutHeader position
    mwcu::BlobUtil::reserve(headerPosition,
                            blob,
                            ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    mwcu::BlobUtil::writeBytes(blob,
                               *headerPosition,
                               reinterpret_cast<const char*>(&ph),
                               ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    if (cat != bmqt::CompressionAlgorithmType::e_NONE) {
        // compressedBlob contains padding already so just append it to blob.

        const int payloadOffset = blob->length();
        bdlbb::BlobUtil::append(blob, compressedBlob);
        mwcu::BlobUtil::findOffset(payloadPosition, *blob, payloadOffset);
    }
    else {
        // Capture payload position
        mwcu::BlobUtil::reserve(payloadPosition, blob, bsl::strlen(payload));

        mwcu::BlobUtil::writeBytes(blob,
                                   *payloadPosition,
                                   payload,
                                   bsl::strlen(payload));

        // Adding padding per BlazingMQ protocol
        const char padding[] = {4, 4, 4, 4};
        bdlbb::BlobUtil::append(blob, padding, 4);
    }
}

void PutTester::populateBlob(bdlbb::Blob*                   blob,
                             bmqp::EventHeader*             eh,
                             bmqp::PutHeader*               ph,
                             int                            length,
                             int                            queueId,
                             const bmqt::MessageGUID&       msgGUID,
                             int                            correlationId,
                             const bmqp::MessageProperties& properties,
                             bool                           isAckRequested,
                             bdlbb::BlobBufferFactory*      bufferFactory,
                             bmqt::CompressionAlgorithmType::Enum cat,
                             bslma::Allocator*                    allocator)
{
    mwcu::BlobUtil::reserve(blob,
                            sizeof(bmqp::EventHeader) +
                                sizeof(bmqp::PutHeader));

    // PutHeader
    ph->setOptionsWords(0);
    ph->setHeaderWords(sizeof(bmqp::PutHeader) / bmqp::Protocol::k_WORD_SIZE);
    ph->setQueueId(queueId);

    // Set GUID if it is not empty.
    // Otherwise set correlation id.
    if (!msgGUID.isUnset()) {
        ph->setMessageGUID(msgGUID);
    }
    else {
        ph->setCorrelationId(correlationId);
    }

    ph->setCompressionAlgorithmType(cat);

    // Set the 'e_ACK_REQUESTED' flag if needed
    if (isAckRequested) {
        int flags = 0;
        bmqp::PutHeaderFlagUtil::setFlag(
            &flags,
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);
        ph->setFlags(flags);
    }

    // EventHeader
    eh->setType(bmqp::EventType::e_PUT);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);

    bdlbb::Blob  emptyBlob(bufferFactory, allocator);
    bdlbb::Blob& propertiesBlob = emptyBlob;

    if (properties.numProperties()) {
        // This method does not use PutBuilder and the format is old.
        const bmqp::MessagePropertiesInfo messagePropertiesInfo =
            bmqp::MessagePropertiesInfo::makeNoSchema();
        propertiesBlob = properties.streamOut(bufferFactory,
                                              messagePropertiesInfo);

        messagePropertiesInfo.applyTo(ph);
    }

    if (cat == bmqt::CompressionAlgorithmType::e_NONE) {
        bdlbb::BlobUtil::append(blob, propertiesBlob);
        populateBlob(blob, length);
    }
    else {
        bdlbb::Blob        dataBlob(bufferFactory, allocator);
        mwcu::MemOutStream error(allocator);

        bdlbb::BlobUtil::append(&dataBlob, propertiesBlob);
        populateBlob(&dataBlob, length);

        bmqp::Compression::compress(blob,
                                    bufferFactory,
                                    cat,
                                    dataBlob,
                                    &error,
                                    allocator);
    }
    const int payloadLenth = blob->length() - sizeof(bmqp::EventHeader) -
                             sizeof(bmqp::PutHeader);
    int       numPaddingBytes = 0;
    const int numWords        = bmqp::ProtocolUtil::calcNumWordsAndPadding(
        &numPaddingBytes,
        payloadLenth);

    // Add padding
    bmqp::ProtocolUtil::appendPaddingRaw(blob, numPaddingBytes);

    ph->setMessageWords(ph->headerWords() + numWords);

    eh->setLength(sizeof(bmqp::EventHeader) +
                  ph->messageWords() * bmqp::Protocol::k_WORD_SIZE);

    mwcu::BlobPosition pos;
    mwcu::BlobUtil::writeBytes(blob,
                               pos,
                               reinterpret_cast<const char*>(eh),
                               eh->headerWords() *
                                   bmqp::Protocol::k_WORD_SIZE);

    int rc = mwcu::BlobUtil::findOffsetSafe(&pos,
                                            *blob,
                                            mwcu::BlobPosition(),
                                            sizeof(bmqp::EventHeader));
    BSLS_ASSERT_SAFE(rc == 0);

    mwcu::BlobUtil::writeBytes(blob,
                               pos,
                               reinterpret_cast<const char*>(ph),
                               ph->headerWords() *
                                   bmqp::Protocol::k_WORD_SIZE);
}

void PutTester::populateBlob(bdlbb::Blob* blob, int atLeastLen)
{
    const char* k_FIXED_PAYLOAD =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdef";

    const int k_FIXED_PAYLOAD_LEN = bsl::strlen(k_FIXED_PAYLOAD);

    int numIters = atLeastLen / k_FIXED_PAYLOAD_LEN + 1;

    for (int i = 0; i < numIters; ++i) {
        bdlbb::BlobUtil::append(blob, k_FIXED_PAYLOAD, k_FIXED_PAYLOAD_LEN);
    }
}

}  // close package namespace
}  // close enterprise namespace
