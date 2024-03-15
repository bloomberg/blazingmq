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

// bmqp_pushmessageiterator.t.cpp                                     -*-C++-*-
#include <bmqp_pushmessageiterator.h>

// BMQ
#include <bmqp_compression.h>
#include <bmqp_messageproperties.h>
#include <bmqp_optionutil.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqt_messageguid.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_random.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlt_currenttime.h>
#include <bdlt_epochutil.h>
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_unordered_set.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Struct representing attributes of a PushMessage.
struct Data {
    int d_flags;

    bdlbb::Blob d_payload;

    bdlbb::Blob d_properties;

    bdlbb::Blob d_appData;

    int d_queueId;

    bmqt::MessageGUID d_guid;

    int d_msgLen;

    int d_propLen;

    int d_optionsSize;

    mwcu::BlobPosition d_optionsPosition;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

    // CREATORS
    Data(bdlbb::BlobBufferFactory* bufferFactory, bslma::Allocator* allocator);
    Data(const Data& other, bslma::Allocator* allocator);
};

// CREATORS
Data::Data(bdlbb::BlobBufferFactory* bufferFactory,
           bslma::Allocator*         allocator)
: d_flags(0)
, d_payload(bufferFactory, allocator)
, d_properties(bufferFactory, allocator)
, d_appData(bufferFactory, allocator)
, d_queueId(-1)
{
    // NOTHING
}

Data::Data(const Data& other, bslma::Allocator* allocator)
: d_flags(other.d_flags)
, d_payload(other.d_payload, allocator)
, d_properties(other.d_properties, allocator)
, d_appData(other.d_appData, allocator)
, d_queueId(other.d_queueId)
, d_guid(other.d_guid)
, d_msgLen(other.d_msgLen)
, d_propLen(other.d_propLen)
, d_optionsSize(other.d_optionsSize)
, d_optionsPosition(other.d_optionsPosition)
{
    // NOTHING
}

typedef bdlb::NullableValue<bmqp::Protocol::MsgGroupId> NullableMsgGroupId;

/// Struct representing attributes of a PushMessage.  This struct is
/// allocator-aware and may be properly used in containers with an
/// allocator.
struct Data1 {
    // DATA
    bmqt::MessageGUID d_guid;

    int d_qid;

    bsl::vector<bmqp::SubQueueInfo> d_subQueueInfos;

    bool d_useOldSubQueueIds;

    NullableMsgGroupId d_msgGroupId;

    bdlbb::Blob d_payload;

    int d_flags;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data1, bslma::UsesBslmaAllocator)

    // CREATORS
    Data1(bdlbb::BlobBufferFactory* bufferFactory,
          bslma::Allocator*         allocator);
    Data1(const Data1& other, bslma::Allocator* allocator);
};

// CREATORS
Data1::Data1(bdlbb::BlobBufferFactory* bufferFactory,
             bslma::Allocator*         allocator)
: d_subQueueInfos(allocator)
, d_useOldSubQueueIds(false)
, d_msgGroupId(allocator)
, d_payload(bufferFactory, allocator)
{
    // NOTHING
}

Data1::Data1(const Data1& other, bslma::Allocator* allocator)
: d_guid(other.d_guid)
, d_qid(other.d_qid)
, d_subQueueInfos(other.d_subQueueInfos, allocator)
, d_useOldSubQueueIds(other.d_useOldSubQueueIds)
, d_msgGroupId(other.d_msgGroupId, allocator)
, d_payload(other.d_payload, allocator)
, d_flags(other.d_flags)
{
    // NOTHING
}

/// Return a 15-bit random number between the specified `min` and the
/// specified `max`, inclusive.  The behavior is undefined unless `min >= 0`
/// and `max >= min`.
static int generateRandomInteger(int min, int max)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(min >= 0);
    BSLS_ASSERT_OPT(max >= min);

    return min + (bsl::rand() % (max - min + 1));
}

/// Populate the specified `subQueueInfos` with the specified
/// `numSubQueueInfos` number of unique, randomly generated SubQueueInfos.
/// Clear the `subQueueInfos` prior to populating it.  The behavior is
/// undefined unless `numSubQueueInfos >= 0` and `numSubQueueInfos <= 200`.
static void
generateSubQueueInfos(bsl::vector<bmqp::SubQueueInfo>* subQueueInfos,
                      int                              numSubQueueInfos)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(subQueueInfos);
    BSLS_ASSERT_OPT(numSubQueueInfos >= 0);
    BSLS_ASSERT_OPT(numSubQueueInfos <= 200);

    subQueueInfos->clear();

    bsl::unordered_set<unsigned int> generatedIds(s_allocator_p);
    for (int i = 0; i < numSubQueueInfos; ++i) {
        unsigned int currId = static_cast<unsigned int>(
            generateRandomInteger(0, 200));
        while (generatedIds.find(currId) != generatedIds.end()) {
            currId = static_cast<unsigned int>(generateRandomInteger(0, 200));
        }
        generatedIds.insert(currId);

        const unsigned int rdaCounter = static_cast<unsigned int>(
            generateRandomInteger(0, bmqp::RdaInfo::k_MAX_COUNTER_VALUE));
        subQueueInfos->push_back(bmqp::SubQueueInfo(currId));
        subQueueInfos->back().rdaInfo().setCounter(rdaCounter);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_OPT(subQueueInfos->size() ==
                    static_cast<unsigned int>(numSubQueueInfos));
}

/// Populate the specified `msgGroupId` with a random Group Id.
static void generateMsgGroupId(bmqp::Protocol::MsgGroupId* msgGroupId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(msgGroupId);

    mwcu::MemOutStream oss(s_allocator_p);
    oss << "gid:" << generateRandomInteger(0, 200);
    *msgGroupId = oss.str();
}

/// Append at least the specified `atLeastLen` bytes to the specified `blob`
/// and populate the specified `payloadLen` with the number of bytes
/// appended.
static void populateBlob(bdlbb::Blob* blob, int* payloadLen, int atLeastLen)
{
    const char* k_FIXED_PAYLOAD =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    const int k_FIXED_PAYLOAD_LEN = bsl::strlen(k_FIXED_PAYLOAD);

    const int numIters = atLeastLen / k_FIXED_PAYLOAD_LEN + 1;

    for (int i = 0; i < numIters; ++i) {
        bdlbb::BlobUtil::append(blob, k_FIXED_PAYLOAD, k_FIXED_PAYLOAD_LEN);
    }

    *payloadLen = blob->length();
}

