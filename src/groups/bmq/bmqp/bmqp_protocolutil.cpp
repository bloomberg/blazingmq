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

// bmqp_protocolutil.cpp                                              -*-C++-*-
#include <bmqp_protocolutil.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_compression.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>

// MWC
#include <mwcc_array.h>
#include <mwcu_blobiterator.h>
#include <mwcu_blobobjectproxy.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_stringrefutil.h>
#include <bdlb_tokenizer.h>
#include <bdlbb_blobutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bslma_default.h>
#include <bslmt_qlock.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqp {

namespace {

/// Static buffer data used by the padding blob buffers and for the raw copy
const char k_PADDING_DATA[9][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {3, 3, 3, 3, 3, 3, 3, 3},
    {4, 4, 4, 4, 4, 4, 4, 4},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {6, 6, 6, 6, 6, 6, 6, 6},
    {7, 7, 7, 7, 7, 7, 7, 7},
    {8, 8, 8, 8, 8, 8, 8, 8},
};

/// Array of all potential padding buffers used for word and dword padding.
bsls::ObjectBuffer<bdlbb::BlobBuffer> g_paddingBlobBuffer[9];

bsls::ObjectBuffer<bdlbb::Blob> g_heartbeatReqBlob;
bsls::ObjectBuffer<bdlbb::Blob> g_heartbeatRspBlob;

/// Static prefilled blobs respectively containing a heartbeat request,
/// heartbeat response and an empty blob.
bsls::ObjectBuffer<bdlbb::Blob> g_emptyBlob;

/// Integer to keep track of the number of calls to `initialize` for the
/// `ProtocolUtil`.  If the value is non-zero, then it has already been
/// initialized, otherwise it can be initialized.  Each call to `initialize`
/// increments the value of this integer by one.  Each call to `shutdown`
/// decrements the value of this integer by one.  If the decremented value
/// is zero, then the objects held by `g_paddingBlobBuffer` are destroyed.
int g_initialized = 0;

/// Lock used to provide thread-safe protection for accessing the
/// `g_initialized` counter.
bslmt::QLock g_initLock = BSLMT_QLOCK_INITIALIZER;

/// Conversion table used to convert an int number to its hexadecimal
/// representation.
const char k_INT_TO_HEX_TABLE[16] = {'0',
                                     '1',
                                     '2',
                                     '3',
                                     '4',
                                     '5',
                                     '6',
                                     '7',
                                     '8',
                                     '9',
                                     'A',
                                     'B',
                                     'C',
                                     'D',
                                     'E',
                                     'F'};

/// Conversion table used to convert a hexadecimal value to it's int
/// representation. (99 is because in the ASCII table, `9` is 57 and `A` is
/// 65, so the 99 represents unexpected invalid value in the input).
const char k_HEX_TO_INT_TABLE[24] = {0,  1,  2,  3,  4,  5,  6,  7,
                                     8,  9,  99, 99, 99, 99, 99, 99,
                                     99, 10, 11, 12, 13, 14, 15, 99};
}  // close unnamed namespace

// -------------------
// struct ProtocolUtil
// -------------------

const char ProtocolUtil::k_NULL_APP_ID[] = "";

const char ProtocolUtil::k_DEFAULT_APP_ID[] = "__default";

