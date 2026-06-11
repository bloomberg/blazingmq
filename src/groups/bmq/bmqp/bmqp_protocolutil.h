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

#ifndef INCLUDED_BMQP_PROTOCOLUTIL
#define INCLUDED_BMQP_PROTOCOLUTIL

//@PURPOSE: Provide utilities for BlazingMQ protocol builders and iterators.
//
//@CLASSES:
//  bmqp::ProtocolUtil: Utilities for BlazingMQ protocol builders and iterators
//
//@DESCRIPTION: 'bmqp::ProtocolUtil' provides a set of utility methods to be
// used by the BlazingMQ protocol builders and iterators.  Such utilities are
// organized in three distinct categories: padding manipulation, hex/binary
// conversion, and protocol related miscellaneous.
//
/// Padding
///-------
// The padding is done by inserting 1 to 4 bytes at the end of the data, to
// round its size to a 4 bytes boundary.  The value of the bytes correspond to
// the number of padding bytes added.  If the data was already padded, 4 bytes
// each having the value '4' will be added.  Similarly, the dword version adds
// 1 to 8 padding bytes to ensure 8 bytes alignment.  Note that using the
// padding related methods is undefined unless 'initialize' has been called.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ
#include <bmqp_protocol.h>
#include <bmqt_resultcode.h>
#include <bmqu_blob.h>

// BDE
#include <balber_berdecoder.h>
#include <balber_berdecoderoptions.h>
#include <balber_berencoder.h>
#include <balber_berencoderoptions.h>
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <baljsn_encoder.h>
#include <baljsn_encoderoptions.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobstreambuf.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bsl_functional.h>
#include <bsl_ios.h>
#include <bsl_ostream.h>
#include <bsl_streambuf.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

namespace bmqp {

// ===================
// struct ProtocolUtil
// ===================

/// Utilities for BlazingMQ protocol builders and iterators
// NOLINTBEGIN(*-avoid-c-arrays)
struct ProtocolUtil {
    // CLASS DATA

    /// This public constant represents an empty appId.
    static const char k_NULL_APP_ID[];

    /// This public constant represents the default appId.
    static const char k_DEFAULT_APP_ID[];

    // CLASS METHODS

    /// Initialization
    ///--------------

    /// DEPRECATED: no-op, remove both initialize/shutdown.
    static void initialize(bslma::Allocator* allocator = 0);

    /// DEPRECATED: no-op, remove both initialize/shutdown.
    static void shutdown();

    /// Padding
    ///-------
    static int calcNumWordsAndPadding(int* padding, int length);

    /// Return the number of words (respectively dwords) needed for a packet
    /// having the specified `length` bytes, and load in the specified
    /// `padding` the number of padding bytes that need to be added to make
    /// this many bytes be 4-bytes (respectively 8-bytes) aligned.  Note
    /// that if `length` is divisible by `4` (respectively `8`), a padding
    /// of `4` (respectively `8`) bytes will be indicated.
    static int calcNumDwordsAndPadding(int* padding, int length);

    /// Append the necessary bytes of padding to the specified `destination`
    /// blob, to align the specified `payloadLength` to a 4-bytes boundary.
    /// Return the number of words needed to represent `payloadLength` data
    /// with its added padding.  Note that this uses `payloadLength` and not
    /// the `blob` size to compute the padding.  The behavior is undefined
    /// if `destination` is an empty blob.
    static int appendPadding(bdlbb::Blob* destination, int payloadLength);

    static void appendPaddingRaw(bdlbb::Blob* destination,
                                 int          numPaddingBytes);
    static void appendPaddingRaw(char* destination, int numPaddingBytes);

    /// Append the specified `numPaddingBytes` of padding to the specified
    /// `destination` for 4-byte alignment (or 8 in the `dword` flavor).
    /// The behavior is undefined unless `numPaddingBytes >= 1` and
    /// `numPaddingBytes <= 4` (or 8 in the `dword` flavor).
    static void appendPaddingDwordRaw(char* destination, int numPaddingBytes);