/// Append to the specified `data` an entry having the specified `qid`,
/// `useOldSubQueueIds` flag and `payloadLength` using the specified
/// `bufferFactory` and `allocator`.  The specified `hasSubQueueInfo`
/// indicates whether the entry contains any subQueue info.  The specified
/// `hasMsgGroupId` indicates whether the entry contains any message group
/// Id.  The behavior is undefined unless `payloadLength >= 0`.
static void appendDatum1(bsl::vector<Data1>*       data,
                         int                       qid,
                         bool                      hasSubQueueInfo,
                         bool                      useOldSubQueueIds,
                         bool                      hasMsgGroupId,
                         int                       payloadLength,
                         bdlbb::BlobBufferFactory* bufferFactory,
                         bslma::Allocator*         allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(data);
    BSLS_ASSERT_OPT(payloadLength >= 0);

    Data1 datum(bufferFactory, allocator);
    datum.d_qid   = qid;
    datum.d_flags = 0;

    // Generate SubQueueInfos
    if (hasSubQueueInfo) {
        generateSubQueueInfos(&datum.d_subQueueInfos, 1);
    }
    datum.d_useOldSubQueueIds = useOldSubQueueIds;

    // Generate Group Id
    if (hasMsgGroupId) {
        BSLS_ASSERT_OPT(datum.d_msgGroupId.isNull());
        generateMsgGroupId(&datum.d_msgGroupId.makeValue());
    }

    // Populate blob
    int numPopulated = 0;
    populateBlob(&datum.d_payload, &numPopulated, payloadLength);
    BSLS_ASSERT_OPT(numPopulated < bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT);

    data->push_back(datum);
}

/// Add to the event being built by the specified `PushEventBuilder`
/// messages using the specified `data`.
static void appendMessages1(bmqp::EventHeader*        eh,
                            bdlbb::Blob*              blob,
                            const bsl::vector<Data1>& data,
                            bdlbb::BlobBufferFactory* bufferFactory,
                            BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(eh);
    BSLS_ASSERT_OPT(blob);
    BSLS_ASSERT_OPT(bufferFactory);

    int eventLength = sizeof(bmqp::EventHeader);

    // Event Header
    eh->setType(bmqp::EventType::e_PUSH);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    // Write Event Header
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    for (bsl::vector<Data1>::size_type i = 0; i < data.size(); ++i) {
        const Data1& D               = data[i];
        int          subQueueWords   = 0;
        const bool   hasSubQueueInfo = D.d_subQueueInfos.size() == 1;
        const bool   isPackedOptions = hasSubQueueInfo &&
                                     !D.d_useOldSubQueueIds &&
                                     (D.d_subQueueInfos[0].id() ==
                                      bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);

        if (hasSubQueueInfo) {
            if (isPackedOptions) {
                subQueueWords = sizeof(bmqp::OptionHeader) /
                                bmqp::Protocol::k_WORD_SIZE;
            }
            else {
                const size_t itemSize    = D.d_useOldSubQueueIds
                                               ? bmqp::Protocol::k_WORD_SIZE
                                               : sizeof(bmqp::SubQueueInfo);
                const int    optionsSize = sizeof(bmqp::OptionHeader) +
                                        (D.d_subQueueInfos.size() * itemSize);
                subQueueWords = optionsSize / bmqp::Protocol::k_WORD_SIZE;
            }
        }

        typedef bmqp::OptionUtil::OptionMeta OptionMeta;
        const bool       hasMsgGroupId = !D.d_msgGroupId.isNull();
        const OptionMeta msgGroupIdOption =
            hasMsgGroupId ? OptionMeta::forOptionWithPadding(
                                bmqp::OptionType::e_MSG_GROUP_ID,
                                D.d_msgGroupId.value().length())
                          : OptionMeta::forNullOption();
        const int msgGroupIdWords = hasMsgGroupId
                                        ? (msgGroupIdOption.size() /
                                           bmqp::Protocol::k_WORD_SIZE)
                                        : 0;

        // Push Header
        bmqp::PushHeader ph;
        ph.setOptionsWords(subQueueWords + msgGroupIdWords);
        ph.setHeaderWords(sizeof(bmqp::PushHeader) /
                          bmqp::Protocol::k_WORD_SIZE);
        ph.setQueueId(D.d_qid);
        ph.setMessageGUID(D.d_guid);
        int       padding = 0;
        const int paddedPayloadNumWords =
            bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding,
                                                       D.d_payload.length());
        ph.setMessageWords(ph.headerWords() + ph.optionsWords() +
                           paddedPayloadNumWords);

        eventLength += (ph.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        // Write Push Header
        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&ph),
                                ph.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Write options
        if (hasSubQueueInfo) {
            // Option Header
            bmqp::OptionHeader oh;
            oh.setType(D.d_useOldSubQueueIds
                           ? bmqp::OptionType::e_SUB_QUEUE_IDS_OLD
                           : bmqp::OptionType::e_SUB_QUEUE_INFOS);
            if (isPackedOptions) {
                oh.setPacked(true).setWords(
                    D.d_subQueueInfos[0].rdaInfo().internalRepresentation());
            }
            else {
                oh.setWords(subQueueWords)
                    .setTypeSpecific(sizeof(bmqp::SubQueueInfo) /
                                     bmqp::Protocol::k_WORD_SIZE);
            }

            // Write option header
            bdlbb::BlobUtil::append(blob,
                                    reinterpret_cast<const char*>(&oh),
                                    sizeof(oh));

            // Write SubQueueInfos (option)
            if (!isPackedOptions) {
                bdlbb::BlobUtil::append(
                    blob,
                    reinterpret_cast<const char*>(D.d_subQueueInfos.data()),
                    subQueueWords * bmqp::Protocol::k_WORD_SIZE);
            }
        }
        if (hasMsgGroupId) {
            // Option Header
            bmqp::OptionUtil::OptionsBox options;
            options.add(blob, D.d_msgGroupId.value().data(), msgGroupIdOption);
        }

        // Write message payload
        bdlbb::BlobUtil::append(blob, D.d_payload);
        bmqp::ProtocolUtil::appendPadding(blob, D.d_payload.length());
    }

    // Set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
    eh->setLength(eventLength);
}