void ProtocolUtil::initialize(bslma::Allocator* allocator)
{
    bslmt::QLockGuard qlockGuard(&g_initLock);

    // NOTE: We pre-increment here instead of post-incrementing inside the
    //       conditional check below because the post-increment of an int does
    //       not work correctly with versions of IBM xlc12 released following
    //       the 'Dec 2015 PTF'.
    ++g_initialized;
    if (g_initialized > 1) {
        return;  // RETURN
    }

    bslma::Allocator* alloc = bslma::Default::globalAllocator(allocator);

    // Prefill the padding blob buffers
    for (int i = 0; i < 9; ++i) {
        bsl::shared_ptr<char> data;
        data.reset(const_cast<char*>(k_PADDING_DATA[i]),
                   bslstl::SharedPtrNilDeleter(),
                   alloc);
        new (g_paddingBlobBuffer[i].buffer()) bdlbb::BlobBuffer(data, i);
    }

    // Prefill the heartbeat blobs
    {
        static const EventHeader header(EventType::e_HEARTBEAT_REQ);
        bsl::shared_ptr<char>    data;
        data.reset(static_cast<char*>(
                       const_cast<void*>(static_cast<const void*>(&header))),
                   bslstl::SharedPtrNilDeleter(),
                   alloc);
        bdlbb::BlobBuffer buffer(data, sizeof(header));
        new (g_heartbeatReqBlob.buffer()) bdlbb::Blob(alloc);
        g_heartbeatReqBlob.object().appendDataBuffer(buffer);
    }
    {
        static const EventHeader header(EventType::e_HEARTBEAT_RSP);
        bsl::shared_ptr<char>    data;
        data.reset(static_cast<char*>(
                       const_cast<void*>(static_cast<const void*>(&header))),
                   bslstl::SharedPtrNilDeleter(),
                   alloc);
        bdlbb::BlobBuffer buffer(data, sizeof(header));
        new (g_heartbeatRspBlob.buffer()) bdlbb::Blob(alloc);
        g_heartbeatRspBlob.object().appendDataBuffer(buffer);
    }

    // Create empty blob
    new (g_emptyBlob.buffer()) bdlbb::Blob(alloc);
}

void ProtocolUtil::shutdown()
{
    bslmt::QLockGuard qlockGuard(&g_initLock);

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized > 0 && "Not initialized");

    if (--g_initialized != 0) {
        return;  // RETURN
    }

    g_heartbeatRspBlob.object().bdlbb::Blob::~Blob();
    g_heartbeatReqBlob.object().bdlbb::Blob::~Blob();
    g_emptyBlob.object().bdlbb::Blob::~Blob();

    for (int i = 0; i < 9; ++i) {
        g_paddingBlobBuffer[i].object().reset();
    }
}

void ProtocolUtil::appendPaddingRaw(bdlbb::Blob* destination,
                                    int          numPaddingBytes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(g_initialized && "Not initialized");
    BSLS_ASSERT_SAFE(numPaddingBytes >= 1 && numPaddingBytes <= 8);
    BSLS_ASSERT_SAFE(destination->numDataBuffers() >= 1);
    // It doesn't make sense to add padding to an empty blob, and assuming at
    // least one buffer simplifies the logic below.

    const bdlbb::BlobBuffer& lastBuffer = destination->buffer(
        destination->numDataBuffers() - 1);
    const int lastBufferDataLength = destination->lastDataBufferLength();
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            lastBuffer.size() - lastBufferDataLength >= numPaddingBytes)) {
        // The last buffer has enough capacity left for the padding bytes,
        // simply expand the buffer's data length and copy the padding bytes.
        destination->setLength(destination->length() + numPaddingBytes);
        bsl::memcpy(lastBuffer.data() + lastBufferDataLength,
                    k_PADDING_DATA[numPaddingBytes],
                    numPaddingBytes);
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The last buffer doesn't have enough capacity, append the already
        // pre-formatted padding-bytes blob buffer.
        destination->appendDataBuffer(
            g_paddingBlobBuffer[numPaddingBytes].object());
    }
}

void ProtocolUtil::appendPaddingRaw(char* destination, int numPaddingBytes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(numPaddingBytes >= 1 && numPaddingBytes <= 4);

    bsl::memcpy(destination, k_PADDING_DATA[numPaddingBytes], numPaddingBytes);
}

void ProtocolUtil::appendPaddingDwordRaw(char* destination,
                                         int   numPaddingBytes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(numPaddingBytes >= 1 && numPaddingBytes <= 8);

    bsl::memcpy(destination, k_PADDING_DATA[numPaddingBytes], numPaddingBytes);
}

int ProtocolUtil::calcUnpaddedLength(const bdlbb::Blob& blob, int length)
{
    BSLS_ASSERT_SAFE(length);

    bsl::pair<int, int> pos =
        bdlbb::BlobUtil::findBufferIndexAndOffset(blob, length - 1);

    BSLS_ASSERT(pos.first < blob.numDataBuffers());
    const bdlbb::BlobBuffer& buf = blob.buffer(pos.first);

    BSLS_ASSERT(pos.second < buf.size());

    return length - buf.data()[pos.second];
}