    /// Return the length of un-padded data given the specified `blob` and
    /// `length`.  Note that `length` can be less than length of the `blob`.
    static int calcUnpaddedLength(const bdlbb::Blob& blob, int length);

    /// Message encoding/decoding
    ///-------------------------

    /// Encode the template specified `message` into the specified `out`
    /// using the specified `encodingType`.  Return 0 on success, or a
    /// non-zero value on error and fill in the specified `errorDescription`
    /// stream with the description of the error.  Use the specified
    /// `allocator` for memory allocations.
    template <class TYPE>
    static int encodeMessage(bsl::ostream&      errorDescription,
                             bsl::streambuf*    out,
                             const TYPE&        message,
                             EncodingType::Enum encodingType,
                             bslma::Allocator*  allocator = 0);
    template <class TYPE>
    static int encodeMessage(bsl::ostream&      errorDescription,
                             bdlbb::Blob*       out,
                             const TYPE&        message,
                             EncodingType::Enum encodingType,
                             bslma::Allocator*  allocator = 0);

    /// Load into the template specified `message`, the decoded message
    /// contained in the specified `stream` using the specified
    /// `encodingType`.  Return 0 on success, or a non-zero value on error
    /// and fill in the specified `errorDescription` stream with the
    /// description of the error.  Use the specified `allocator` for memory
    /// allocations.
    template <class TYPE>
    static int decodeMessage(bsl::ostream&      errorDescription,
                             TYPE*              message,
                             bsl::streambuf*    stream,
                             EncodingType::Enum encodingType,
                             bslma::Allocator*  allocator = 0);

    /// Load into the template specified `message`, the decoded message
    /// contained at the specified `offset` of the specified `blob` using
    /// the specified `encodingType`.  Return 0 on success, or a non-zero
    /// value on error and fill in the specified `errorDescription` stream
    /// with the description of the error.  Use the specified `allocator`
    /// for memory allocations.
    template <class TYPE>
    static int decodeMessage(bsl::ostream&      errorDescription,
                             TYPE*              message,
                             const bdlbb::Blob& blob,
                             int                offset,
                             EncodingType::Enum encodingType,
                             bslma::Allocator*  allocator = 0);

    /// Load into the template specified `message`, the decoded message
    /// having the specified `length` contained at the specified `offset` of
    /// the specified `entry` using the specified `encodingType`.  Return 0
    /// on success, or a non-zero value on error and fill in the specified
    /// `errorDescription` stream with the description of the error.  Use
    /// the specified `allocator` for memory allocations.
    template <class TYPE>
    static int decodeMessage(bsl::ostream&      errorDescription,
                             TYPE*              message,
                             const char*        entry,
                             int                offset,
                             int                length,
                             EncodingType::Enum encodingType,
                             bslma::Allocator*  allocator = 0);

    /// Protocol miscellaneous
    ///----------------------
    static int ackResultToCode(bmqt::AckResult::Enum value);

    /// Convert the specified `value` between an `AckResult` enum value
    /// (used in BlazingMQ APIs) and a code value (used in the
    /// `bmqp::AckMessage` protocol structure).  The reason is that the
    /// protocol structure only allows 4 bits to encode the status value,
    /// while the `AckResult` has a much wider range of possible values.
    static bmqt::AckResult::Enum ackResultFromCode(int value);

    /// From the specified `featureSet`, load the list of values
    /// associated with the specified `fieldName` into the specified
    /// `fieldValues`. Return true if `featureSet` is properly-formed and
    /// `fieldName` is a key found in `featureSet`; return false otherwise.
    ///
    /// The `featureSet` must be formated as follows:
    ///   "<fieldName1>:<val1>,<val2>,<val3>;<fieldName2>:<val4>"
    static bool loadFieldValues(bsl::vector<bsl::string>* fieldValues,
                                const bsl::string&        fieldName,
                                const bsl::string&        featureSet);