/// Populate specified `blob` with a PUSH event which has specified
/// `numMsgs` PUSH messages, update specified `eh` with corresponding
/// EventHeader, and update specified `vec` with the expected values.
void populateBlob(bdlbb::Blob*              blob,
                  bmqp::EventHeader*        eh,
                  bsl::vector<Data>*        vec,
                  size_t                    numMsgs,
                  bdlbb::BlobBufferFactory* bufferFactory,
                  bool                      implicitAppData,
                  bool                      zeroLengthMsgs,
                  bslma::Allocator*         allocator)
{
    // Create guid from valid hex rep

    // Above hex string represents a valid guid with these values:
    // TS = bdlb::BigEndianUint64::make(12345)
    // IP = 98765
    // ID = 9999
    const char HEX_REP[] = "0000000000003039CD8101000000270F";

    int seed        = bsl::numeric_limits<int>::max();
    int eventLength = sizeof(bmqp::EventHeader);

    eh->setType(bmqp::EventType::e_PUSH);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);
    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    for (size_t i = 0; i < numMsgs; ++i) {
        Data        data(bufferFactory, allocator);
        bdlbb::Blob properties(bufferFactory, allocator);
        bdlbb::Blob payload(bufferFactory, allocator);
        int         blobSize = bdlb::Random::generate15(&seed) + 1;
        // avoid value of zero
        int propAreaSize = blobSize / 2 + 13 +  // some random value
                           sizeof(bmqp::MessagePropertyHeader) +
                           sizeof(bmqp::MessagePropertiesHeader);
        int optionsSize      = 0;
        int optionsWords     = 0;
        int numSubQueueInfos = i % 5;
        // Number of subQueueInfos vary from 0-4

        data.d_flags = 0;  // explicitly initialize to zero

        // Options
        bmqp::OptionHeader              oh;
        bsl::vector<bmqp::SubQueueInfo> subQueueInfos(allocator);
        if (numSubQueueInfos > 0) {
            // We need to write an option header followed by option payload.
            optionsSize = sizeof(bmqp::OptionHeader) +
                          (numSubQueueInfos * sizeof(bmqp::SubQueueInfo));
            optionsWords = optionsSize / bmqp::Protocol::k_WORD_SIZE;

            oh.setType(bmqp::OptionType::e_SUB_QUEUE_INFOS);
            oh.setWords(optionsWords);

            generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);
        }

        bool isAppDataImplicit = false;
        if (implicitAppData && (0 == ((i + 1) % 7))) {
            // If implicit app data msgs can be added, add one every 7th msg.
            bmqp::PushHeaderFlagUtil::setFlag(
                &data.d_flags,
                bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);
            blobSize          = 0;
            isAppDataImplicit = true;
            propAreaSize      = 0;
        }

        int                           padding             = 0;
        int                           paddedPropsNumWords = 0;
        bmqp::MessagePropertiesHeader mph;
        bsl::vector<char>             props(allocator);

        if (!isAppDataImplicit) {
            // We need to have a valid 'bmqp::MessagePropertiesHeader' because
            // bmqp::PushMsgIter::next()' validates it.  It does not validate
            // the each individual 'bmqp::MessagePropertyHeader', so we can
            // put anything as long as the length of the properties area is
            // correct.

            paddedPropsNumWords = bmqp::ProtocolUtil::calcNumWordsAndPadding(
                &padding,
                propAreaSize);
            mph.setHeaderSize(sizeof(mph));
            mph.setMessagePropertyHeaderSize(
                sizeof(bmqp::MessagePropertyHeader));
            mph.setNumProperties(4);  // This field doesn't matter
            mph.setMessagePropertiesAreaWords(paddedPropsNumWords);
            bdlbb::BlobUtil::append(&properties,
                                    reinterpret_cast<const char*>(&mph),
                                    sizeof(mph));
            props.resize(static_cast<size_t>(propAreaSize) - sizeof(mph), 'x');
            bdlbb::BlobUtil::append(&properties, props.data(), props.size());
            BSLS_ASSERT_OPT(properties.length() == propAreaSize);
            bmqp::ProtocolUtil::appendPadding(&properties, propAreaSize);
            // Message properties area is word aligned.
            BSLS_ASSERT_OPT(properties.length() == propAreaSize + padding);
        }

        if (zeroLengthMsgs && (0 == (i % 5))) {
            // If zero-length msgs can be added, add one every 5th msg.
            blobSize = 0;
        }

        bsl::vector<char> blobData(blobSize, 'a', allocator);
        bdlbb::BlobUtil::append(&payload, blobData.data(), blobSize);

        data.d_payload    = payload;
        data.d_properties = properties;
        bdlbb::BlobUtil::append(&properties, payload);
        data.d_appData = properties;
        data.d_queueId = blobSize;
        data.d_guid.fromHex(HEX_REP);  // TBD use new guid every time
        data.d_msgLen  = blobSize;
        data.d_propLen = paddedPropsNumWords * bmqp::Protocol::k_WORD_SIZE;
        // Recall that properties' length retrieved from the iterator
        // includes padding.
        data.d_optionsSize = optionsSize;

        // PushHeader
        bmqp::PushHeader ph;
        ph.setOptionsWords(optionsWords);
        ph.setHeaderWords(sizeof(bmqp::PushHeader) /
                          bmqp::Protocol::k_WORD_SIZE);
        ph.setQueueId(data.d_queueId);
        ph.setMessageGUID(data.d_guid);
        padding = 0;
        const int paddedPayloadNumWords =
            bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding, blobSize);
        ph.setMessageWords(ph.headerWords() + ph.optionsWords() +
                           paddedPropsNumWords + paddedPayloadNumWords);

        // Set the 'e_MESSAGE_PROPERTIES' flag.

        bmqp::PushHeaderFlagUtil::setFlag(
            &data.d_flags,
            bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES);
        ph.setFlags(data.d_flags);

        eventLength += (ph.messageWords() * bmqp::Protocol::k_WORD_SIZE);

        // Write PUSH header to blob.

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&ph),
                                ph.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Options
        if (numSubQueueInfos > 0) {
            // Write option header
            mwcu::BlobUtil::reserve(&data.d_optionsPosition, blob, sizeof(oh));

            mwcu::BlobUtil::writeBytes(blob,
                                       data.d_optionsPosition,
                                       reinterpret_cast<const char*>(&oh),
                                       sizeof(oh));

            // Write options
            bdlbb::BlobUtil::append(
                blob,
                reinterpret_cast<const char*>(subQueueInfos.data()),
                numSubQueueInfos * sizeof(bmqp::SubQueueInfo));
        }

        if (!isAppDataImplicit) {
            // Write 'bmqp::MessagePropertiesHeader'.

            BSLS_ASSERT_OPT(0 < propAreaSize);

            bdlbb::BlobUtil::append(blob,
                                    reinterpret_cast<const char*>(&mph),
                                    sizeof(mph));

            // Write the properties area.

            bdlbb::BlobUtil::append(blob, props.data(), props.size());

            bmqp::ProtocolUtil::appendPadding(blob, propAreaSize);
            // Message properties area is word aligned.

            // Write message payload.

            bdlbb::BlobUtil::append(blob, blobData.data(), blobSize);
        }

        // Padding needs to be there all the time, even if 'blobSize' == 0 (ie,
        // 'isAppDataImplicit' is true).  In this case, padding will be 4
        // bytes.

        bmqp::ProtocolUtil::appendPadding(blob, blobSize);

        vec->push_back(data);
    }

    // set EventHeader length
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        blob->buffer(0).data());
    e->setLength(eventLength);
}