bool ProtocolUtil::isValidWordPaddingByte(char value)
{
    switch (value) {
    case 1:
    case 2:
    case 3:
    case 4: {
        return true;  // RETURN
    }
    default: {
        return false;  // RETURN
    }
    }
}

bool ProtocolUtil::isValidDWordPaddingByte(char value)
{
    switch (value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8: {
        return true;  // RETURN
    }
    default: {
        return false;  // RETURN
    }
    }
}

const bdlbb::Blob& ProtocolUtil::heartbeatReqBlob()
{
    return g_heartbeatReqBlob.object();
}

const bdlbb::Blob& ProtocolUtil::heartbeatRspBlob()
{
    return g_heartbeatRspBlob.object();
}

const bdlbb::Blob& ProtocolUtil::emptyBlob()
{
    return g_emptyBlob.object();
}

void ProtocolUtil::hexToBinary(char* buffer, int length, const char* hex)
{
    for (int i = 0; i < length; ++i) {
        const int index = 2 * i;
        const int ch1   = hex[index + 0] - '0';
        const int ch2   = hex[index + 1] - '0';

        buffer[i] = (k_HEX_TO_INT_TABLE[ch1] << 4) | (k_HEX_TO_INT_TABLE[ch2]);
    }
}

void ProtocolUtil::binaryToHex(char*       buffer,
                               const char* binary,
                               int         binaryBufferlength)
{
    for (int i = 0; i < binaryBufferlength; ++i) {
        const int           index = 2 * i;
        const unsigned char ch    = binary[i];

        buffer[index]     = k_INT_TO_HEX_TABLE[ch >> 4];
        buffer[index + 1] = k_INT_TO_HEX_TABLE[ch & 0xF];
    }
}

int ProtocolUtil::ackResultToCode(bmqt::AckResult::Enum value)
{
    switch (value) {
    case bmqt::AckResult::e_SUCCESS: {
        return 0;  // RETURN
    }
    case bmqt::AckResult::e_LIMIT_MESSAGES: {
        return 1;  // RETURN
    }
    case bmqt::AckResult::e_LIMIT_BYTES: {
        return 2;  // RETURN
    }
    case bmqt::AckResult::e_STORAGE_FAILURE: {
        return 6;  // RETURN
    }
    case bmqt::AckResult::e_NOT_READY: {
        return 7;  // RETURN
    }
    case bmqt::AckResult::e_UNKNOWN:
    case bmqt::AckResult::e_TIMEOUT:
    case bmqt::AckResult::e_NOT_CONNECTED:
    case bmqt::AckResult::e_CANCELED:
    case bmqt::AckResult::e_NOT_SUPPORTED:
    case bmqt::AckResult::e_REFUSED:
    case bmqt::AckResult::e_INVALID_ARGUMENT:
    default: {
        // Value of '5' is reserved.
        return 5;  // RETURN
    }
    }
}

bmqt::AckResult::Enum ProtocolUtil::ackResultFromCode(int value)
{
    switch (value) {
    case 0: {
        return bmqt::AckResult::e_SUCCESS;  // RETURN
    }
    case 1: {
        return bmqt::AckResult::e_LIMIT_MESSAGES;  // RETURN
    }
    case 2: {
        return bmqt::AckResult::e_LIMIT_BYTES;  // RETURN
    }
    case 6: {
        return bmqt::AckResult::e_STORAGE_FAILURE;  // RETURN
    }
    case 7: {
        return bmqt::AckResult::e_NOT_READY;  // RETURN
    }
    case 5:  // Value of '5' is reserved.
    default: {
        return bmqt::AckResult::e_UNKNOWN;  // RETURN
    }
    };
}

