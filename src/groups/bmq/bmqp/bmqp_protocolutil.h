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

// bmqp_protocolutil.h                                                -*-C++-*-
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

#include <bmqc_twokeyhashmap.h>
#include <bmqp_ctrlmsg_messages.h>
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
#include <bdlsb_fixedmemoutstreambuf.h>
#include <bsl_functional.h>
#include <bsl_ios.h>
#include <bsl_ostream.h>
#include <bsl_streambuf.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {

namespace bmqp {

// ===================
// struct ProtocolUtil
// ===================

/// Utilities for BlazingMQ protocol builders and iterators
struct ProtocolUtil {
    // PUBLIC TYPES

    template <class VALUE>
    struct QueueInfo {
        struct StreamInfo {
            VALUE d_value;

            StreamInfo(const VALUE& value);
        };

        typedef bmqc::TwoKeyHashMap<bsl::string, unsigned int, StreamInfo>
            StreamsMap;
        // Map {appId, subId} to StreamInfo

        /// This struct provides named access to `VALUE` - by `appId` and by
        /// `subId`
        struct iterator {
            typename StreamsMap::iterator d_iterator;

            iterator(const typename StreamsMap::iterator& it);
            iterator(const iterator& other);

            VALUE& value();

            unsigned int subId() const;

            const bsl::string& appId() const;

            iterator& operator++();

            iterator* operator->();

            bool operator!=(const iterator& other) const;
            bool operator==(const iterator& other) const;
        };
        /// This struct provides named access to `VALUE` - by `appId` and by
        /// `subId`
        struct const_iterator {
            typename StreamsMap::const_iterator d_iterator;

            const_iterator(const typename StreamsMap::iterator& it);
            const_iterator(const typename StreamsMap::const_iterator& cit);

            const_iterator(const iterator& other);

            const VALUE& value() const;

            unsigned int subId() const;

            const bsl::string& appId() const;

            const_iterator& operator++();

            const_iterator* operator->();

            bool operator!=(const const_iterator& other) const;
            bool operator==(const const_iterator& other) const;
        };

        /// Map subscriptionId to StreamInfo
        typedef bsl::unordered_map<unsigned int, iterator> SubscriptionsMap;
        typedef bsl::vector<bmqp_ctrlmsg::Subscription>    Subscriptions;

        StreamsMap d_streams;

        SubscriptionsMap d_subscriptions;

        bslma::Allocator* d_allocator_p;

        QueueInfo(bslma::Allocator* allocator);
        QueueInfo(const QueueInfo& other, bslma::Allocator* allocator = 0);

        iterator insert(const bsl::string& appId,
                        unsigned int       subId,
                        const VALUE&       value);
        iterator insert(const bmqp_ctrlmsg::QueueHandleParameters& parameters,
                        const VALUE&                               value);
        iterator insert(const bmqp_ctrlmsg::QueueStreamParameters& parameters,
                        const VALUE&                               value);

        iterator       findBySubIdSafe(unsigned int subId);
        const_iterator findBySubIdSafe(unsigned int subId) const;

        iterator       findByAppIdSafe(const bsl::string& appId);
        const_iterator findByAppIdSafe(const bsl::string& appId) const;

        iterator       findBySubId(unsigned int subId);
        const_iterator findBySubId(unsigned int subId) const;

        iterator findByHandleParameters(
            const bmqp_ctrlmsg::QueueHandleParameters& parameters);

        iterator       findByAppId(const bsl::string& appId);
        const_iterator findByAppId(const bsl::string& appId) const;

        const_iterator
        findBySubscriptionIdSafe(unsigned int subscriptionId) const;

        const_iterator findBySubscriptionId(unsigned int subscriptionId) const;

        const_iterator begin() const;
        iterator       begin();

        const_iterator end() const;
        iterator       end();

        void erase(const_iterator it);

        void removeSubscriptions(const bsl::string& appId);

        void
        addSubscriptions(const bmqp_ctrlmsg::StreamParameters& parameters);

        void clear();

        typename StreamsMap::size_type size() const;
    };

    // CLASS DATA

    /// This public constant represents an empty appId.
    static const char k_NULL_APP_ID[];

    /// This public constant represents the default appId.
    static const char k_DEFAULT_APP_ID[];

    // CLASS METHODS

    /// Initialization
    ///--------------