void populateBlob(bdlbb::Blob*             blob,
                  bmqp::EventHeader*       eh,
                  bdlbb::Blob*             eb,     // expected appData
                  int*                     ebLen,  // expected appDataLen
                  mwcu::BlobPosition*      headerPosition,
                  mwcu::BlobPosition*      payloadPosition,
                  int                      queueId,
                  const bmqt::MessageGUID& guid,
                  bmqt::CompressionAlgorithmType::Enum cat,
                  bdlbb::Blob*                         compressedEb,
                  bdlbb::BlobBufferFactory*            bufferFactory,
                  bslma::Allocator*                    allocator)
{
    // Payload is 36 bytes. Per BlazingMQ protocol, it will require 4 bytes of
    // padding (ie 1 word)
    const char* payload = "abcdefghijklmnopqrstuvwxyz1234567890";  // 36

    *ebLen = bsl::strlen(payload);

    bdlbb::BlobUtil::append(eb, payload, *ebLen);
    mwcu::MemOutStream error(allocator);
    bdlbb::Blob        compressedBlob(bufferFactory, allocator);
    int                payloadLength = bsl::strlen(payload);
    bmqp::Compression::compress(&compressedBlob,
                                bufferFactory,
                                cat,
                                payload,
                                payloadLength,
                                &error,
                                allocator);
    bdlbb::BlobUtil::append(compressedEb, compressedBlob);

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

    int payloadLenWords = compressedBlob.length() /
                          bmqp::Protocol::k_WORD_SIZE;

    // PushHeader
    bmqp::PushHeader ph;
    ph.setOptionsWords(0);
    ph.setHeaderWords(sizeof(bmqp::PushHeader) / bmqp::Protocol::k_WORD_SIZE);
    ph.setQueueId(queueId);
    ph.setMessageGUID(guid);
    ph.setMessageWords(ph.headerWords() + payloadLenWords);
    ph.setCompressionAlgorithmType(cat);

    int eventLength = sizeof(bmqp::EventHeader) +
                      ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

    // EventHeader
    eh->setLength(eventLength);
    eh->setType(bmqp::EventType::e_PUSH);
    eh->setHeaderWords(sizeof(bmqp::EventHeader) /
                       bmqp::Protocol::k_WORD_SIZE);

    bdlbb::BlobUtil::append(blob,
                            reinterpret_cast<const char*>(eh),
                            eh->headerWords() * bmqp::Protocol::k_WORD_SIZE);

    // Capture PushHeader position
    mwcu::BlobUtil::reserve(headerPosition,
                            blob,
                            ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    mwcu::BlobUtil::writeBytes(blob,
                               *headerPosition,
                               reinterpret_cast<const char*>(&ph),
                               ph.headerWords() * bmqp::Protocol::k_WORD_SIZE);

    const int payloadOffset = blob->length();
    bdlbb::BlobUtil::append(blob, compressedBlob);
    mwcu::BlobUtil::findOffset(payloadPosition, *blob, payloadOffset);
}

void breathingTestHelper(
    bool                                 decompressFlag,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType,
    bdlbb::BlobBufferFactory*            bufferFactory_p)
{
    // Create valid iter
    bdlbb::Blob        blob(bufferFactory_p, s_allocator_p);
    bdlbb::Blob        expectedBlob(bufferFactory_p, s_allocator_p);
    bdlbb::Blob        expectedCompressedBlob(bufferFactory_p, s_allocator_p);
    int                expectedBlobLength = 0;
    mwcu::BlobPosition expectedHeaderPos;
    mwcu::BlobPosition expectedPayloadPos;
    mwcu::BlobPosition retrievedPayloadPos;
    bdlbb::Blob        retrievedPayloadBlob(s_allocator_p);

    // Populate blob
    const int         queueId = 123;
    bmqt::MessageGUID guid;
    bmqp::EventHeader eventHeader;

    populateBlob(&blob,
                 &eventHeader,
                 &expectedBlob,
                 &expectedBlobLength,
                 &expectedHeaderPos,
                 &expectedPayloadPos,
                 queueId,
                 guid,
                 compressionAlgorithmType,
                 &expectedCompressedBlob,
                 bufferFactory_p,
                 s_allocator_p);

    // Iterate and verify
    bmqp::PushMessageIterator iter(&blob,
                                   eventHeader,
                                   decompressFlag,
                                   bufferFactory_p,
                                   s_allocator_p);

    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(true, iter.next());
    ASSERT_EQ(queueId, iter.header().queueId());
    ASSERT_EQ(guid, iter.header().messageGUID());

    ASSERT_EQ(false, iter.hasMessageProperties());
    ASSERT_EQ(false, iter.hasOptions());
    ASSERT_EQ(0, iter.optionsSize());

    if (decompressFlag) {
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());

        bmqp::MessageProperties emptyProps(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob,
                                           expectedBlob));
    }

    bmqp::OptionsView emptyOptionsView(s_allocator_p);
    ASSERT_EQ(0, iter.loadOptionsView(&emptyOptionsView));
    ASSERT_EQ(true, emptyOptionsView.isValid());

    ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos));
    ASSERT_EQ(retrievedPayloadPos, expectedPayloadPos);

    retrievedPayloadBlob.removeAll();
    ASSERT_EQ(0, iter.loadApplicationData(&retrievedPayloadBlob));
    ASSERT_GT(retrievedPayloadBlob.length(), 0);

    // expect decompressed payload if decompressFlag is true and vice-versa.

    if (decompressFlag) {
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(expectedBlob,
                                           retrievedPayloadBlob));
        ASSERT_EQ(retrievedPayloadBlob.length(), expectedBlob.length());
    }
    else {
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(expectedCompressedBlob,
                                           retrievedPayloadBlob));
        ASSERT_EQ(retrievedPayloadBlob.length(),
                  expectedCompressedBlob.length());
    }

    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(false, iter.next());
    ASSERT_EQ(false, iter.isValid());

    // Copy
    bmqp::PushMessageIterator iter2(iter, s_allocator_p);
    ASSERT_EQ(false, iter2.isValid());

    // Clear
    iter.clear();
    ASSERT_EQ(false, iter.isValid());
    ASSERT_EQ(false, iter2.isValid());

    // Assign
    iter = iter2;
    ASSERT_EQ(false, iter.isValid());
    ASSERT_EQ(false, iter2.isValid());

    // Reset, iterate and verify again
    iter.reset(&blob, eventHeader, decompressFlag);
    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(true, iter.next());
    ASSERT_EQ(queueId, iter.header().queueId());
    ASSERT_EQ(guid, iter.header().messageGUID());
    ASSERT_EQ(false, iter.hasMessageProperties());
    ASSERT_EQ(false, iter.hasOptions());
    ASSERT_EQ(0, iter.optionsSize());
    ASSERT_EQ(0, iter.loadOptionsView(&emptyOptionsView));
    ASSERT_EQ(true, emptyOptionsView.isValid());

    if (decompressFlag) {
        ASSERT_EQ(expectedBlobLength, iter.messagePayloadSize());
        ASSERT_EQ(expectedBlobLength, iter.applicationDataSize());
        ASSERT_EQ(0, iter.messagePropertiesSize());

        bdlbb::Blob emptyBlob(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyBlob));
        ASSERT_EQ(0, emptyBlob.length());

        bmqp::MessageProperties emptyProps(s_allocator_p);
        ASSERT_EQ(0, iter.loadMessageProperties(&emptyProps));
        ASSERT_EQ(0, emptyProps.numProperties());

        bdlbb::Blob retrievedPayloadBlob2(s_allocator_p);

        ASSERT_EQ(0, iter.loadMessagePayload(&retrievedPayloadBlob2));
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(retrievedPayloadBlob2,
                                           expectedBlob));
        retrievedPayloadBlob2.removeAll();
    }

    mwcu::BlobPosition retrievedPayloadPos2;
    ASSERT_EQ(0, iter.loadApplicationDataPosition(&retrievedPayloadPos2));
    ASSERT_EQ(retrievedPayloadPos2, expectedPayloadPos);

    bdlbb::Blob retrievedApplicationData(s_allocator_p);
    ASSERT_EQ(0, iter.loadApplicationData(&retrievedApplicationData));

    // expect decompressed payload if decompressFlag is true and vice-versa.

    if (decompressFlag) {
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(expectedBlob,
                                           retrievedApplicationData));
        ASSERT_EQ(retrievedApplicationData.length(), expectedBlob.length());
    }
    else {
        ASSERT_EQ(0,
                  bdlbb::BlobUtil::compare(expectedCompressedBlob,
                                           retrievedApplicationData));
        ASSERT_EQ(retrievedApplicationData.length(),
                  expectedCompressedBlob.length());
    }

    ASSERT_EQ(true, iter.isValid());
    ASSERT_EQ(false, iter.next());
    ASSERT_EQ(false, iter.isValid());
}
}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   - Create invalid iterator and assert validity.
//   - Create invalid iterator from an existing invalid iterator and check
//     the validity for both.
//   - Assign an invalid iterator and check the validity.
//   - Create a valid iterator which does not allow decompression i.e. the
//     decompress flag is set to false. The iterator has messages which
//     have not been compressed.
//   - Create a valid iterator which does not allow decompression i.e. the
//     decompress flag is set to false. The iterator has messages which
//     have been compressed using zlib algorithm.
//   - Create a valid iterator which allows decompression i.e. the
//     decompress flag is set to true. The iterator has messages which
//     have not been compressed.
//   - Create a valid iterator which allows decompression i.e. the
//     decompress flag is set to true. The iterator has messages which
//     have been compressed using ZLIB compression algorithm.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);

    {
        // Create invalid iter
        bmqp::PushMessageIterator iter(&bufferFactory, s_allocator_p);
        ASSERT_EQ(false, iter.isValid());
    }

    {
        // Create invalid iter from another invalid iter
        bmqp::PushMessageIterator iter1(&bufferFactory, s_allocator_p);
        bmqp::PushMessageIterator iter2(iter1, s_allocator_p);

        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        // Assigning invalid iter
        bmqp::PushMessageIterator iter1(&bufferFactory, s_allocator_p),
            iter2(&bufferFactory, s_allocator_p);
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());

        iter1 = iter2;
        ASSERT_EQ(false, iter1.isValid());
        ASSERT_EQ(false, iter2.isValid());
    }

    {
        PV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_NONE COMPRESSION");
        breathingTestHelper(true,  // decompress flag
                            bmqt::CompressionAlgorithmType::e_NONE,
                            &bufferFactory);
    }

    {
        PV("CREATE VALID ITER DECOMPRESSFLAG FALSE, E_ZLIB COMPRESSION");
        breathingTestHelper(false,  // decompress flag
                            bmqt::CompressionAlgorithmType::e_ZLIB,
                            &bufferFactory);
    }

    {
        PV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_NONE COMPRESSION");
        breathingTestHelper(true,  // decompress flag
                            bmqt::CompressionAlgorithmType::e_NONE,
                            &bufferFactory);
    }

    {
        PV("CREATE VALID ITER DECOMPRESSFLAG TRUE, E_ZLIB COMPRESSION");
        breathingTestHelper(true,  // decompress flag
                            bmqt::CompressionAlgorithmType::e_ZLIB,
                            &bufferFactory);
    }
}