bool ProtocolUtil::loadFieldValues(bsl::vector<bsl::string>* fieldValues,
                                   const bsl::string&        fieldName,
                                   const bsl::string&        featureSet)
{
    // Tokenize "<fieldName1>:<val1>,<val2>,<val3>;<fieldName2>:<val4>" into:
    //     {"<fieldName1>:<val1>,<val2>,<val3>", "<fieldName2>:<val4>"}
    bdlb::Tokenizer fieldTokenIt(featureSet, ";");

    for (; fieldTokenIt.isValid(); ++fieldTokenIt) {
        const bslstl::StringRef fieldStr = fieldTokenIt.token();
        const bslstl::StringRef colonPos =
            bdlb::StringRefUtil::strstr(fieldStr, ":");
        // If there is a colon, at least one value must follow
        if (colonPos.end() == fieldStr.end()) {
            return false;  // RETURN
        }
        // Not having a colon indicates that the field has no specified value
        if (colonPos.empty()) {
            if (fieldStr != fieldName) {
                continue;  // CONTINUE
            }
            return true;  // RETURN
        }

        const bslstl::StringRef key(fieldStr.begin(), colonPos.begin());
        if (key != fieldName) {
            continue;  // CONTINUE
        }
        const bslstl::StringRef values(colonPos.end(), fieldStr.end());

        // Tokenize "<val1>,<val2>,<val3>" into:
        //     {"<val1>", "<val2>", "<val3>"}
        bdlb::Tokenizer valueTokenIt(values, ",");
        for (; valueTokenIt.isValid(); ++valueTokenIt) {
            fieldValues->push_back(valueTokenIt.token());
        }
        return true;  // RETURN
    }
    return false;
}

bool ProtocolUtil::hasFeature(const char*        fieldName,
                              const char*        featureName,
                              const bsl::string& featureSet)
{
    bsl::vector<bsl::string> features;
    return (loadFieldValues(&features, fieldName, featureSet) &&
            bsl::find(features.cbegin(), features.cend(), featureName) !=
                features.cend());
}