    /// Perform some one time initialization needed by the padding and
    /// heartbeats related methods by preallocating the various size padding
    /// buffers and blobs.  This method only needs to be called once before
    /// any other method, but can be called multiple times provided that for
    /// each call to `initialize` there is a corresponding call to
    /// `shutdown`.  Use the optionally specified `allocator` for any memory
    /// allocation, or the `global` allocator if none is provided.  Note
    /// that specifying the allocator is provided for test drivers only, and
    /// therefore users should let it default to the global allocator.
    static void initialize(bslma::Allocator* allocator = 0);

    /// Pendant operation of the `initialize` one.  The number of calls to
    /// `shutdown` must equal the number of calls to `initialize`, without
    /// corresponding `shutdown` calls, to fully destroy the objects.  It is
    /// safe to call `initialize` after calling `shutdown`.  The behaviour
    /// is undefined if `shutdown` is called without `initialize` first
    /// being called.  Note that shutdown must not be called until all blobs
    /// which had a padding buffer appended have been destroyed.
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

    /// Return true if the specified `value` is a valid padding byte for a
    /// word sized alignment and false otherwise.
    static bool isValidWordPaddingByte(char value);

    /// Return true if the specified `value` is a valid padding byte for a
    /// double word sized alignment and false otherwise.
    static bool isValidDWordPaddingByte(char value);

    /// HEARTBEAT
    ///---------
    static const bdlbb::Blob& heartbeatReqBlob();

    /// Return a read-only reference to the statically created and prefilled
    /// blob corresponding to a heartbeat request or heartbeat response
    /// event.
    static const bdlbb::Blob& heartbeatRspBlob();

    /// Return const reference to the pre-allocated empty blob.  If you need
    /// an empty blob and not going to modify it use this method instead of
    /// creating another blob.  Heap allocate it to prevent 'exit-time-
    /// destructor needed' compiler warning.  Causes valgrind-reported
    /// memory leak.
    static const bdlbb::Blob& emptyBlob();

    /// Hex/Binary conversion
    ///---------------------

    /// Load into the specified `buffer` of specified `length` the binary
    /// representation of the specified hexadecimal `hex` buffer.  The
    /// behavior is undefined unless length of `hex` buffer is twice
    /// `length`.
    static void hexToBinary(char* buffer, int length, const char* hex);

    /// Load into the specified `buffer` the hex representation of the
    /// specified `binary` buffer of the specified `binaryBufferlength`
    /// size.  The behavior is undefined unless the length of `buffer` is
    /// twice `binaryBufferlength`.
    static void
    binaryToHex(char* buffer, const char* binary, int binaryBufferlength);

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
    static bmqt::EventBuilderResult::Enum buildEvent(
        const bsl::function<bmqt::EventBuilderResult::Enum(void)>& action,
        const bsl::function<void(void)>&                           overflowCb);

    /// Encode Receipt into the specified `blob` for the specified
    /// `partitionId`, `primaryLeaseId`, and `sequenceNumber`.
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

    /// Temporary
    static void convert(
        bmqp_ctrlmsg::QueueStreamParameters*                     to,
        const bmqp_ctrlmsg::StreamParameters&                    from,
        const bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>& subIdInfo);

    /// Temporary
    static void convert(bmqp_ctrlmsg::StreamParameters*            to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from);

    /// Temporary
    static void convert(bmqp_ctrlmsg::ConfigureStream*            to,
                        const bmqp_ctrlmsg::ConfigureQueueStream& from);

    static void makeResponse(bmqp_ctrlmsg::ControlMessage*         response,
                             const bmqp_ctrlmsg::StreamParameters& from,
                             const bmqp_ctrlmsg::ControlMessage&   request);

    /// Temporary
    static void convert(bmqp_ctrlmsg::ConsumerInfo*                to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from);
    static const bmqp_ctrlmsg::ConsumerInfo&
    consumerInfo(const bmqp_ctrlmsg::StreamParameters& from);

    static bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>
    makeSubQueueIdInfo(const bsl::string& appId, unsigned int subId);

    static bool verify(const bmqp_ctrlmsg::ConsumerInfo& ci);

    static bool verify(const bmqp_ctrlmsg::StreamParameters& parameters);
};

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

        int rc = encoder.encode(out, message);