static void test2_iteratorReset()
{
    mwctst::TestHelper::printTestName("ITERATOR RESET");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bmqp::PushMessageIterator      pmt(&bufferFactory, s_allocator_p);
    bdlbb::Blob                    copiedBlob(s_allocator_p);
    bdlbb::Blob                    expectedBlob(&bufferFactory, s_allocator_p);
    bdlbb::Blob        expectedCompressedBlob(&bufferFactory, s_allocator_p);
    int                expectedBlobLength = 0;
    mwcu::BlobPosition headerPosition;
    mwcu::BlobPosition payloadPosition;
    const int          queueId = 123;
    bmqt::MessageGUID  guid;
    bmqp::EventHeader  eventHeader;
    bdlbb::Blob        payloadBlob(s_allocator_p);
    bdlbb::Blob        appDataBlob(s_allocator_p);

    {
        bdlbb::Blob blob(&bufferFactory, s_allocator_p);

        // Populate blob
        populateBlob(&blob,
                     &eventHeader,
                     &expectedBlob,
                     &expectedBlobLength,
                     &headerPosition,
                     &payloadPosition,
                     queueId,
                     guid,
                     bmqt::CompressionAlgorithmType::e_ZLIB,
                     &expectedCompressedBlob,
                     &bufferFactory,
                     s_allocator_p);

        bmqp::PushMessageIterator iter(&blob,
                                       eventHeader,
                                       true,  // decompress flag
                                       &bufferFactory,
                                       s_allocator_p);

        ASSERT_EQ(true, iter.isValid());

        // Copy 'blob' into 'copiedBlob'. Reset 'pmt' with 'copiedBlob'
        // and 'iter'

        copiedBlob = blob;
        pmt.reset(&copiedBlob, iter);
    }

    // Iterate and verify
    ASSERT_EQ(true, pmt.isValid());
    ASSERT_EQ(true, pmt.next());
    ASSERT_EQ(queueId, pmt.header().queueId());
    ASSERT_EQ(guid, pmt.header().messageGUID());
    ASSERT_EQ(expectedBlobLength, pmt.messagePayloadSize());
    ASSERT_EQ(expectedBlobLength, pmt.applicationDataSize());

    ASSERT_EQ(0, pmt.loadMessagePayload(&payloadBlob));
    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payloadBlob, expectedBlob));

    ASSERT_EQ(0, pmt.loadApplicationData(&appDataBlob));
    ASSERT_EQ(0, bdlbb::BlobUtil::compare(appDataBlob, expectedBlob));

    ASSERT_EQ(true, pmt.isValid());
    ASSERT_EQ(false, pmt.next());
    ASSERT_EQ(false, pmt.isValid());
}