    static bool hasFeature(const char*        fieldName,
                           const char*        feature,
                           const bsl::string& featureSet);

    /// Invoke the specified `action` and if it returns e_EVENT_TOO_BIG then
    /// invoke the specified `overflowCb` and call `action` again.  Return
    /// result code returned from `action`
    /// DEPRECATED: use `buildEvent` with functors instead, to avoid loss of
    ///             performance from `bsl::function` on critical paths.
    static bmqt::EventBuilderResult::Enum buildEvent(
        const bsl::function<bmqt::EventBuilderResult::Enum(void)>& action,
        const bsl::function<void(void)>&                           overflowCb);

    /// Invoke the specified `actionCb` and if it returns e_EVENT_TOO_BIG then
    /// invoke the specified `overflowCb` and call `actionCb` again.  Return
    /// result code returned from `action`
    template <class ACTION_FUNCTOR_TYPE, class OVERFLOW_FUNCTOR_TYPE>
    static bmqt::EventBuilderResult::Enum
    buildEvent(ACTION_FUNCTOR_TYPE&   actionCb,
               OVERFLOW_FUNCTOR_TYPE& overflowCb);

    /// Encode Receipt into the specified `blob` (expected to be empty) for
    /// the specified `partitionId`, `primaryLeaseId`, and `sequenceNumber`.
    static void buildReceipt(bdlbb::Blob*        blob,
                             int                 partitionId,
                             unsigned int        primaryLeaseId,
                             bsls::Types::Uint64 sequenceNumber);

    /// If the specified `src` has MessageProperties encoded in the new
    /// style (schema > 0), re-write `propertyValueLength` for each property
    /// to represent length (old style) instead of offset (new style).
    /// If the specified `dst` is the same as `src`, perform modification
    /// in `src`, otherwise copy Message Properties headers to `dst`,
    /// perform modification, and append the rest.  Do not copy when there
    /// is no modification.  If the specified `cat` is not `e_NONE`, make
    /// sure the `dst` contains entire Message Properties area (possibly
    /// modified in the previous step), and append de-compressed payload
    /// (past the Message Properties area) to `dst`.  The behavior is
    /// undefined if `cat` is not `e_NONE` and `dst == src`.
    /// Return `0` on success.
    static int convertToOld(bdlbb::Blob*                         dst,
                            const bdlbb::Blob*                   src,
                            bmqt::CompressionAlgorithmType::Enum cat,
                            bdlbb::BlobBufferFactory*            factory,
                            bslma::Allocator*                    allocator);

    /// Parse `MesasgePropertiesHeader` out of the specified `blob` at the
    /// specified `position` and load the size of message properties
    /// (messagePropertiesAreaWords * WORD_SIZE) into the specified `size`.
    /// Return `true` on success, `false` otherwise.
    static int readPropertiesSize(int*                      size,
                                  const bdlbb::Blob&        blob,
                                  const bmqu::BlobPosition& position);