int ProtocolUtil::convertToOld(bdlbb::Blob*                         dst,
                               const bdlbb::Blob*                   src,
                               bmqt::CompressionAlgorithmType::Enum cat,
                               bdlbb::BlobBufferFactory*            factory,
                               bslma::Allocator*                    allocator)
{
    enum RcEnum {
        rc_SUCCESS                          = 0,
        rc_NO_MSG_PROPERTIES_HEADER         = -1,
        rc_INCOMPLETE_MSG_PROPERTIES_HEADER = -2,
        rc_INCORRECT_LENGTH                 = -3,
        rc_INVALID_MPH_SIZE                 = -4,
        rc_INVALID_NUM_PROPERTIES           = -5,
        rc_MISSING_MSG_PROPERTY_HEADERS     = -6,
        rc_NO_MSG_PROPERTY_HEADER           = -7,
        rc_INCOMPLETE_MSG_PROPERTY_HEADER   = -8,
        rc_INVALID_PROPERTY_TYPE            = -9,
        rc_INVALID_PROPERTY_NAME_LENGTH     = -10,
        rc_INVALID_PROPERTY_VALUE_LENGTH    = -11,
        rc_MISSING_PROPERTY_AREA            = -12,
        rc_PROPERTY_NAME_STREAMIN_FAILURE   = -13,
        rc_PROPERTY_VALUE_STREAMIN_FAILURE  = -14,
        rc_DUPLICATE_PROPERTY_NAME          = -15
    };

    BSLS_ASSERT_SAFE(src->length());

    // Specified 'blob' must be in the same format as the wire protocol.  So it
    // should start with a MessagePropertiesHeader followed by one or more
    // MessagePropertyHeader followed by one or more (name, value) pairs, and
    // end with padding for word alignment.

    mwcu::BlobIterator blobIter(src,
                                mwcu::BlobPosition(),
                                src->length(),
                                true);

    // Read 'MessagePropertiesHeader'.
    mwcu::BlobObjectProxy<MessagePropertiesHeader> srcHeader(
        src,
        -MessagePropertiesHeader::k_MIN_HEADER_SIZE,
        true,    // read flag
        false);  // write flag

    if (!srcHeader.isSet()) {
        return rc_NO_MSG_PROPERTIES_HEADER;  // RETURN
    }

    srcHeader.resize(srcHeader->headerSize());
    if (!srcHeader.isSet()) {
        return rc_INCOMPLETE_MSG_PROPERTIES_HEADER;  // RETURN
    }

    const int mphSize       = srcHeader->messagePropertyHeaderSize();
    int       numProps      = srcHeader->numProperties();
    int       advanceLength = srcHeader->headerSize();
    const int dataOffset    = advanceLength + numProps * mphSize;
    // start of names and values

    if (0 >= mphSize) {
        return rc_INVALID_MPH_SIZE;  // RETURN
    }

    if (blobIter.remaining() < dataOffset) {
        return rc_MISSING_MSG_PROPERTY_HEADERS;  // RETURN
    }
    const int mpsSize = srcHeader->messagePropertiesAreaWords() *
                        Protocol::k_WORD_SIZE;

    if (mpsSize > src->length()) {
        return rc_INCORRECT_LENGTH;  // RETURN
    }

    if (dst != src) {
        BSLS_ASSERT_SAFE(dst->length() == 0);
        dst->setLength(dataOffset);
        bdlbb::BlobUtil::copy(dst, 0, *src, 0, dataOffset);

        blobIter =
            mwcu::BlobIterator(dst, mwcu::BlobPosition(), dataOffset, true);
    }

    // Iterate over all 'MessagePropertyHeader' fields and keep track of
    // relevant values, to avoid a second pass over the blob.
    const int totalSize   = calcUnpaddedLength(*src, mpsSize);
    int       totalLength = dataOffset;
    int       previous    = 0;

    // need to keep two instances, current and previous
    mwcu::BlobObjectProxy<MessagePropertyHeader> mph[2];

    while (numProps--) {
        // Move the blob position to the beginning of
        // 'MessagePropertyHeader'.

        if (!blobIter.advance(advanceLength)) {
            // Failed to advance blob to next 'MessagePropertyHeader'
            return rc_NO_MSG_PROPERTY_HEADER;  // RETURN
        }
        int current = 1 - previous;  // switch to another instance

        mph[current].reset(dst,
                           blobIter.position(),
                           mphSize,
                           true,   // read flag
                           true);  // write flag

        if (!mph[current].isSet()) {
            return rc_INCOMPLETE_MSG_PROPERTY_HEADER;  // RETURN
        }

        // Keep track of property's info.
        int length  = 0;
        int nameLen = mph[current]->propertyNameLength();
        int offset  = mph[current]->propertyValueLength();

        // New style.  Calculate length as delta between offsets
        // 'offset' is to the property name
        if (!mph[previous].isSet()) {
            // The first property.
            if (offset) {
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            mph[current]->setPropertyValueLength(0);
        }
        else {
            // Calculate the previous length as delta between offsets.
            if (offset < (mph[previous]->propertyValueLength() +
                          mph[previous]->propertyNameLength())) {
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            length = offset - mph[previous]->propertyValueLength() -
                     mph[previous]->propertyNameLength();

            mph[previous]->setPropertyValueLength(length);
            totalLength += length;
        }
        if (numProps == 0) {
            // The last property.
            // Calculate the length as delta between the total and offset.
            if (totalSize < (dataOffset + offset + nameLen)) {
                return rc_INVALID_PROPERTY_VALUE_LENGTH;  // RETURN
            }
            length = totalSize - dataOffset - offset - nameLen;
            mph[current]->setPropertyValueLength(length);
            totalLength += length;
        }

        previous = current;
        // Update 'advanceLength' for the next iteration.
        advanceLength = mphSize;
        totalLength += nameLen;
    }

    if (totalSize != totalLength) {
        return rc_INCORRECT_LENGTH;  // RETURN
    }

    int rc = rc_SUCCESS;

    if (cat == bmqt::CompressionAlgorithmType::e_NONE) {
        if (dst != src && dst->length()) {
            bdlbb::BlobUtil::append(dst, *src, dst->length());
        }  // else there is no conversion and no schema; just use 'src'
    }
    else {
        BSLS_ASSERT_SAFE(dst != src);

        if (mpsSize >= src->length()) {
            return rc_INCORRECT_LENGTH;  // RETURN
        }

        if (dst->length() == 0) {
            // No modifications so far.  Append all MPs before de-compressing.
            bdlbb::BlobUtil::append(dst, *src, 0, mpsSize);
        }
        else {
            // MP headers have modified.  Append all the rest of MPs (data).
            bdlbb::BlobUtil::append(dst,
                                    *src,
                                    dst->length(),
                                    mpsSize - dst->length());
        }
        bdlbb::Blob compressed(factory, allocator);

        bdlbb::BlobUtil::append(&compressed, *src, mpsSize);

        mwcu::MemOutStream error(allocator);
        rc = bmqp::Compression::decompress(dst,
                                           factory,
                                           cat,
                                           compressed,
                                           &error,
                                           allocator);
    }

    return rc;
}

int ProtocolUtil::readPropertiesSize(int*                      size,
                                     const bdlbb::Blob&        blob,
                                     const mwcu::BlobPosition& position)
{
    enum RcEnum {
        // Return codes
        rc_OK                               = 0,
        rc_INCOMPLETE_MSG_PROPERTIES_HEADER = -1,
        rc_CORRUPT_MESSAGE                  = -2
    };
    mwcu::BlobObjectProxy<MessagePropertiesHeader> mpsHeader(
        &blob,
        position,
        -MessagePropertiesHeader::k_MIN_HEADER_SIZE,
        true,    // read flag
        false);  // write flag

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!mpsHeader.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INCOMPLETE_MSG_PROPERTIES_HEADER;  // RETURN
    }

    const int mpsHeaderSize = mpsHeader->headerSize();

    mpsHeader.resize(mpsHeaderSize);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!mpsHeader.isSet())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INCOMPLETE_MSG_PROPERTIES_HEADER;  // RETURN
    }

    *size = mpsHeader->messagePropertiesAreaWords() *
            bmqp::Protocol::k_WORD_SIZE;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(*size > blob.length())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // MessagePropertiesHeader indicates more bytes than in the blob
        return rc_CORRUPT_MESSAGE;  // RETURN
    }
    // Note that per contract, 'd_messagePropertiesSize' includes
    // padding length and message properties header.
    return rc_OK;
}