/// Test iterating over PUSH event having *NO* PUSH messages
static void test3_iteratePushEventHavingNoMessages()
{
    mwctst::TestHelper::printTestName(
        "ITERATE PUSH EVENT HAVING NO PUSH MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;

    populateBlob(&eventBlob,
                 &eventHeader,
                 &data,
                 0,
                 &bufferFactory,
                 false,  // No zero-length msgs
                 false,  // No implicit app data
                 s_allocator_p);

    // Verify non-validity
    bmqp::PushMessageIterator iter(&bufferFactory, s_allocator_p);
    ASSERT_LT(iter.reset(&eventBlob, eventHeader, true), 0);
    ASSERT_EQ(false, iter.isValid());
}

/// Test iterating over invalid PUSH event (having a PUSH message, but not
/// enough bytes in the blob).
static void test4_iterateInvalidPushEvent()
{
    mwctst::TestHelper::printTestName("ITERATE INVALID PUSH EVENT");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);

    bsl::vector<Data> data(s_allocator_p);
    bmqp::EventHeader eventHeader;

    populateBlob(&eventBlob,
                 &eventHeader,
                 &data,
                 2,
                 &bufferFactory,
                 false,  // No zero-length msgs
                 false,  // No implicit app data
                 s_allocator_p);

    // Render the blob invalid by removing it's last byte
    bdlbb::BlobUtil::erase(&eventBlob, eventBlob.length() - 1, 1);

    // Verify
    bmqp::PushMessageIterator iter(&eventBlob,
                                   eventHeader,
                                   true,  // decompress flag
                                   &bufferFactory,
                                   s_allocator_p);
    ASSERT_EQ(true, iter.isValid());

    // First message is valid..
    ASSERT_EQ(1, iter.next());
    // Second message is error..
    int rc = iter.next();
    ASSERT_LT(rc, 0);
    bsl::cout << "Error returned: " << rc << bsl::endl;
    iter.dumpBlob(bsl::cout);
}