    /// Parse the specified `input` of the specified `length` starting at
    /// the specified `position` and supporting all styles of
    /// MessageProperties compression indicated by the specified
    /// `haveMessageProperties`, `haveNewMessageProperties`, and `cat`.  If
    /// the specified `messagePropertiesOutput` is not `0`, output the
    /// MessageProperties blob into `messagePropertiesOutput` and the rest
    /// of input data into the specified `dataOutput`; otherwise, output
    /// everything into `dataOutput`.  Return `0` on success, `1` if the
    /// input is compressed and the specified `decompressFlag` is `false` in
    /// which case parsing of MessageProperties succeeds only if
    /// `haveNewMessageProperties` is `true` (un-compressed) and parsing of
    /// data does not succeed.  Return negative code on failure.
    static int parse(bdlbb::Blob*              messagePropertiesOutput,
                     int*                      messagePropertiesSize,
                     bdlbb::Blob*              dataOutput,
                     const bdlbb::Blob&        input,
                     int                       length,
                     bool                      decompressFlag,
                     const bmqu::BlobPosition& position,
                     bool                      haveMessageProperties,
                     bool                      haveNewMessageProperties,
                     bmqt::CompressionAlgorithmType::Enum cat,
                     bdlbb::BlobBufferFactory*            blobBufferFactory,
                     bslma::Allocator*                    allocator);
};
// NOLINTEND(*-avoid-c-arrays)

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// struct ProtocolUtil
// -------------------

inline int ProtocolUtil::calcNumWordsAndPadding(int* padding, int length)
{
    int numWords = (length + Protocol::k_WORD_SIZE) / Protocol::k_WORD_SIZE;
    *padding     = numWords * Protocol::k_WORD_SIZE - length;

    return numWords;
}

inline int ProtocolUtil::calcNumDwordsAndPadding(int* padding, int length)
{
    int numWords = (length + Protocol::k_DWORD_SIZE) / Protocol::k_DWORD_SIZE;
    *padding     = numWords * Protocol::k_DWORD_SIZE - length;

    return numWords;
}

inline int ProtocolUtil::appendPadding(bdlbb::Blob* destination,
                                       int          payloadLength)
{
    int numPaddingBytes = 0;
    int numWords = calcNumWordsAndPadding(&numPaddingBytes, payloadLength);

    appendPaddingRaw(destination, numPaddingBytes);

    return numWords;
}

template <class TYPE>
int ProtocolUtil::encodeMessage(bsl::ostream&      errorDescription,
                                bsl::streambuf*    out,
                                const TYPE&        message,
                                EncodingType::Enum encodingType,
                                bslma::Allocator*  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    switch (encodingType) {
    case EncodingType::e_BER: {
        balber::BerEncoderOptions options;
        balber::BerEncoder        encoder(&options, allocator);

        const int rc = encoder.encode(out, message);

        // Debug print message if any
        bslstl::StringRef logStr = encoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!logStr.empty())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            errorDescription << "BER encoder returned the following "
                             << "[rc: " << rc << "]\n"
                             << logStr;
        }

        return rc;  // RETURN
    }
    case EncodingType::e_JSON: {
        baljsn::EncoderOptions options;
        baljsn::Encoder        encoder(allocator);

        const int rc = encoder.encode(out, message, options);

        // Debug print message if any
        bsl::string logStr = encoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!logStr.empty())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            errorDescription << "JSON encoder returned the following "
                             << "[rc: " << rc << "]\n"
                             << logStr;
        }

        return rc;  // RETURN
    }
    case EncodingType::e_UNKNOWN:
    default: {
        errorDescription << "Unsupported encoding type: " << encodingType;
        return -1;  // RETURN
    }
    }
}

template <class TYPE>
int ProtocolUtil::encodeMessage(bsl::ostream&      errorDescription,
                                bdlbb::Blob*       out,
                                const TYPE&        message,
                                EncodingType::Enum encodingType,
                                bslma::Allocator*  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bdlbb::OutBlobStreamBuf osb(out);
    return encodeMessage(errorDescription,
                         &osb,
                         message,
                         encodingType,
                         allocator);
}