        // Debug print message if any
        bslstl::StringRef logStr = encoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(logStr.length() != 0)) {
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

        int rc = encoder.encode(out, message, options);

        // Debug print message if any
        bsl::string logStr = encoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(logStr.length() != 0)) {
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
        balber::BerDecoder        decoder(&options, allocator);
        options.setSkipUnknownElements(true);
        options.setDefaultEmptyStrings(false);

        int rc = decoder.decode(stream, message);

        // Debug print message if any
        bslstl::StringRef logStr = decoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(logStr.length() != 0)) {
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

        int rc = decoder.decode(stream, message, options);

        // Debug print message if any
        bsl::string logStr = decoder.loggedMessages();
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(logStr.length() != 0)) {
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

inline void ProtocolUtil::buildReceipt(bdlbb::Blob*        blob,
                                       int                 partitionId,
                                       unsigned int        primaryLeaseId,
                                       bsls::Types::Uint64 sequenceNumber)
{
    blob->removeAll();

    blob->setLength(sizeof(EventHeader) + sizeof(ReplicationReceipt));

    char* buffer = blob->buffer(0).data();
    new (buffer) EventHeader(EventType::e_REPLICATION_RECEIPT);
    reinterpret_cast<EventHeader*>(buffer)->setLength(blob->length());

    ReplicationReceipt* receipt = reinterpret_cast<ReplicationReceipt*>(
        buffer + sizeof(EventHeader));

    (*receipt)
        .setPartitionId(partitionId)
        .setPrimaryLeaseId(primaryLeaseId)
        .setSequenceNum(sequenceNumber);
}

inline bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>
ProtocolUtil::makeSubQueueIdInfo(const bsl::string& appId, unsigned int subId)
{
    bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo> result;
    if (subId != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        result.makeValue();
        result.value().appId() = appId;
        result.value().subId() = subId;
    }

    return result;
}

inline void ProtocolUtil::convert(
    bmqp_ctrlmsg::QueueStreamParameters*                     to,
    const bmqp_ctrlmsg::StreamParameters&                    from,
    const bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>& subIdInfo)
{
    BSLS_ASSERT_SAFE(from.subscriptions().size() < 2);

    to->subIdInfo() = subIdInfo;

    if (from.subscriptions().size() > 0) {
        BSLS_ASSERT_SAFE(from.subscriptions().size() == 1);
        // Enable multiple subscriptions in SDK only when all brokers support
        // them.
        BSLS_ASSERT_SAFE(from.subscriptions()[0].consumers().size() > 0);
        // New ConfigureStream request can carry multiple priorities but the
        // first element in the array is the highest priority one.
        const bmqp_ctrlmsg::ConsumerInfo& ci =
            from.subscriptions()[0].consumers()[0];

        to->consumerPriority()       = ci.consumerPriority();
        to->consumerPriorityCount()  = ci.consumerPriorityCount();
        to->maxUnconfirmedMessages() = ci.maxUnconfirmedMessages();
        to->maxUnconfirmedBytes()    = ci.maxUnconfirmedBytes();
    }
    else {
        to->consumerPriority()       = Protocol::k_CONSUMER_PRIORITY_INVALID;
        to->consumerPriorityCount()  = 0;
        to->maxUnconfirmedMessages() = 0;
        to->maxUnconfirmedBytes()    = 0;
    }
}

inline void
ProtocolUtil::convert(bmqp_ctrlmsg::ConfigureStream*            to,
                      const bmqp_ctrlmsg::ConfigureQueueStream& from)
{
    const bmqp_ctrlmsg::QueueStreamParameters& oldStype =
        from.streamParameters();

    to->qId() = from.qId();
    to->streamParameters().subscriptions().resize(1);
    bmqp_ctrlmsg::Subscription& subscription =
        to->streamParameters().subscriptions()[0];

    if (!oldStype.subIdInfo().isNull()) {
        subscription.sId()             = oldStype.subIdInfo().value().subId();
        to->streamParameters().appId() = oldStype.subIdInfo().value().appId();
    }
    // else DEFAULT_INITIALIZER_ID      (0),
    //      DEFAULT_INITIALIZER_APP_ID  ("__default")

    subscription.consumers().resize(1);
    convert(&subscription.consumers()[0], oldStype);
}

inline void
ProtocolUtil::convert(bmqp_ctrlmsg::StreamParameters*            to,
                      const bmqp_ctrlmsg::QueueStreamParameters& from)
{
    if (!from.subIdInfo().isNull()) {
        to->appId() = from.subIdInfo().value().appId();
    }
    // else DEFAULT_INITIALIZER_APP_ID  ("__default")

    if (from.consumerPriorityCount()) {
        to->subscriptions().resize(1);
        to->subscriptions()[0].consumers().resize(1);

        convert(&to->subscriptions()[0].consumers()[0], from);
    }
    else {
        BSLS_ASSERT_SAFE(from.consumerPriority() ==
                         Protocol::k_CONSUMER_PRIORITY_INVALID);
        BSLS_ASSERT_SAFE(from.consumerPriorityCount() == 0);
        BSLS_ASSERT_SAFE(from.maxUnconfirmedMessages() == 0);
        BSLS_ASSERT_SAFE(from.maxUnconfirmedBytes() == 0);
    }
}

inline void
ProtocolUtil::convert(bmqp_ctrlmsg::ConsumerInfo*                to,
                      const bmqp_ctrlmsg::QueueStreamParameters& from)
{
    to->consumerPriority()       = from.consumerPriority();
    to->consumerPriorityCount()  = from.consumerPriorityCount();
    to->maxUnconfirmedMessages() = from.maxUnconfirmedMessages();
    to->maxUnconfirmedBytes()    = from.maxUnconfirmedBytes();
}

inline void
ProtocolUtil::makeResponse(bmqp_ctrlmsg::ControlMessage*         response,
                           const bmqp_ctrlmsg::StreamParameters& from,
                           const bmqp_ctrlmsg::ControlMessage&   request)
{
    response->rId() = request.rId();

    if (request.choice().isConfigureQueueStreamValue()) {
        bmqp_ctrlmsg::ConfigureQueueStream& oldStyleRequest =
            response->choice().makeConfigureQueueStreamResponse().request();
        const bmqp_ctrlmsg::ConfigureQueueStream& original =
            request.choice().configureQueueStream();
        bmqp_ctrlmsg::QueueStreamParameters parameters =
            oldStyleRequest.streamParameters();

        oldStyleRequest.qId() = original.qId();

        // QE::configureHandle mirrors parameters currently but let's leave
        // the possibility for changing them.  For this reason, reconstruct
        // parameters.

        convert(&parameters, from, original.streamParameters().subIdInfo());
    }
    else {
        BSLS_ASSERT_SAFE(request.choice().isConfigureStreamValue());

        bmqp_ctrlmsg::ConfigureStream& newStyleRequest =
            response->choice().makeConfigureStreamResponse().request();

        newStyleRequest = request.choice().configureStream();
    }
}

inline const bmqp_ctrlmsg::ConsumerInfo&
ProtocolUtil::consumerInfo(const bmqp_ctrlmsg::StreamParameters& from)
{
    size_t n = from.subscriptions().size();

    if (n == 0) {
        static const bmqp_ctrlmsg::ConsumerInfo& defaultValues = *(
            new bmqp_ctrlmsg::ConsumerInfo());

        // Heap allocate it to prevent 'exit-time-destructor needed' compiler
        // warning.  Causes valgrind-reported memory leak.

        return defaultValues;
    }

    BSLS_ASSERT_SAFE(from.subscriptions()[0].consumers().size() > 0);

    // Consider only the first (highest priority) consumer
    return from.subscriptions()[0].consumers()[0];
}

// ------------------------------------------
// struct ProtocolUtil::QueueInfo::StreamInfo
// ------------------------------------------

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::StreamInfo::StreamInfo(
    const VALUE& value)
: d_value(value)
{
    // NOTHING
}

// ----------------------------------------
// struct ProtocolUtil::QueueInfo::iterator
// ----------------------------------------

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::iterator::iterator(
    const typename StreamsMap::iterator& it)
: d_iterator(it)
{
    // NOTHING
}

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::iterator::iterator(
    const iterator& other)
: d_iterator(other.d_iterator)
{
    // NOTHING
}

template <class VALUE>
inline VALUE& ProtocolUtil::QueueInfo<VALUE>::iterator::value()
{
    return d_iterator->value().d_value;
}

template <class VALUE>
inline unsigned int ProtocolUtil::QueueInfo<VALUE>::iterator::subId() const
{
    return d_iterator->key2();
}

template <class VALUE>
inline const bsl::string&
ProtocolUtil::QueueInfo<VALUE>::iterator::appId() const
{
    return d_iterator->key1();
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator&
ProtocolUtil::QueueInfo<VALUE>::iterator::operator++()
{
    ++d_iterator;
    return *this;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator*
ProtocolUtil::QueueInfo<VALUE>::iterator::operator->()
{
    return this;
}

template <class VALUE>
inline bool ProtocolUtil::QueueInfo<VALUE>::iterator::operator!=(
    const iterator& other) const
{
    return d_iterator != other.d_iterator;
}

template <class VALUE>
inline bool ProtocolUtil::QueueInfo<VALUE>::iterator::operator==(
    const iterator& other) const
{
    return d_iterator == other.d_iterator;
}

// ----------------------------------------------
// struct ProtocolUtil::QueueInfo::const_iterator
// ----------------------------------------------

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::const_iterator::const_iterator(
    const typename StreamsMap::iterator& it)
: d_iterator(it)
{
    // NOTHING
}

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::const_iterator::const_iterator(
    const typename StreamsMap::const_iterator& cit)
: d_iterator(cit)
{
    // NOTHING
}

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::const_iterator::const_iterator(
    const iterator& other)
: d_iterator(other.d_iterator)
{
    // NOTHING
}

template <class VALUE>
inline const VALUE&
ProtocolUtil::QueueInfo<VALUE>::const_iterator::value() const
{
    return d_iterator->value().d_value;
}

template <class VALUE>
inline unsigned int
ProtocolUtil::QueueInfo<VALUE>::const_iterator::subId() const
{
    return d_iterator->key2();
}

template <class VALUE>
inline const bsl::string&
ProtocolUtil::QueueInfo<VALUE>::const_iterator::appId() const
{
    return d_iterator->key1();
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator&
ProtocolUtil::QueueInfo<VALUE>::const_iterator::operator++()
{
    ++d_iterator;
    return *this;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator*
ProtocolUtil::QueueInfo<VALUE>::const_iterator::operator->()
{
    return this;
}

template <class VALUE>
inline bool ProtocolUtil::QueueInfo<VALUE>::const_iterator::operator!=(
    const const_iterator& other) const
{
    return d_iterator != other.d_iterator;
}

template <class VALUE>
inline bool ProtocolUtil::QueueInfo<VALUE>::const_iterator::operator==(
    const const_iterator& other) const
{
    return d_iterator == other.d_iterator;
}

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::QueueInfo(bslma::Allocator* allocator)
: d_streams(allocator)
, d_subscriptions(allocator)
, d_allocator_p(allocator)
{
}

template <class VALUE>
inline ProtocolUtil::QueueInfo<VALUE>::QueueInfo(const QueueInfo&  other,
                                                 bslma::Allocator* allocator)
: d_streams(other.d_streams)
, d_subscriptions(other.d_subscriptions)
, d_allocator_p(allocator)
{
    // NOTHING
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::insert(const bsl::string& appId,
                                       unsigned int       subId,
                                       const VALUE&       value)
{
    // Extract previous Subscriptions pointing to the 'appId'
    bsl::vector<bsls::Types::Uint64> subscriptions(d_allocator_p);

    typename SubscriptionsMap::iterator it = d_subscriptions.begin();
    while (it != d_subscriptions.end()) {
        iterator subQueue(it->second);
        if (subQueue->appId() == appId) {
            subscriptions.push_back(it->first);
            it = d_subscriptions.erase(it);
        }
        else {
            ++it;
        }
    }

    d_streams.eraseByKey1(appId);

    bsl::pair<typename StreamsMap::iterator, typename StreamsMap::InsertResult>
        result = d_streams.insert(appId, subId, StreamInfo(value));

    BSLS_ASSERT_SAFE(result.second == StreamsMap::e_INSERTED);

    // Point extracted Subscriptions to the new entry
    iterator itStream(result.first);

    for (size_t i = 0; i < subscriptions.size(); ++i) {
        bsl::pair<typename SubscriptionsMap::iterator, bool> insertRC =
            d_subscriptions.insert(bsl::make_pair(subscriptions[i], itStream));
        BSLS_ASSERT_SAFE(insertRC.second);
        (void)insertRC;
    }
    return itStream;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::insert(
    const bmqp_ctrlmsg::QueueHandleParameters& parameters,
    const VALUE&                               value)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }

    return insert(id.appId(), id.subId(), value);
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::insert(
    const bmqp_ctrlmsg::QueueStreamParameters& parameters,
    const VALUE&                               value)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }

    return insert(id.appId(), id.subId(), value);
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubIdSafe(unsigned int subId)
{
    return iterator(d_streams.findByKey2(subId));
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubIdSafe(unsigned int subId) const
{
    return const_iterator(d_streams.findByKey2(subId));
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::findByAppIdSafe(const bsl::string& appId)
{
    return iterator(d_streams.findByKey1(appId));
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findByAppIdSafe(const bsl::string& appId) const
{
    return const_iterator(d_streams.findByKey1(appId));
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubId(unsigned int subId)
{
    iterator result = findBySubIdSafe(subId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubId(unsigned int subId) const
{
    const_iterator result = findBySubIdSafe(subId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::findByHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& parameters)
{
    bmqp_ctrlmsg::SubQueueIdInfo id(d_allocator_p);

    if (!parameters.subIdInfo().isNull()) {
        id = parameters.subIdInfo().value();
    }
    return iterator(d_streams.findByKey2(id.subId()));
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::findByAppId(const bsl::string& appId)
{
    iterator result = findByAppIdSafe(appId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findByAppId(const bsl::string& appId) const
{
    const_iterator result = findByAppIdSafe(appId);
    BSLS_ASSERT_SAFE(result != d_streams.end());

    return result;
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubscriptionIdSafe(
    unsigned int subscriptionId) const
{
    typename SubscriptionsMap::const_iterator cit = d_subscriptions.find(
        subscriptionId);
    return cit == d_subscriptions.end() ? end() : const_iterator(cit->second);
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::findBySubscriptionId(
    unsigned int subscriptionId) const
{
    typename SubscriptionsMap::const_iterator cit = d_subscriptions.find(
        subscriptionId);

    BSLS_ASSERT_SAFE(cit != d_subscriptions.end());

    return const_iterator(cit->second);
}
template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::begin() const
{
    return const_iterator(d_streams.begin());
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::begin()
{
    return iterator(d_streams.begin());
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::const_iterator
ProtocolUtil::QueueInfo<VALUE>::end() const
{
    return const_iterator(d_streams.end());
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::iterator
ProtocolUtil::QueueInfo<VALUE>::end()
{
    return iterator(d_streams.end());
}

template <class VALUE>
inline void ProtocolUtil::QueueInfo<VALUE>::erase(const_iterator it)
{
    removeSubscriptions(it->appId());

    d_streams.erase(it.d_iterator);
}

template <class VALUE>
inline void
ProtocolUtil::QueueInfo<VALUE>::removeSubscriptions(const bsl::string& appId)
{
    typename SubscriptionsMap::iterator it = d_subscriptions.begin();
    while (it != d_subscriptions.end()) {
        iterator subQueue(it->second);
        if (subQueue->appId() == appId) {
            it = d_subscriptions.erase(it);
        }
        else {
            ++it;
        }
    }
}

template <class VALUE>
inline void ProtocolUtil::QueueInfo<VALUE>::addSubscriptions(
    const bmqp_ctrlmsg::StreamParameters& parameters)
{
    removeSubscriptions(parameters.appId());

    iterator itStream = findByAppId(parameters.appId());

    const Subscriptions& subscriptions = parameters.subscriptions();
    for (Subscriptions::const_iterator it = subscriptions.begin();
         it != subscriptions.end();
         ++it) {
        bsl::pair<typename SubscriptionsMap::iterator, bool> insertRC =
            d_subscriptions.insert(bsl::make_pair(it->sId(), itStream));
        BSLS_ASSERT_SAFE(insertRC.second);
        (void)insertRC;
    }
}

template <class VALUE>
inline void ProtocolUtil::QueueInfo<VALUE>::clear()
{
    d_streams.clear();
    d_subscriptions.clear();
}

template <class VALUE>
inline typename ProtocolUtil::QueueInfo<VALUE>::StreamsMap::size_type
ProtocolUtil::QueueInfo<VALUE>::size() const
{
    return d_streams.size();
}

template <class T>
inline bsl::ostream& operator<<(bsl::ostream&                     os,
                                const ProtocolUtil::QueueInfo<T>& rhs)
{
    bslim::Printer printer(&os, 0, -1);  // one line

    printer.printValue(rhs.begin(), rhs.end());

    return os;
}

}  // close package namespace
}  // close enterprise namespace

#endif