static void test5_iteratePushEventHavingMultipleMessages()
{
    mwctst::TestHelper::printTestName("PUSH EVENT HAVING MULTIPLE MESSAGES");
    // Test iterating over PUSH event having multiple PUSH messages

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              data(s_allocator_p);
    bmqp::EventHeader              eventHeader;
    const size_t                   k_NUM_MSGS = 1000;

    populateBlob(&eventBlob,
                 &eventHeader,
                 &data,
                 k_NUM_MSGS,
                 &bufferFactory,
                 false,  // No zero-length PUSH msgs.
                 true,   // make some PUSH msgs w/ implicit app data
                 s_allocator_p);

    // Iterate and verify
    bmqp::PushMessageIterator iter(&eventBlob,
                                   eventHeader,
                                   true,  // decompress flag
                                   &bufferFactory,
                                   s_allocator_p);
    ASSERT_EQ(true, iter.isValid());

    size_t index = 0;

    while ((1 == iter.next()) && index < data.size()) {
        const Data& D = data[index];
        ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());
        ASSERT_EQ_D(index, D.d_guid, iter.header().messageGUID());

        const bool isAppDataImplicit = bmqp::PushHeaderFlagUtil::isSet(
            iter.header().flags(),
            bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);
        ASSERT_EQ_D(index,
                    isAppDataImplicit,
                    iter.isApplicationDataImplicit());

        const bool hasMsgProps = bmqp::PushHeaderFlagUtil::isSet(
            iter.header().flags(),
            bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES);
        ASSERT_EQ_D(index, true, hasMsgProps);
        ASSERT_EQ_D(index, hasMsgProps, iter.hasMessageProperties());
        ASSERT_EQ_D(index, D.d_propLen, iter.messagePropertiesSize());

        const bool hasOptions = iter.header().optionsWords() > 0;
        ASSERT_EQ_D(index, hasOptions, iter.hasOptions());
        ASSERT_EQ_D(index, D.d_optionsSize, iter.optionsSize());
        ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
        ASSERT_EQ_D(index,
                    (D.d_msgLen + D.d_propLen),
                    iter.applicationDataSize());

        bdlbb::Blob        props(s_allocator_p);
        bdlbb::Blob        payload(s_allocator_p);
        bdlbb::Blob        appData(s_allocator_p);
        mwcu::BlobPosition propsPos;
        mwcu::BlobPosition payloadPos;
        mwcu::BlobPosition appDataPos;

        // Below, we are relying on the imp detail when we check for rc of
        // '-1' for various 'load*' routines when 'isAppDataImplicit' flag
        // is true.  If implementation is updated to return a different rc,
        // we will need to update it here as well. Alternatively, instead
        // of specifically checking for '-1', we could just check for a
        // non-zero rc.

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationDataPosition(&propsPos));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationDataPosition(&appDataPos));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadMessageProperties(&props));

        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(props, D.d_properties));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadMessagePayload(&payload));

        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(payload, D.d_payload));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationData(&appData));
        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(appData, D.d_appData));
        ++index;
    }

    ASSERT_EQ(index, data.size());
    ASSERT_EQ(false, iter.isValid());
}

/// Test iterating over PUSH event containing one or more zero-length PUSH
/// messages.
static void test6_iteratePushEventHavingZeroLengthMessages()
{
    mwctst::TestHelper::printTestName(
        "PUSH EVENT HAVING ZERO-LENGTH MESSAGES");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);
    bsl::vector<Data>              data(s_allocator_p);
    bmqp::EventHeader              eventHeader;
    const size_t                   k_NUM_MSGS = 1000;

    populateBlob(&eventBlob,
                 &eventHeader,
                 &data,
                 k_NUM_MSGS,
                 &bufferFactory,
                 true,  // make some PUSH msgs zero-length
                 true,  // make some PUSH msgs w/ implicit app data
                 s_allocator_p);

    // Iterate and verify
    bmqp::PushMessageIterator iter(&eventBlob,
                                   eventHeader,
                                   true,  // decompress flag
                                   &bufferFactory,
                                   s_allocator_p);
    ASSERT_EQ(true, iter.isValid());

    size_t index = 0;

    while (iter.next() == 1 && index < data.size()) {
        const Data& D = data[index];

        ASSERT_EQ_D(index, D.d_queueId, iter.header().queueId());
        ASSERT_EQ_D(index, D.d_guid, iter.header().messageGUID());

        const bool isAppDataImplicit = bmqp::PushHeaderFlagUtil::isSet(
            iter.header().flags(),
            bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD);
        ASSERT_EQ_D(index,
                    isAppDataImplicit,
                    iter.isApplicationDataImplicit());

        const bool hasMsgProps = bmqp::PushHeaderFlagUtil::isSet(
            iter.header().flags(),
            bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES);
        ASSERT_EQ_D(index, true, hasMsgProps);
        ASSERT_EQ_D(index, hasMsgProps, iter.hasMessageProperties());
        ASSERT_EQ_D(index, D.d_propLen, iter.messagePropertiesSize());

        const bool hasOptions = iter.header().optionsWords() > 0;
        ASSERT_EQ_D(index, hasOptions, iter.hasOptions());
        ASSERT_EQ_D(index, D.d_optionsSize, iter.optionsSize());

        ASSERT_EQ_D(index, D.d_msgLen, iter.messagePayloadSize());
        ASSERT_EQ_D(index,
                    (D.d_msgLen + D.d_propLen),
                    iter.applicationDataSize());

        bdlbb::Blob        props(s_allocator_p);
        bdlbb::Blob        payload(s_allocator_p);
        bdlbb::Blob        appData(s_allocator_p);
        mwcu::BlobPosition propsPos;
        mwcu::BlobPosition payloadPos;
        mwcu::BlobPosition appDataPos;

        // Below, we are relying on the imp detail when we check for rc of
        // '-1' for various 'load*' routines when 'isAppDataImplicit' flag
        // is true.  If implementation is updated to return a different rc,
        // we will need to update it here as well. Alternatively, instead
        // of specifically checking for '-1', we could just check for a
        // non-zero rc.

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationDataPosition(&propsPos));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationDataPosition(&appDataPos));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadMessageProperties(&props));
        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(props, D.d_properties));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadMessagePayload(&payload));

        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(payload, D.d_payload));

        ASSERT_EQ_D(index,
                    isAppDataImplicit ? -1 : 0,
                    iter.loadApplicationData(&appData));
        ASSERT_EQ_D(index, 0, bdlbb::BlobUtil::compare(appData, D.d_appData));
        ++index;
    }

    ASSERT_EQ(index, data.size());
    ASSERT_EQ(false, iter.isValid());
}