int ProtocolUtil::parse(bdlbb::Blob*              messagePropertiesOutput,
                        int*                      messagePropertiesSize,
                        bdlbb::Blob*              dataOutput,
                        const bdlbb::Blob&        input,
                        int                       length,
                        bool                      decompressFlag,
                        const mwcu::BlobPosition& position,
                        bool                      haveMessageProperties,
                        bool                      haveNewMessageProperties,
                        bmqt::CompressionAlgorithmType::Enum cat,
                        bdlbb::BlobBufferFactory*            blobBufferFactory,
                        bslma::Allocator*                    allocator)
{
    // This is to capture parsing and de-compressing MessageProperties in a
    // single place.

    enum RcEnum {
        // Return codes
        rc_COMPRESSED_DATA                    = 1,
        rc_OK                                 = 0,
        rc_INVALID_MSG_PROPERTIES_HEADER      = -1,
        rc_DECOMPRESSION_FAILURE              = -2,
        rc_INVALID_COMPRESSION_ALGORITHM_TYPE = -3
    };

    BSLS_ASSERT_SAFE(messagePropertiesSize);
    BSLS_ASSERT_SAFE(dataOutput);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            cat < bmqt::CompressionAlgorithmType::k_LOWEST_SUPPORTED_TYPE ||
            cat > bmqt::CompressionAlgorithmType::k_HIGHEST_SUPPORTED_TYPE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Unsupported compression algorithm type: possibly malformed message
        // or mismatch between broker/client library available compression
        // algorithms

        return rc_INVALID_COMPRESSION_ALGORITHM_TYPE;  // RETURN
    }

    int offset;
    int rc = mwcu::BlobUtil::positionToOffsetSafe(&offset, input, position);
    int copyFrom = offset;
    int copyLen  = length;

    BSLS_ASSERT_SAFE(rc == 0);

    // First, try to read messagePropertiesSize.
    *messagePropertiesSize = 0;

    if (haveMessageProperties) {
        // There are MPs, in either way.

        if (haveNewMessageProperties ||
            cat == bmqt::CompressionAlgorithmType::e_NONE) {
            // Can read not-compressed MPs
            rc = bmqp::ProtocolUtil::readPropertiesSize(messagePropertiesSize,
                                                        input,
                                                        position);
            if (rc < 0) {
                return rc * 10 + rc_INVALID_MSG_PROPERTIES_HEADER;  // RETURN
            }
            if (messagePropertiesOutput) {
                bdlbb::BlobUtil::append(messagePropertiesOutput,
                                        input,
                                        offset,
                                        *messagePropertiesSize);
                copyFrom += *messagePropertiesSize;
                copyLen -= *messagePropertiesSize;
            }  // else don't rush appending to 'dataOutput'.
        }      // else MPs are compressed
    }

    if (cat == bmqt::CompressionAlgorithmType::e_NONE) {
        bdlbb::BlobUtil::append(dataOutput, input, copyFrom, copyLen);

        return rc_OK;  // RETURN
    }

    if (!decompressFlag) {
        return rc_COMPRESSED_DATA;  // RETURN
    }

    // De-compress, if requested
    bdlbb::Blob        bufferCompressed(blobBufferFactory, allocator);
    mwcu::MemOutStream error(allocator);
    bdlbb::Blob        bufferDecompressed(blobBufferFactory, allocator);
    bdlbb::Blob*       decompressedBlob;

    if (0 == messagePropertiesOutput) {
        if (haveNewMessageProperties) {
            bdlbb::BlobUtil::append(dataOutput,
                                    input,
                                    offset,
                                    *messagePropertiesSize);
        }
        decompressedBlob = dataOutput;
    }
    else {
        // Have to separate MPs and the data
        decompressedBlob = &bufferDecompressed;
    }

    // bmqp::Compression::decompress does not take offset.
    bdlbb::BlobUtil::append(&bufferCompressed,
                            input,
                            offset + *messagePropertiesSize,
                            length - *messagePropertiesSize);

    rc = bmqp::Compression::decompress(decompressedBlob,
                                       blobBufferFactory,
                                       cat,
                                       bufferCompressed,
                                       &error,
                                       allocator);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // This failure means that the compressed message is possibly corrupt.

        return rc * 10 + rc_DECOMPRESSION_FAILURE;  // RETURN
    }

    if (haveMessageProperties && !haveNewMessageProperties) {
        // Now read de-compressed MPs
        rc = bmqp::ProtocolUtil::readPropertiesSize(messagePropertiesSize,
                                                    *decompressedBlob,
                                                    mwcu::BlobPosition());
        if (rc < 0) {
            return rc * 10 + rc_INVALID_MSG_PROPERTIES_HEADER;  // RETURN
        }

        if (messagePropertiesOutput) {
            BSLS_ASSERT_SAFE(decompressedBlob == &bufferDecompressed);
            bdlbb::BlobUtil::append(messagePropertiesOutput,
                                    bufferDecompressed,
                                    0,
                                    *messagePropertiesSize);
            bdlbb::BlobUtil::append(dataOutput,
                                    bufferDecompressed,
                                    *messagePropertiesSize,
                                    bufferDecompressed.length() -
                                        *messagePropertiesSize);
        }  // else, de-compressed directly to the 'dataOutput'
    }
    else if (messagePropertiesOutput) {
        // have already appended message properties if any
        bdlbb::BlobUtil::append(dataOutput,
                                bufferDecompressed,
                                0,
                                bufferDecompressed.length());
    }  // else, de-compressed directly to the 'dataOutput'

    return rc_OK;
}

// static
bool ProtocolUtil::verify(const bmqp_ctrlmsg::ConsumerInfo& ci)
{
    if (ci.consumerPriority() == bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID) {
        return ci.consumerPriorityCount() == 0;
    }
    else {
        return ci.consumerPriorityCount() > 0;
    }
}

// static
bool ProtocolUtil::verify(const bmqp_ctrlmsg::StreamParameters& parameters)
{
    for (size_t i = 0; i < parameters.subscriptions().size(); ++i) {
        const bmqp_ctrlmsg::Subscription& s = parameters.subscriptions()[i];
        for (size_t n = 0; n < s.consumers().size(); ++n) {
            if (!verify(s.consumers()[n])) {
                return false;
            }
        }
    }
    return true;
}

}  // close package namespace
}  // close enterprise namespace