template <class TYPE>
int ProtocolUtil::decodeMessage(bsl::ostream&      errorDescription,
                                TYPE*              message,
                                bsl::streambuf*    stream,
                                EncodingType::Enum encodingType,
                                bslma::Allocator*  allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(message);

    switch (encodingType) {
    case EncodingType::e_BER: {
        balber::BerDecoderOptions options;
        options.setSkipUnknownElements(true);
        options.setDefaultEmptyStrings(false);
        balber::BerDecoder decoder(&options, allocator);

        const int rc = decoder.decode(stream, message);

        // Debug print message if any
        bslstl::StringRef logStr = decoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!logStr.empty())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            errorDescription << "BER decoder returned the following "
                             << "[rc: " << rc << "]\n"
                             << logStr;
        }

        return rc;  // RETURN
    }
    case EncodingType::e_JSON: {
        baljsn::Decoder        decoder(allocator);
        baljsn::DecoderOptions options;
        options.setSkipUnknownElements(true);

        const int rc = decoder.decode(stream, message, options);

        // Debug print message if any
        bsl::string logStr = decoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!logStr.empty())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            errorDescription << "JSON decoder returned the following "
                             << "[rc: " << rc << "]\n"
                             << logStr;
        }

        return rc;  // RETURN
    }
    case EncodingType::e_UNKNOWN:
    default: {
        errorDescription << "Unsupported encoding type: " << encodingType;
        return -1;  // RETURN
    }
    }
}

template <class TYPE>
int ProtocolUtil::decodeMessage(bsl::ostream&      errorDescription,
                                TYPE*              message,
                                const bdlbb::Blob& blob,
                                int                offset,
                                EncodingType::Enum encodingType,
                                bslma::Allocator*  allocator)
{
    bdlbb::InBlobStreamBuf isb(&blob);
    isb.pubseekpos(offset, bsl::ios_base::in);
    return decodeMessage(errorDescription,
                         message,
                         &isb,
                         encodingType,
                         allocator);
}

template <class TYPE>
int ProtocolUtil::decodeMessage(bsl::ostream&      errorDescription,
                                TYPE*              message,
                                const char*        entry,
                                int                offset,
                                int                length,
                                EncodingType::Enum encodingType,
                                bslma::Allocator*  allocator)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    bdlsb::FixedMemInStreamBuf isb(entry + offset, length);
    return decodeMessage(errorDescription,
                         message,
                         &isb,
                         encodingType,
                         allocator);
}

inline bmqt::EventBuilderResult::Enum ProtocolUtil::buildEvent(
    const bsl::function<bmqt::EventBuilderResult::Enum(void)>& action,
    const bsl::function<void(void)>&                           overflowCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(action);
    BSLS_ASSERT_SAFE(overflowCb);

    bmqt::EventBuilderResult::Enum rc = action();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqt::EventBuilderResult::e_EVENT_TOO_BIG == rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        overflowCb();
        rc = action();
    }
    return rc;
};

template <class ACTION_FUNCTOR_TYPE, class OVERFLOW_FUNCTOR_TYPE>
inline bmqt::EventBuilderResult::Enum
ProtocolUtil::buildEvent(ACTION_FUNCTOR_TYPE&   actionCb,
                         OVERFLOW_FUNCTOR_TYPE& overflowCb)
{
    bmqt::EventBuilderResult::Enum rc = actionCb();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            bmqt::EventBuilderResult::e_EVENT_TOO_BIG == rc)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        overflowCb();
        rc = actionCb();
    }
    return rc;
};

inline void ProtocolUtil::buildReceipt(bdlbb::Blob*        blob,
                                       int                 partitionId,
                                       unsigned int        primaryLeaseId,
                                       bsls::Types::Uint64 sequenceNumber)
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 == blob->length());

    blob->setLength(sizeof(EventHeader) + sizeof(ReplicationReceipt));

    char* buffer = blob->buffer(0).data();
    new (buffer) EventHeader(EventType::e_REPLICATION_RECEIPT);
    reinterpret_cast<EventHeader*>(buffer)->setLength(blob->length());

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast)
    ReplicationReceipt* receipt = reinterpret_cast<ReplicationReceipt*>(
        buffer + sizeof(EventHeader));
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast)

    (*receipt)
        .setPartitionId(partitionId)
        .setPrimaryLeaseId(primaryLeaseId)
        .setSequenceNum(sequenceNumber);
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

}  // close package namespace
}  // close enterprise namespace

#endif