static void test7_extractOptions()
// ------------------------------------------------------------------------
// EXTRACT OPTIONS
//
// Concerns:
//   If we have a valid iterator associated with a flattened event
//   (i.e. an event having messages with at most one subQueueId), then
//   a) extracting the queue info returns a queueId having either the
//   subQueueId of the message currently pointed to by the iterator or the
//   default subQueueId, as well as the expected RDA counter, and b)
//   extracting msgGroupId (if available) returns the expected Group Id.
//
// Plan:
//   1) Create an event composed of six messages.  Msg1 and Msg2 have
//      the same id but a different subQueueId, Msg3 has no subQueueId,
//      Msg4 has one subQueueId and one groupId, Msg 5 has no subQueueId but
//      one GroupId, Msg 6 has one subQueueId encoded using old SubQueueId
//      options.
//   2) Extract the queue infos for each message, and verify that the
//      correct subQueueIds were extracted for Msg1, Msg2 Msg 4 & Msg 6,
//      the default subQueueId was extracted for Msg3 & Msg 5.  Also verify
//      the correct RDA counters were extracted for all messages.
//  3)  Extract Group Ids for each message, and verify that the values are
//      as expected.
//
// Testing:
//   Extracting queueId, RDA counter and/or msgGroupId from the message of
//   a flattened PUSH event.
//   - extractQueueInfo(...)
//   - extractMsgGroupId(...)
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("EXTRACT OPTIONS");

    bdlbb::PooledBlobBufferFactory bufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    eventBlob(&bufferFactory, s_allocator_p);
    bsl::vector<Data1>             data(s_allocator_p);
    bmqp::EventHeader              eventHeader;
    int                            qid;
    bool                           hasSubQueueInfo   = false;
    bool                           useOldSubQueueIds = false;
    bool                           hasMsgGroupId     = false;
    int                            payloadLength     = 0;

    // Msg1: One SubQueueId
    qid               = 1;
    hasSubQueueInfo   = true;
    useOldSubQueueIds = false;
    hasMsgGroupId     = false;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Msg2: One SubQueueId
    qid               = 1;
    hasSubQueueInfo   = true;
    useOldSubQueueIds = false;
    hasMsgGroupId     = false;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Msg3: No SubQueueId and no GroupId
    qid               = 1;
    hasSubQueueInfo   = false;
    useOldSubQueueIds = false;
    hasMsgGroupId     = false;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Msg4: One SubQueueId and one GroupId
    qid               = 2;
    hasSubQueueInfo   = true;
    useOldSubQueueIds = false;
    hasMsgGroupId     = true;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Msg5: No SubQueueId and one GroupId
    qid               = 1;
    hasSubQueueInfo   = false;
    useOldSubQueueIds = false;
    hasMsgGroupId     = true;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Msg 6: One subQueueId encoded using old SubQueueId options
    qid               = 1;
    hasSubQueueInfo   = true;
    useOldSubQueueIds = true;
    hasMsgGroupId     = false;
    payloadLength     = generateRandomInteger(1, 120);
    appendDatum1(&data,
                 qid,
                 hasSubQueueInfo,
                 useOldSubQueueIds,
                 hasMsgGroupId,
                 payloadLength,
                 &bufferFactory,
                 s_allocator_p);

    // Build the event blob
    appendMessages1(&eventHeader,
                    &eventBlob,
                    data,
                    &bufferFactory,
                    s_allocator_p);

    // Iterate and verify
    bmqp::PushMessageIterator iter(&eventBlob,
                                   eventHeader,
                                   true,  // decompress flag
                                   &bufferFactory,
                                   s_allocator_p);
    ASSERT_EQ(iter.isValid(), true);

    size_t index = 0;
    while (iter.next() == 1 && index < data.size()) {
        const Data1& D = data[index];

        ASSERT_EQ_D(index, D.d_qid, iter.header().queueId());

        BSLS_ASSERT_OPT(D.d_qid != -1);
        int           queueId(-1);
        bmqp::RdaInfo rdaInfo;
        unsigned int  subscriptionId;

        iter.extractQueueInfo(&queueId, &subscriptionId, &rdaInfo);
        if (D.d_subQueueInfos.size() == 0) {
            PV("Expected: " << D.d_qid << ", Actual: " << queueId);
            const unsigned int expected =
                bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;
            ASSERT_EQ_D(index, queueId, D.d_qid);
            ASSERT_EQ_D(index, subscriptionId, expected);
        }
        else {
            BSLS_ASSERT_OPT(D.d_subQueueInfos.size() == 1);

            PV("Expected: " << D.d_qid << ", Actual: " << queueId);

            ASSERT_EQ_D(index, queueId, D.d_qid);
            ASSERT_EQ_D(index, subscriptionId, D.d_subQueueInfos[0].id());
        }
        if (D.d_useOldSubQueueIds) {
            ASSERT_EQ_D(index, rdaInfo.isUnlimited(), true);
        }
        else {
            ASSERT_EQ_D(index,
                        rdaInfo.counter(),
                        D.d_subQueueInfos[0].rdaInfo().counter());
        }

        const bool expectedToHaveMsgGroupId = !D.d_msgGroupId.isNull();
        const bool actualHavingMsgGroupId   = iter.hasMsgGroupId();

        PV("Expected to have: " << bsl::boolalpha << expectedToHaveMsgGroupId
                                << ", actually have: " << bsl::boolalpha
                                << actualHavingMsgGroupId);
        if (expectedToHaveMsgGroupId != actualHavingMsgGroupId) {
            ASSERT_EQ_D(index,
                        expectedToHaveMsgGroupId,
                        actualHavingMsgGroupId);
        }
        else if (actualHavingMsgGroupId) {
            bmqp::Protocol::MsgGroupId        actual(s_allocator_p);
            const bmqp::Protocol::MsgGroupId& expected =
                D.d_msgGroupId.value();
            iter.extractMsgGroupId(&actual);

            PV("Expected: " << expected << ", Actual: " << queueId);

            ASSERT_EQ_D(index, actual, expected);
        }

        ++index;
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD:
    // figure out the right way to "fix" this.
    Data dummy(static_cast<bdlbb::BlobBufferFactory*>(0), s_allocator_p);
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data, bslma::UsesBslmaAllocator> >(
            dummy));
    Data1 dummy1(static_cast<bdlbb::BlobBufferFactory*>(0), s_allocator_p);
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data1, bslma::UsesBslmaAllocator> >(
            dummy1));

    bmqp::ProtocolUtil::initialize(s_allocator_p);

    unsigned int seed = bsl::time(0);
    bsl::srand(seed);
    PV("Seed: " << seed);

    switch (_testCase) {
    case 0:
    case 7: test7_extractOptions(); break;
    case 6: test6_iteratePushEventHavingZeroLengthMessages(); break;
    case 5: test5_iteratePushEventHavingMultipleMessages(); break;
    case 4: test4_iterateInvalidPushEvent(); break;
    case 3: test3_iteratePushEventHavingNoMessages(); break;
    case 2: test2_iteratorReset(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
