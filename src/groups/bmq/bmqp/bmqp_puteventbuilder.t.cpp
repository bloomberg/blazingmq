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

// bmqp_puteventbuilder.t.cpp                                         -*-C++-*-
#include <bmqp_puteventbuilder.h>

// BMQ
#include <bmqp_compression.h>
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocolutil.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_puttester.h>
#include <bmqt_messageguid.h>

#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_guid.h>
#include <bdlb_guidutil.h>
#include <bdlb_random.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>  // for bsl::strlen
#include <bsl_fstream.h>
#include <bsl_ios.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_assert.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_memory.h>
#include <bsl_vector.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

int s_seed = bsl::numeric_limits<int>::max();

struct Data {
    int               d_qid;
    bmqt::MessageGUID d_guid;
    bdlbb::Blob       d_payload;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

    // CREATORS
    Data(bdlbb::BlobBufferFactory* bufferFactory, bslma::Allocator* allocator);
    Data(const Data& other, bslma::Allocator* allocator);
};

// CREATORS
Data::Data(bdlbb::BlobBufferFactory* bufferFactory,
           bslma::Allocator*         allocator)
: d_qid(0)
, d_guid()
, d_payload(bufferFactory, allocator)
{
    // NOTHING
}

Data::Data(const Data& other, bslma::Allocator* allocator)
: d_qid(other.d_qid)
, d_guid(other.d_guid)
, d_payload(other.d_payload, allocator)
{
    // NOTHING
}

#ifdef BMQ_ENABLE_MSG_GROUPID
void setMsgGroupId(bmqp::PutEventBuilder* peb, const size_t iteration)
{
    bmqu::MemOutStream oss(bmqtst::TestHelperUtil::allocator());
    oss << "gid:" << iteration;
    peb->setMsgGroupId(oss.str());
}

void validateGroupId(const size_t                    iteration,
                     const bmqp::PutMessageIterator& putIter)
{
    BMQTST_ASSERT(putIter.hasMsgGroupId());
    bmqp::Protocol::MsgGroupId msgGroupId;
    BMQTST_ASSERT(putIter.extractMsgGroupId(&msgGroupId));
    bmqu::MemOutStream oss(bmqtst::TestHelperUtil::allocator());
    oss << "gid:" << iteration;
    BMQTST_ASSERT_EQ(oss.str(), msgGroupId);
}
#endif

bmqt::EventBuilderResult::Enum
appendMessage(size_t                    iteration,
              bmqp::PutEventBuilder*    peb,
              bsl::vector<Data>*        vec,
              bdlbb::BlobBufferFactory* bufferFactory,
              bool                      zeroLenMsg,
              const bmqt::MessageGUID&  guid,
              bslma::Allocator*         allocator)
{
    Data data(bufferFactory, bmqtst::TestHelperUtil::allocator());
    data.d_guid = guid;
    data.d_qid  = iteration;

    int         blobSize = zeroLenMsg ? 0 : bdlb::Random::generate15(&s_seed);
    bsl::string str(blobSize, 'x', allocator);
    bdlbb::BlobUtil::append(&data.d_payload, str.c_str(), blobSize);

    vec->push_back(data);

    if (0 == iteration || 0 == blobSize % 2) {
        peb->startMessage();
    }

#ifdef BMQ_ENABLE_MSG_GROUPID
    setMsgGroupId(peb, iteration);
#endif

    peb->setMessagePayload(&data.d_payload);
    peb->setMessageGUID(data.d_guid);
    return peb->packMessage(data.d_qid);
}

unsigned int findExpectedCrc32(
    const char*                          messagePayload,
    const int                            messagePayloadLen,
    bmqp::MessageProperties*             msgProps,
    bool                                 hasProperties,
    bdlbb::BlobBufferFactory*            bufferFactory,
    bslma::Allocator*                    allocator,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType)
{
    bdlbb::Blob applicationData(bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
    if (hasProperties) {
        bdlbb::BlobUtil::append(
            &applicationData,
            msgProps->streamOut(
                bufferFactory,
                bmqp::MessagePropertiesInfo::makeInvalidSchema()));
        // New format.
    }

    bmqu::MemOutStream error(allocator);
    int                rc = bmqp::Compression::compress(&applicationData,
                                         bufferFactory,
                                         compressionAlgorithmType,
                                         messagePayload,
                                         messagePayloadLen,
                                         &error,
                                         allocator);
    BSLS_ASSERT_OPT(rc == 0);
    unsigned int expectedCrc32 = bmqp::Crc32c::calculate(applicationData);
    return expectedCrc32;
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
//   - Create a simple PUT event which does not use compression for any of
//     the messages.
//   - Create a PUT event which uses ZLIB algorithm for compression. Note
//     that we compress the message properties as well as message payload.
//   - Create a PUT event with mix of messages using either ZLIB algorithm
//     for compression or contains messages which are not compressed.
//   - Create a PUT event with mix of messages of very small size expecting
//     no compression to be used.
//   - Create a PUT event with messages specified to compress using unknown
//     compression algorithm. Expect it to default to none compression.
//   - Create a PUT event with compressed message as payload. Use it for
//     calling packMessageRaw using compressionAlgorithmType ZLib. Expect
//     it to not compress the payload.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
#ifdef BMQ_ENABLE_MSG_GROUPID
    const bmqp::Protocol::MsgGroupId k_MSG_GROUP_ID(
        "gid:0",
        bmqtst::TestHelperUtil::allocator());
#endif
    const int                k_PROPERTY_VAL_ENCODING = 3;
    const bsl::string        k_PROPERTY_VAL_ID       = "myCoolId";
    const unsigned int       k_CRC32                 = 123;
    const bsls::Types::Int64 k_TIME_STAMP            = 1234567890LL;
    const int                k_NUM_PROPERTIES        = 3;
    const char*              k_PAYLOAD = "abcdefghijklmnopqrstuvwxyz";
    const int                k_PAYLOAD_BIGGER_LEN =
        bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE + 400;

    char        k_PAYLOAD_BIGGER[k_PAYLOAD_BIGGER_LEN];
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);
    const char* k_HEX_GUIDS[] = {"40000000000000000000000000000001",
                                 "40000000000000000000000000000002",
                                 "40000000000000000000000000000003",
                                 "40000000000000000000000000000004"};

    for (int i = 0; i < k_PAYLOAD_BIGGER_LEN; i++) {
        k_PAYLOAD_BIGGER[i] = k_PAYLOAD[i % 26];
    }

    {
        PVV("DO NOT USE COMPRESSION FOR MESSAGE PROPERTIES AND PAYLOAD");
        bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(
            0 ==
            msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING));
        BSLS_ASSERT_OPT(0 ==
                        msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
        BSLS_ASSERT_OPT(
            0 == msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

        BSLS_ASSERT_OPT(k_NUM_PROPERTIES == msgProps.numProperties());

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

        obj.startMessage();
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasProperties;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 1234, k_HEX_GUIDS[0], k_TIME_STAMP, true, false},
                      {L_, 5678, k_HEX_GUIDS[1], 9876543210LL, true, true},
                      {L_, 9876, k_HEX_GUIDS[2], 0LL, false, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
        unsigned int expectedCrc32[k_NUM_DATA];

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setMessageGUID(guid);
            obj.setCrc32c(k_CRC32);
            obj.setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_NONE);

            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

            if (test.d_hasNewTimeStamp) {
                BSLS_ASSERT_OPT(0 ==
                                msgProps.setPropertyAsInt64("timestamp",
                                                            test.d_timeStamp));
            }

            if (!test.d_hasProperties) {
                obj.clearMessageProperties();
            }

            expectedCrc32[idx] = findExpectedCrc32(
                k_PAYLOAD_BIGGER,
                k_PAYLOAD_BIGGER_LEN,
                &msgProps,
                test.d_hasProperties,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator(),
                obj.compressionAlgorithmType());

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);

            bmqt::EventBuilderResult::Enum rc = obj.packMessage(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            BMQTST_ASSERT_GT(obj.eventSize(),
                             k_PAYLOAD_BIGGER_LEN * msgNum +
                                 msgProps.totalSize());
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(k_HEX_GUIDS[idx]);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());
            BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                             putIter.header().compressionAlgorithmType());

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() ==
                            k_PAYLOAD_BIGGER_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD_BIGGER,
                                                 k_PAYLOAD_BIGGER_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqt::PropertyType::Enum ptype;
            bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

            if (!test.d_hasProperties) {
                BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
                BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
                BMQTST_ASSERT_EQ(0, prop.numProperties());
            }
            else {
                BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
                BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
                BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
                BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

                BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                                 k_PROPERTY_VAL_ENCODING);

                BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"),
                                 k_PROPERTY_VAL_ID);
                BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"),
                                 test.d_timeStamp);
            }

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
            BMQTST_ASSERT_EQ(putIter.isValid(), true);
#endif
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageGUID(guid);
        bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        BMQTST_ASSERT_GT(obj.eventSize(), k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }
    {
        PVV("USE ZLIB COMPRESSION FOR MESSAGE PROPERTIES AND PAYLOAD");
        bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(
            0 ==
            msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING));
        BSLS_ASSERT_OPT(0 ==
                        msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
        BSLS_ASSERT_OPT(
            0 == msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

        BSLS_ASSERT_OPT(k_NUM_PROPERTIES == msgProps.numProperties());

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

        obj.startMessage();
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasProperties;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 9876, k_HEX_GUIDS[0], k_TIME_STAMP, true, false},
                      {L_, 5432, k_HEX_GUIDS[1], 9876543210LL, true, true},
                      {L_, 3333, k_HEX_GUIDS[2], 0LL, false, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
        unsigned int expectedCrc32[k_NUM_DATA];

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setMessageGUID(guid);
            obj.setCrc32c(k_CRC32);
            obj.setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_ZLIB);
            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

            if (test.d_hasNewTimeStamp) {
                BSLS_ASSERT_OPT(0 ==
                                msgProps.setPropertyAsInt64("timestamp",
                                                            test.d_timeStamp));
            }

            if (!test.d_hasProperties) {
                obj.clearMessageProperties();
            }

            expectedCrc32[idx] = findExpectedCrc32(
                k_PAYLOAD_BIGGER,
                k_PAYLOAD_BIGGER_LEN,
                &msgProps,
                test.d_hasProperties,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator(),
                obj.compressionAlgorithmType());

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);

            bmqt::EventBuilderResult::Enum rc = obj.packMessage(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            // since we compress a large size message; expect event to be small
            BMQTST_ASSERT_LT(obj.eventSize(),
                             k_PAYLOAD_BIGGER_LEN * msgNum +
                                 msgProps.totalSize());
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());
            BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_ZLIB,
                             putIter.header().compressionAlgorithmType());

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() ==
                            k_PAYLOAD_BIGGER_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD_BIGGER,
                                                 k_PAYLOAD_BIGGER_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqt::PropertyType::Enum ptype;
            bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

            if (!test.d_hasProperties) {
                BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
                BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
                BMQTST_ASSERT_EQ(0, prop.numProperties());
            }
            else {
                BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
                BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
                BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
                BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

                BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                                 k_PROPERTY_VAL_ENCODING);

                BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"),
                                 k_PROPERTY_VAL_ID);
                BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"),
                                 test.d_timeStamp);
            }

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(putIter.isValid(), true);
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessageGUID(guid);
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setCompressionAlgorithmType(
            bmqt::CompressionAlgorithmType::e_ZLIB);
        bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);

        // Compression using ZLIB should reduce a large message size
        // significantly
        BMQTST_ASSERT_LT(obj.eventSize(), k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }
    {
        PVV("USE MIX OF ZLIB COMPRESSION AND NO COMPRESSION FOR MESSAGES");
        bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(
            0 ==
            msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING));
        BSLS_ASSERT_OPT(0 ==
                        msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
        BSLS_ASSERT_OPT(
            0 == msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

        BSLS_ASSERT_OPT(k_NUM_PROPERTIES == msgProps.numProperties());

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

        obj.startMessage();
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasProperties;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 9876, k_HEX_GUIDS[0], k_TIME_STAMP, true, false},
                      {L_, 5432, k_HEX_GUIDS[1], 9876543210LL, true, true},
                      {L_, 3333, k_HEX_GUIDS[2], 0LL, false, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
        unsigned int expectedCrc32[k_NUM_DATA];

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            if (idx % 2 == 0) {
                obj.setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_ZLIB);
            }
            else {
                obj.setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE);
            }
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setMessageGUID(guid);
            obj.setCrc32c(k_CRC32);

            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

            if (test.d_hasNewTimeStamp) {
                BSLS_ASSERT_OPT(0 ==
                                msgProps.setPropertyAsInt64("timestamp",
                                                            test.d_timeStamp));
            }

            if (!test.d_hasProperties) {
                obj.clearMessageProperties();
            }

            expectedCrc32[idx] = findExpectedCrc32(
                k_PAYLOAD_BIGGER,
                k_PAYLOAD_BIGGER_LEN,
                &msgProps,
                test.d_hasProperties,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator(),
                obj.compressionAlgorithmType());

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);

            bmqt::EventBuilderResult::Enum rc = obj.packMessage(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            // mix of zlib and no compression should have event much smaller
            // than message
            BMQTST_ASSERT_LT(obj.eventSize(),
                             k_PAYLOAD_BIGGER_LEN * msgNum +
                                 msgProps.totalSize());
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());
            if (idx % 2 == 0) {
                BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_ZLIB,
                                 putIter.header().compressionAlgorithmType());
            }
            else {
                BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                                 putIter.header().compressionAlgorithmType());
            }

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() ==
                            k_PAYLOAD_BIGGER_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD_BIGGER,
                                                 k_PAYLOAD_BIGGER_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqt::PropertyType::Enum ptype;
            bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

            if (!test.d_hasProperties) {
                BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
                BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
                BMQTST_ASSERT_EQ(0, prop.numProperties());
            }
            else {
                BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
                BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
                BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
                BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

                BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                                 k_PROPERTY_VAL_ENCODING);

                BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"),
                                 k_PROPERTY_VAL_ID);
                BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"),
                                 test.d_timeStamp);
            }

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(putIter.isValid(), true);
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageGUID(guid);
        obj.setCompressionAlgorithmType(
            bmqt::CompressionAlgorithmType::e_ZLIB);
        bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);

        // since mix of ZLIB compression used so expect event to be much
        // smaller than message size.
        BMQTST_ASSERT_LT(obj.eventSize(), k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }

    {
        PVV("USE COMPRESSION MIX BUT VERY SMALL MESSAGE: EXPECT NO COMPRESS");
        bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(
            0 ==
            msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING));
        BSLS_ASSERT_OPT(0 ==
                        msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
        BSLS_ASSERT_OPT(
            0 == msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

        BSLS_ASSERT_OPT(k_NUM_PROPERTIES == msgProps.numProperties());

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

        obj.startMessage();
        obj.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN);
        obj.setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasProperties;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 9876, k_HEX_GUIDS[0], k_TIME_STAMP, true, false},
                      {L_, 5432, k_HEX_GUIDS[1], 9876543210LL, true, true},
                      {L_, 3333, k_HEX_GUIDS[2], 0LL, false, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
        unsigned int expectedCrc32[k_NUM_DATA];

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            if (idx % 2 == 0) {
                obj.setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_ZLIB);
            }
            else {
                obj.setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_NONE);
            }
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setCrc32c(k_CRC32);
            obj.setMessageGUID(guid);

            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

            if (test.d_hasNewTimeStamp) {
                BSLS_ASSERT_OPT(0 ==
                                msgProps.setPropertyAsInt64("timestamp",
                                                            test.d_timeStamp));
            }

            if (!test.d_hasProperties) {
                obj.clearMessageProperties();
            }

            expectedCrc32[idx] = findExpectedCrc32(
                k_PAYLOAD,
                k_PAYLOAD_LEN,
                &msgProps,
                test.d_hasProperties,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator(),
                bmqt::CompressionAlgorithmType::e_NONE);

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_LEN);

            bmqt::EventBuilderResult::Enum rc = obj.packMessage(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_LEN);
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            BMQTST_ASSERT_GT(obj.eventSize(),
                             k_PAYLOAD_LEN * msgNum + msgProps.totalSize());
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());

            // since test message size is small we dont expect compression
            BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                             putIter.header().compressionAlgorithmType());

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() == k_PAYLOAD_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD,
                                                 k_PAYLOAD_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqt::PropertyType::Enum ptype;
            bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

            if (!test.d_hasProperties) {
                BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
                BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
                BMQTST_ASSERT_EQ(0, prop.numProperties());
            }
            else {
                BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
                BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
                BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
                BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

                BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                                 k_PROPERTY_VAL_ENCODING);

                BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"),
                                 k_PROPERTY_VAL_ID);
                BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"),
                                 test.d_timeStamp);
            }

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(putIter.isValid(), true);
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessageGUID(guid);
        obj.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN);
        bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        BMQTST_ASSERT_GT(obj.eventSize(), k_PAYLOAD_LEN);
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD,
                                             k_PAYLOAD_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }

    {
        PVV("COMPRESSION USING UNKNOWN ALGORITHM TYPE");
        bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(
            0 ==
            msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING));
        BSLS_ASSERT_OPT(0 ==
                        msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
        BSLS_ASSERT_OPT(
            0 == msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

        BSLS_ASSERT_OPT(k_NUM_PROPERTIES == msgProps.numProperties());

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

        obj.startMessage();
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        obj.setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasProperties;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 9876, k_HEX_GUIDS[0], k_TIME_STAMP, true, false},
                      {L_, 5432, k_HEX_GUIDS[1], 9876543210LL, true, true},
                      {L_, 3333, k_HEX_GUIDS[2], 0LL, false, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
        unsigned int expectedCrc32[k_NUM_DATA];

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setMessageGUID(guid);
            obj.setCrc32c(k_CRC32);
            obj.setCompressionAlgorithmType(
                bmqt::CompressionAlgorithmType::e_UNKNOWN);

            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

            if (test.d_hasNewTimeStamp) {
                BSLS_ASSERT_OPT(0 ==
                                msgProps.setPropertyAsInt64("timestamp",
                                                            test.d_timeStamp));
            }

            if (!test.d_hasProperties) {
                obj.clearMessageProperties();
            }

            expectedCrc32[idx] = findExpectedCrc32(
                k_PAYLOAD_BIGGER,
                k_PAYLOAD_BIGGER_LEN,
                &msgProps,
                test.d_hasProperties,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator(),
                bmqt::CompressionAlgorithmType::e_NONE);

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);

            bmqt::EventBuilderResult::Enum rc = obj.packMessage(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            BMQTST_ASSERT_GT(obj.eventSize(),
                             k_PAYLOAD_BIGGER_LEN * msgNum +
                                 msgProps.totalSize());
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());
            BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
                             putIter.header().compressionAlgorithmType());

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() ==
                            k_PAYLOAD_BIGGER_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD_BIGGER,
                                                 k_PAYLOAD_BIGGER_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqt::PropertyType::Enum ptype;
            bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

            if (!test.d_hasProperties) {
                BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
                BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
                BMQTST_ASSERT_EQ(0, prop.numProperties());
            }
            else {
                BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
                BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
                BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
                BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

                BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                                 k_PROPERTY_VAL_ENCODING);

                BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"),
                                 k_PROPERTY_VAL_ID);
                BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
                BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
                BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"),
                                 test.d_timeStamp);
            }

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(putIter.isValid(), true);
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessageGUID(guid);
        obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN);
        bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        BMQTST_ASSERT_GT(obj.eventSize(), k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }

    {
        PVV("DO NOT USE COMPRESSION FOR RELAYED PUT MESSAGES");

        // Create PutEventBuilder
        bmqp::PutEventBuilder obj(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(obj.crc32c(), 0U);
        bmqu::MemOutStream error(bmqtst::TestHelperUtil::allocator());

        obj.startMessage();

        // create payload which has compressed data.

        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        bmqp::Compression::compress(&payload,
                                    &bufferFactory,
                                    bmqt::CompressionAlgorithmType::e_ZLIB,
                                    k_PAYLOAD_BIGGER,
                                    k_PAYLOAD_BIGGER_LEN,
                                    &error,
                                    bmqtst::TestHelperUtil::allocator());
        obj.setMessagePayload(&payload);

#ifdef BMQ_ENABLE_MSG_GROUPID
        obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif

        struct Test {
            int                d_line;
            int                d_queueId;
            const char*        d_guidHex;
            bsls::Types::Int64 d_timeStamp;
            bool               d_hasNewTimeStamp;
        } k_DATA[] = {{L_, 9876, k_HEX_GUIDS[0], k_TIME_STAMP, false},
                      {L_, 5432, k_HEX_GUIDS[1], 9876543210LL, true},
                      {L_, 3333, k_HEX_GUIDS[2], 0LL, false}};

        // Pack messages
        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test   = k_DATA[idx];
            const int         msgNum = idx + 1;
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            obj.setCrc32c(k_CRC32)
                .setCompressionAlgorithmType(
                    bmqt::CompressionAlgorithmType::e_ZLIB)
                .setMessageGUID(guid);

            BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);

#ifdef BMQ_ENABLE_MSG_GROUPID
            BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
            BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), payload.length());

            bmqt::EventBuilderResult::Enum rc = obj.packMessageRaw(
                test.d_queueId);

            BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
            BMQTST_ASSERT_EQ(obj.messageCount(), msgNum);
            BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), payload.length());
            BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
            BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

            BMQTST_ASSERT_GT(obj.eventSize(), payload.length() * msgNum);
        }

        // Get blob and use bmqp iterator to test.  Note that bmqp event and
        // bmqp iterators are lower than bmqp builders, and thus, can be used
        // to test them.
        bmqp::Event rawEvent(obj.blob().get(),
                             bmqtst::TestHelperUtil::allocator());

        BSLS_ASSERT_OPT(rawEvent.isValid());
        BSLS_ASSERT_OPT(rawEvent.isPutEvent());

        bmqp::PutMessageIterator putIter(&bufferFactory,
                                         bmqtst::TestHelperUtil::allocator());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BSLS_ASSERT_OPT(putIter.isValid());
        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&       test = k_DATA[idx];
            bmqt::MessageGUID guid;
            guid.fromHex(test.d_guidHex);

            BMQTST_ASSERT_EQ(1, putIter.next());
            BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
            BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
            BMQTST_ASSERT_EQ(k_CRC32, putIter.header().crc32c());
            BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_ZLIB,
                             putIter.header().compressionAlgorithmType());

            payloadBlob.removeAll();

            BSLS_ASSERT_OPT(putIter.loadMessagePayload(&payloadBlob) == 0);
            BSLS_ASSERT_OPT(putIter.messagePayloadSize() ==
                            k_PAYLOAD_BIGGER_LEN);

            int res, compareResult;
            res = bmqu::BlobUtil::compareSection(&compareResult,
                                                 payloadBlob,
                                                 bmqu::BlobPosition(),
                                                 k_PAYLOAD_BIGGER,
                                                 k_PAYLOAD_BIGGER_LEN);

            BSLS_ASSERT_OPT(res == 0);
            BSLS_ASSERT_OPT(compareResult == 0);

            bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
            BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
            BMQTST_ASSERT_EQ(0, prop.numProperties());

#ifdef BMQ_ENABLE_MSG_GROUPID
            bmqp::Protocol::MsgGroupId msgGroupId(
                bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
            BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
            BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

            BMQTST_ASSERT_EQ(putIter.isValid(), true);
        }

        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 3 msgs
        BMQTST_ASSERT_EQ(false, putIter.isValid());

        // Reset the builder, pack 1 msg. Test.
        obj.reset();
        BMQTST_ASSERT_EQ(0, obj.messageCount());
        BMQTST_ASSERT_EQ(0U, obj.crc32c());

        obj.startMessage();

        // Pack one msg
        const int         k_QID = 9876;
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[3]);

        obj.setMessagePayload(&payload);
        obj.setMessageGUID(guid);
        obj.setCompressionAlgorithmType(
            bmqt::CompressionAlgorithmType::e_ZLIB);
        bmqt::EventBuilderResult::Enum rc = obj.packMessageRaw(k_QID);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        BMQTST_ASSERT_GT(obj.eventSize(), payload.length());
        BMQTST_ASSERT_EQ(obj.messageCount(), 1);

        rawEvent.reset(obj.blob().get());
        rawEvent.loadPutMessageIterator(&putIter, true);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
        int                     res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);
        BMQTST_ASSERT_EQ(0, res);
        BMQTST_ASSERT_EQ(0, compareResult);
        BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
        BMQTST_ASSERT_EQ(0, prop.numProperties());
        BMQTST_ASSERT_EQ(true, putIter.isValid());
        BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
        BMQTST_ASSERT_EQ(false, putIter.isValid());
    }
}

static void test2_manipulators_one()
// ------------------------------------------------------------------------
// MANIPULATORS TEST
//
// Concerns:
//   Test manipulators.
//
// Plan:
//   Build an event with multiple messages. Iterate and test.
//   Note that we use only setMessagePayload(const char* data, int size)
//   flavor in this test.
//   See case 4 for testing the
//      setMessagePayload(const bdlbb::Blob& data)      variant.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MANIPULATORS - ONE");

    struct TestData {
        int          d_line;
        int          d_qid;
        const char*  d_hexGuid;
        const char   d_payload[200];
        int          d_payloadLen;  // real length
        unsigned int d_crc32c;
    } k_DATA[] = {
        {L_, 0, "40000000000000000000000000000001", {'a'}, 1, 0},
        {L_, 1, "40000000000000000000000000000002", {"abcdefghijkl"}, 12, 0},
        {L_,
         -1,
         "40000000000000000000000000000003",
         {"abcedefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234"},
         57,  // 26 + 26 + 5
         0},
        {L_,
         bsl::numeric_limits<int>::max(),
         "40000000000000000000000000000004",
         {"abcedefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0"},
         53,  // 26 + 26 + 1
         0},
        {L_,
         bsl::numeric_limits<int>::min(),
         "40000000000000000000000000000005",
         {"abcedefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"},
         52,  // 26 + 26
         0},
        {L_,
         bsl::numeric_limits<int>::min(),
         "40000000000000000000000000000006",
         {"abcedefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY"},
         55,  // 26 + 25
         0},
        {L_,
         bsl::numeric_limits<int>::max(),
         "40000000000000000000000000000007",
         {"abcedefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234"},
         57,  // 26 + 26 + 5
         0},
    };

    const int k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Create PutEventBuilder
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());

    // Properties.
    bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, msgProps.setPropertyAsInt32("encoding", 3));
    BMQTST_ASSERT_EQ(0, msgProps.setPropertyAsString("id", "myCoolId"));
    BMQTST_ASSERT_EQ(0, msgProps.setPropertyAsInt64("timestamp", 0LL));

    const int numProps = 3;
    BMQTST_ASSERT_EQ(numProps, msgProps.numProperties());

    // Set flags
    int phFlags = 0;
    bmqp::PutHeaderFlagUtil::setFlag(&phFlags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    bmqp::PutHeaderFlagUtil::setFlag(
        &phFlags,
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);

    for (int dataIdx = 0; dataIdx < k_NUM_DATA; ++dataIdx) {
        TestData& data   = k_DATA[dataIdx];
        const int k_LINE = data.d_line;

        P(k_LINE);

        obj.startMessage();

        BMQTST_ASSERT_EQ_D(dataIdx, obj.unpackedMessageSize(), 0);

#ifdef BMQ_ENABLE_MSG_GROUPID
        setMsgGroupId(&obj, dataIdx);
#endif
        obj.setMessagePayload(data.d_payload, data.d_payloadLen);

        BMQTST_ASSERT_EQ(0,
                         msgProps.setPropertyAsInt64("timestamp",
                                                     dataIdx * 10LL));

        obj.setMessageProperties(&msgProps)
            .setMessageGUID(bmqp::MessageGUIDGenerator::testGUID())
            .setFlags(phFlags);

        data.d_crc32c = findExpectedCrc32(
            data.d_payload,
            data.d_payloadLen,
            &msgProps,
            true,  // hasProperties
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator(),
            bmqt::CompressionAlgorithmType::e_NONE);

        bmqt::EventBuilderResult::Enum rc = obj.packMessage(data.d_qid);

        BMQTST_ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);

        BMQTST_ASSERT_LT_D(dataIdx, data.d_payloadLen, obj.eventSize());
        BMQTST_ASSERT_EQ_D(dataIdx, dataIdx + 1, obj.messageCount());
        BMQTST_ASSERT_EQ_D(dataIdx,
                           data.d_payloadLen,
                           obj.unpackedMessageSize());
    }

    // Iterate and check
    bmqp::Event rawEvent(obj.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_OPT(true == rawEvent.isValid());
    BSLS_ASSERT_OPT(true == rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);
    BMQTST_ASSERT_EQ(true, putIter.isValid());

    int dataIndex = 0;

    while ((putIter.next() == 1) && dataIndex < k_NUM_DATA) {
        const TestData&   data = k_DATA[dataIndex];
        bmqt::MessageGUID guid;
        guid.fromHex(data.d_hexGuid);

        BMQTST_ASSERT_EQ_D(dataIndex, true, putIter.isValid());

        BMQTST_ASSERT_EQ_D(dataIndex, guid, putIter.header().messageGUID());
        BMQTST_ASSERT_EQ_D(dataIndex, data.d_qid, putIter.header().queueId());
        BMQTST_ASSERT_EQ_D(dataIndex,
                           data.d_crc32c,
                           putIter.header().crc32c());
        BMQTST_ASSERT_EQ_D(dataIndex, phFlags, putIter.header().flags());

        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ_D(dataIndex,
                           0,
                           putIter.loadMessagePayload(&payloadBlob));

        BMQTST_ASSERT_EQ_D(dataIndex,
                           data.d_payloadLen,
                           putIter.messagePayloadSize());

        int res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             data.d_payload,
                                             data.d_payloadLen);

        BMQTST_ASSERT_EQ_D(dataIndex, 0, res);
        BMQTST_ASSERT_EQ_D(dataIndex, 0, compareResult);

        bmqp::MessageProperties  p(bmqtst::TestHelperUtil::allocator());
        bmqt::PropertyType::Enum ptype;
        BMQTST_ASSERT_EQ(true, putIter.hasMessageProperties());
        BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&p));
        BMQTST_ASSERT_EQ(numProps, p.numProperties());
        BMQTST_ASSERT_EQ(true, p.hasProperty("encoding", &ptype));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);
        BMQTST_ASSERT_EQ(3, p.getPropertyAsInt32("encoding"));
        BMQTST_ASSERT_EQ(true, p.hasProperty("id", &ptype));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
        BMQTST_ASSERT_EQ("myCoolId", p.getPropertyAsString("id"));
        BMQTST_ASSERT_EQ(true, p.hasProperty("timestamp", &ptype));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
        BMQTST_ASSERT_EQ(dataIndex * 10LL, p.getPropertyAsInt64("timestamp"));

#ifdef BMQ_ENABLE_MSG_GROUPID
        validateGroupId(dataIndex, putIter);
#endif

        ++dataIndex;
    }

    BMQTST_ASSERT_EQ(dataIndex, k_NUM_DATA);
    BMQTST_ASSERT_EQ(false, putIter.isValid());
}

static void test3_eventTooBig()
// ------------------------------------------------------------------------
// EVENT TOO BIG TEST
//
// Concerns:
//   Test behavior when trying to build *one* big message.
//
// Plan:
//   TODO
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EVENT TOO BIG");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bdlbb::Blob bigMsgPayload(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
#ifdef BMQ_ENABLE_MSG_GROUPID
    bmqp::Protocol::MsgGroupId k_MSG_GROUP_ID(
        "gid:0",
        bmqtst::TestHelperUtil::allocator());
#endif
    const int         k_QID = 4321;
    bmqt::MessageGUID guid  = bmqp::MessageGUIDGenerator::testGUID();

    bmqp::PutTester::populateBlob(&bigMsgPayload,
                                  bmqp::PutHeader::k_MAX_PAYLOAD_SIZE_SOFT +
                                      1);

    BSLS_ASSERT(bmqp::PutHeader::k_MAX_PAYLOAD_SIZE_SOFT <
                bigMsgPayload.length());

    // Create PutEventBuilder
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());

    obj.startMessage();
#ifdef BMQ_ENABLE_MSG_GROUPID
    obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif
    obj.setMessageGUID(guid);
    obj.setMessagePayload(&bigMsgPayload);

    bmqt::EventBuilderResult::Enum rc = obj.packMessage(k_QID);
    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG);

    // Now append a "regular"-sized message
    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    // Now append a "regular"-sized message
#ifdef BMQ_ENABLE_MSG_GROUPID
    obj.setMsgGroupId(k_MSG_GROUP_ID);
#endif
    obj.setMessageGUID(guid);
    obj.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN);
    rc = obj.packMessage(k_QID);

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    BMQTST_ASSERT_LT(k_PAYLOAD_LEN, obj.eventSize());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and bmqp
    // iterators are lower than bmqp builders, and thus, can be used to test
    // them.
    bmqp::Event rawEvent(obj.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT_EQ(true, putIter.isValid());
    BMQTST_ASSERT_EQ(1, putIter.next());

    BMQTST_ASSERT_EQ(putIter.header().queueId(), k_QID);
    BMQTST_ASSERT_EQ(putIter.header().messageGUID(), guid);

    bdlbb::Blob payloadBlob(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
    BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_LEN);

    int res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         payloadBlob,
                                         bmqu::BlobPosition(),
                                         k_PAYLOAD,
                                         k_PAYLOAD_LEN);

    BMQTST_ASSERT_EQ(res, 0);
    BMQTST_ASSERT_EQ(compareResult, 0);

#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT(putIter.hasMsgGroupId());

    bmqp::Protocol::MsgGroupId msgGroupId(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
    BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif

    BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
}

static void test4_manipulators_two()
// ------------------------------------------------------------------------
// MANIPULATORS - TWO
//
// Concerns:
//   Test manipulators.
//
// Plan:
//   Build an event with multiple messages. Iterate and test.
//   Note that we use only setMessagePayload(const Blob& data)
//   flavor in this test.
//   See case 2 for testing the
//      setMessagePayload(const char* data, int size)    variant.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MANIPULATORS - TWO");

    // Create PutEventBuilder
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     data(bmqtst::TestHelperUtil::allocator());
    const size_t          k_NUM_MSGS = 1000;

    for (size_t dataIdx = 0; dataIdx < k_NUM_MSGS; ++dataIdx) {
        bmqt::EventBuilderResult::Enum rc = appendMessage(
            dataIdx,
            &obj,
            &data,
            &bufferFactory,
            false,
            bmqp::MessageGUIDGenerator::testGUID(),
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);
    }

    // Iterate and check
    bmqp::Event rawEvent(obj.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);
    BMQTST_ASSERT_EQ(true, putIter.isValid());

    size_t dataIndex = 0;

    while ((putIter.next() == 1) && dataIndex < k_NUM_MSGS) {
        const Data& D = data[dataIndex];

        BMQTST_ASSERT_EQ_D(dataIndex, true, putIter.isValid());
        BMQTST_ASSERT_EQ_D(dataIndex, D.d_qid, putIter.header().queueId());
        BMQTST_ASSERT_EQ_D(dataIndex,
                           D.d_guid,
                           putIter.header().messageGUID());

        bdlbb::Blob payloadBlob(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ_D(dataIndex,
                           0,
                           putIter.loadMessagePayload(&payloadBlob));

        BMQTST_ASSERT_EQ_D(dataIndex,
                           payloadBlob.length(),
                           putIter.messagePayloadSize());

        BMQTST_ASSERT_EQ_D(dataIndex,
                           0,
                           bdlbb::BlobUtil::compare(payloadBlob, D.d_payload));

#ifdef BMQ_ENABLE_MSG_GROUPID
        validateGroupId(dataIndex, putIter);
#endif

        ++dataIndex;
    }

    BMQTST_ASSERT_EQ(dataIndex, data.size());
    BMQTST_ASSERT_EQ(false, putIter.isValid());
}

static void test5_putEventWithZeroLengthMessage()
// ------------------------------------------------------------------------
// PUT EVENT WITH ZERO LENGTH MESSAGE
//
// Concerns:
//   Zero-length PUT messages are not supported.
//
// Plan:
//   Build a PUT event containing zero-length PUT message and verify that
//   'bmqp::PutMessageIterator' will skip it gracefully.
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUT EVENT WITH ZERO LEGNTH MESSAGE");

    // Create PutEventBuilder
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>     data(bmqtst::TestHelperUtil::allocator());

    bmqt::EventBuilderResult::Enum rc = appendMessage(
        0,
        &obj,
        &data,
        &bufferFactory,
        true,
        bmqp::MessageGUIDGenerator::testGUID(),
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ_D(0, rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Iterate and check
    bmqp::Event rawEvent(obj.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT(true == rawEvent.isValid());
    BSLS_ASSERT(true == rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);
    BMQTST_ASSERT_EQ(true, putIter.isValid());

    BMQTST_ASSERT_NE(1, putIter.next());
}

static void test6_emptyBuilder()
// ------------------------------------------------------------------------
// EMPTY BUILDER
//
// Concerns:
//   Behaviour of setters and getters when 'bmqp::PutEventBuilder'
//   object is not initialized and when it has no packed messages.
// Plan:
//   1. Create 'bmqp::PutEventBuilder' and verify that its setters fire
//      assert.
//   2. Call 'bmqp::PutEventBuilder::startMessage()' and verify that its
//      setters and getters work consistently.
//
// Testing:
//   bmqp::PutEventBuilder setters and getters
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EMPTY BUILDER");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
#ifdef BMQ_ENABLE_MSG_GROUPID
    bmqp::Protocol::MsgGroupId k_MSG_GROUP_ID(
        "gid:0",
        bmqtst::TestHelperUtil::allocator());
#endif

    unsigned char zeroGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
    bsl::memset(zeroGuidBuf, 0, bmqt::MessageGUID::e_SIZE_BINARY);
    bmqt::MessageGUID zeroGuid;
    zeroGuid.fromBinary(zeroGuidBuf);

    unsigned char onesGuidBuf[bmqt::MessageGUID::e_SIZE_BINARY];
    bsl::memset(onesGuidBuf, 1, bmqt::MessageGUID::e_SIZE_BINARY);
    bmqt::MessageGUID onesGuid;
    onesGuid.fromBinary(onesGuidBuf);

    const char* k_PAYLOAD = "abcdefghijklmnopqrstuvwxyz";

    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), 0);
    BMQTST_ASSERT_SAFE_FAIL(obj.setFlags(0));
#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_SAFE_FAIL(obj.setMsgGroupId(k_MSG_GROUP_ID));
#endif
    BMQTST_ASSERT_SAFE_FAIL(obj.setMessageGUID(zeroGuid));
    BMQTST_ASSERT_SAFE_FAIL(obj.setCrc32c(0));
    BMQTST_ASSERT_SAFE_FAIL(
        obj.setMessagePayload(k_PAYLOAD, bsl::strlen(k_PAYLOAD)));
    BMQTST_ASSERT_SAFE_FAIL(obj.setMessagePayload(NULL));

    obj.startMessage();

    const int evtSize = sizeof(bmqp::EventHeader);

    BMQTST_ASSERT_EQ(obj.messageGUID(), zeroGuid);
#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), true);
#endif
    BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), 0);
    BMQTST_ASSERT_EQ(obj.eventSize(), evtSize);
    BMQTST_ASSERT_EQ(obj.flags(), 0);
    BMQTST_ASSERT_EQ(obj.messageCount(), 0);

    BMQTST_ASSERT_SAFE_FAIL(obj.setMessagePayload(k_PAYLOAD, -1));

    obj.setMessageGUID(onesGuid);

    BMQTST_ASSERT_EQ(obj.messageGUID(), onesGuid);

#ifdef BMQ_ENABLE_MSG_GROUPID
    obj.setMsgGroupId(k_MSG_GROUP_ID);

    BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
    BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);

    obj.clearMsgGroupId();

    BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), true);
#endif

    static_cast<void>(k_PAYLOAD);  // suppress 'unused-variable' warning in
                                   // prod build
}

static void test7_multiplePackMessage()
// ------------------------------------------------------------------------
// MULTIPLE PACK MESSAGE
//
// Concerns:
//   Behaviour of setters and getters when 'bmqp::PutEventBuilder'
//   object is used with multiple calls to packMessage.
// Plan:
//   1. Create 'bmqp::PutEventBuilder' and verify that its setters fire
//      assert.
//   2. Call 'bmqp::PutEventBuilder::startMessage()', call
//      'bmqp::PutEventBuilder::packMessage()' multiple times and verify
//      that both packed messages are identical i.e. exhibit the exact
//      same behavior by asserting on compressionAlgorithmType, payload
//      size.
//
// Testing:
//   multiple calls to bmqp::PutEventBuilder::packMessage()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TEST MULTIPLE CALLS TO PACK MESSAGE");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
#ifdef BMQ_ENABLE_MSG_GROUPID
    const bmqp::Protocol::MsgGroupId k_MSG_GROUP_ID(
        "gid:0",
        bmqtst::TestHelperUtil::allocator());
#endif
    const int                k_PROPERTY_VAL_ENCODING = 3;
    const bsl::string        k_PROPERTY_VAL_ID       = "myCoolId";
    const unsigned int       k_CRC32                 = 123;
    const bsls::Types::Int64 k_TIME_STAMP            = 1234567890LL;
    const int                k_NUM_PROPERTIES        = 3;
    const char*              k_PAYLOAD = "abcdefghijklmnopqrstuvwxyz";
    const int                k_PAYLOAD_BIGGER_LEN =
        bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE + 400;
    char        k_PAYLOAD_BIGGER[k_PAYLOAD_BIGGER_LEN];
    const char* k_HEX_GUIDS[] = {"40000000000000000000000000000001",
                                 "40000000000000000000000000000002",
                                 "40000000000000000000000000000003"};

    BSLMF_ASSERT(k_PAYLOAD_BIGGER_LEN >
                 bmqp::Protocol::k_COMPRESSION_MIN_APPDATA_SIZE);

    for (int i = 0; i < k_PAYLOAD_BIGGER_LEN; i++) {
        k_PAYLOAD_BIGGER[i] = k_PAYLOAD[i % 26];
    }

    bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0,
                     msgProps.setPropertyAsInt32("encoding",
                                                 k_PROPERTY_VAL_ENCODING));
    BMQTST_ASSERT_EQ(0, msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
    BMQTST_ASSERT_EQ(0,
                     msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

    BMQTST_ASSERT_EQ(k_NUM_PROPERTIES, msgProps.numProperties());

    // Create PutEventBuilder
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

    obj.startMessage();
    obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN)
        .setMessageProperties(&msgProps)
        .setMessageGUID(bmqp::MessageGUIDGenerator::testGUID())
#ifdef BMQ_ENABLE_MSG_GROUPID
        .setMsgGroupId(k_MSG_GROUP_ID)
#endif
        .setCompressionAlgorithmType(bmqt::CompressionAlgorithmType::e_ZLIB);

    int d_q1 = 9876;
    int d_q2 = 9877;

    // Pack message

    obj.setCrc32c(k_CRC32);

    BMQTST_ASSERT_EQ(obj.crc32c(), k_CRC32);
    BMQTST_ASSERT_EQ(0,
                     msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

    unsigned int expectedCrc32 = findExpectedCrc32(
        k_PAYLOAD_BIGGER,
        k_PAYLOAD_BIGGER_LEN,
        &msgProps,
        true,  // has properties
        &bufferFactory,
        bmqtst::TestHelperUtil::allocator(),
        obj.compressionAlgorithmType());

#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), false);
    BMQTST_ASSERT_EQ(obj.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

    BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);

    // 1st pack message call
    bmqt::EventBuilderResult::Enum rc = obj.packMessage(d_q1);
    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // 2nd pack message call
    obj.setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());
    rc = obj.packMessage(d_q2);
    BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    BMQTST_ASSERT_EQ(obj.messageCount(), 2);
    BMQTST_ASSERT_EQ(obj.unpackedMessageSize(), k_PAYLOAD_BIGGER_LEN);
    BMQTST_ASSERT_EQ(obj.messageGUID(), bmqt::MessageGUID());
    BMQTST_ASSERT_EQ(obj.crc32c(), 0U);

    // since we compress a large size message; expect event to be small
    BMQTST_ASSERT_LT(obj.eventSize(),
                     2 * (k_PAYLOAD_BIGGER_LEN + msgProps.totalSize()));

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(obj.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(rawEvent.isValid());
    BMQTST_ASSERT(rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT(putIter.isValid());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

    // check for the 2 packed messages
    for (size_t idx = 0; idx < 2; ++idx) {
        bmqt::MessageGUID guid;
        guid.fromHex(k_HEX_GUIDS[idx]);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(d_q1 + static_cast<int>(idx),
                         putIter.header().queueId());
        BMQTST_ASSERT_EQ(expectedCrc32, putIter.header().crc32c());
        BMQTST_ASSERT_EQ(bmqt::CompressionAlgorithmType::e_ZLIB,
                         putIter.header().compressionAlgorithmType());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

        payloadBlob.removeAll();

        BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
        BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

        int res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD_BIGGER,
                                             k_PAYLOAD_BIGGER_LEN);

        BMQTST_ASSERT_EQ(res, 0);
        BMQTST_ASSERT_EQ(compareResult, 0);

        bmqt::PropertyType::Enum ptype;
        bmqp::MessageProperties  prop(bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(putIter.hasMessageProperties(), true);
        BMQTST_ASSERT_EQ(putIter.loadMessageProperties(&prop), 0);
        BMQTST_ASSERT_EQ(prop.numProperties(), k_NUM_PROPERTIES);
        BMQTST_ASSERT_EQ(prop.hasProperty("encoding", &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

        BMQTST_ASSERT_EQ(prop.getPropertyAsInt32("encoding"),
                         k_PROPERTY_VAL_ENCODING);

        BMQTST_ASSERT_EQ(prop.hasProperty("id", &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);
        BMQTST_ASSERT_EQ(prop.getPropertyAsString("id"), k_PROPERTY_VAL_ID);
        BMQTST_ASSERT_EQ(prop.hasProperty("timestamp", &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);
        BMQTST_ASSERT_EQ(prop.getPropertyAsInt64("timestamp"), k_TIME_STAMP);

#ifdef BMQ_ENABLE_MSG_GROUPID
        bmqp::Protocol::MsgGroupId msgGroupId(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT_EQ(putIter.hasMsgGroupId(), true);
        BMQTST_ASSERT_EQ(putIter.extractMsgGroupId(&msgGroupId), true);
        BMQTST_ASSERT_EQ(msgGroupId, k_MSG_GROUP_ID);
#endif
        BMQTST_ASSERT_EQ(putIter.isValid(), true);
    }

    BMQTST_ASSERT_EQ(true, putIter.isValid());
    BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 2 msgs
    BMQTST_ASSERT_EQ(false, putIter.isValid());

    // Start a new message in builder, pack one more message
    obj.startMessage();

    // Pack one msg
    const int         k_QID = 9876;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_GUIDS[2]);

    obj.setMessagePayload(k_PAYLOAD_BIGGER, k_PAYLOAD_BIGGER_LEN)
        .setMessageGUID(guid);
    rc = obj.packMessage(k_QID);

    BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    BMQTST_ASSERT_GT(obj.eventSize(), k_PAYLOAD_BIGGER_LEN);
    BMQTST_ASSERT_EQ(obj.messageCount(), 3);
#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_EQ(obj.msgGroupId().isNull(), true);
#endif
    BMQTST_ASSERT_EQ(obj.compressionAlgorithmType(),
                     bmqt::CompressionAlgorithmType::e_NONE);
    rawEvent.reset(obj.blob().get());
    rawEvent.loadPutMessageIterator(&putIter, true);

    // we want to test the 3rd message so we call next thrice
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
    BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());

    payloadBlob.removeAll();

    BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);
    BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_BIGGER_LEN);

    bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
    int                     res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         payloadBlob,
                                         bmqu::BlobPosition(),
                                         k_PAYLOAD_BIGGER,
                                         k_PAYLOAD_BIGGER_LEN);
    BMQTST_ASSERT_EQ(0, res);
    BMQTST_ASSERT_EQ(0, compareResult);
    BMQTST_ASSERT_EQ(false, putIter.hasMessageProperties());
#ifdef BMQ_ENABLE_MSG_GROUPID
    BMQTST_ASSERT_EQ(false, putIter.hasMsgGroupId());
#endif
    BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&prop));
    BMQTST_ASSERT_EQ(0, prop.numProperties());
    BMQTST_ASSERT_EQ(true, putIter.isValid());
    BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
    BMQTST_ASSERT_EQ(false, putIter.isValid());
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::PutEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::PutEventBuilder encode bmqp::EventType::e_PUT event.
//   2. Store binary representation of this event into a file.
//   3. Read this file, decode event and verify that it contains a message
//      with expected properties and payload.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DECODE FROM FILE");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bdlbb::Blob outBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream       os(bmqtst::TestHelperUtil::allocator());
    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    bdlb::Guid               guid = bdlb::GuidUtil::generate();

    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);
    const int   k_QID         = 9876;
    const int   k_SIZE        = 256;
    char        buf[k_SIZE]   = {0};

    const int                k_PROPERTY_VAL_ENCODING = 3;
    const bsl::string        k_PROPERTY_VAL_ID       = "myCoolId";
    const bsls::Types::Int64 k_TIME_STAMP            = 1234567890LL;
    const int                k_PROPERTY_NUM          = 3;

    bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());
    bmqt::MessageGUID       msgGuid = bmqp::MessageGUIDGenerator::testGUID();

    msgProps.setPropertyAsInt32("encoding", k_PROPERTY_VAL_ENCODING);
    msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID);
    msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP);

    int padding = 0;
    bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding, msgProps.totalSize());

    const int k_PROPERTY_SIZE = msgProps.totalSize() + padding;

    // Set flags
    int phFlags = 0;
    bmqp::PutHeaderFlagUtil::setFlag(&phFlags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    bmqp::PutHeaderFlagUtil::setFlag(
        &phFlags,
        bmqp::PutHeaderFlags::e_MESSAGE_PROPERTIES);

    // Create PutEventBuilder
    bmqp::PutEventBuilder obj(blobSpPool.get(),
                              bmqtst::TestHelperUtil::allocator());

    obj.startMessage();

    // Pack one msg
    obj.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setMessageGUID(msgGuid)
        .setFlags(phFlags)
        .setMessageProperties(&msgProps);

    const unsigned int k_CRC32 = findExpectedCrc32(
        k_PAYLOAD,
        k_PAYLOAD_LEN,
        &msgProps,
        true,
        &bufferFactory,
        bmqtst::TestHelperUtil::allocator(),
        obj.compressionAlgorithmType());

    obj.packMessage(k_QID);

    os << "msg_put_"
       << "_" << guid << ".bin" << bsl::ends;

    /// Functor invoked to delete the file at the specified `filePath`
    struct local {
        static void deleteFile(const char* filePath)
        {
            BSLS_ASSERT_OPT(bsl::remove(filePath) == 0);
        }
    };

    bdlb::ScopeExitAny guard(
        bdlf::BindUtil::bind(local::deleteFile, os.str().data()));

    // Dump blob into file
    bsl::ofstream ofile(os.str().data(), bsl::ios::binary);

    BSLS_ASSERT(ofile.good() == true);

    bdlbb::BlobUtil::copy(buf, *obj.blob(), 0, obj.blob()->length());
    ofile.write(buf, k_SIZE);
    ofile.close();
    bsl::memset(buf, 0, k_SIZE);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios::binary);

    BSLS_ASSERT(ifile.good() == true);

    ifile.read(buf, k_SIZE);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, k_SIZE);

    outBlob.appendDataBuffer(dataBlobBuffer);
    outBlob.setLength(obj.blob()->length());

    BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(*obj.blob(), outBlob), 0);

    // Decode event
    bmqp::Event rawEvent(&outBlob, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(rawEvent.isPutEvent(), true);

    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(k_QID, putIter.header().queueId());
    BMQTST_ASSERT_EQ(msgGuid, putIter.header().messageGUID());
    BMQTST_ASSERT_EQ(k_CRC32, putIter.header().crc32c());
    BMQTST_ASSERT_EQ(phFlags, putIter.header().flags());

    BMQTST_ASSERT_EQ(putIter.loadMessagePayload(&payloadBlob), 0);

    BMQTST_ASSERT_EQ(putIter.messagePayloadSize(), k_PAYLOAD_LEN);

    int res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         payloadBlob,
                                         bmqu::BlobPosition(),
                                         k_PAYLOAD,
                                         k_PAYLOAD_LEN);

    BMQTST_ASSERT_EQ(0, res);
    BMQTST_ASSERT_EQ(0, compareResult);

    msgProps.clear();
    BMQTST_ASSERT_EQ(true, putIter.hasMessageProperties());
    BMQTST_ASSERT_EQ(0, putIter.loadMessageProperties(&msgProps));

    BMQTST_ASSERT_EQ(k_PROPERTY_NUM, msgProps.numProperties());
    BMQTST_ASSERT_EQ(k_PROPERTY_SIZE, putIter.messagePropertiesSize());

    BMQTST_ASSERT_EQ(msgProps.getPropertyAsInt32("encoding"),
                     k_PROPERTY_VAL_ENCODING);
    BMQTST_ASSERT_EQ(msgProps.getPropertyAsString("id"), k_PROPERTY_VAL_ID);
    BMQTST_ASSERT_EQ(msgProps.getPropertyAsInt64("timestamp"), k_TIME_STAMP);

    BMQTST_ASSERT_EQ(true, putIter.isValid());
    BMQTST_ASSERT_EQ(0, putIter.next());  // we added only 1 msg
    BMQTST_ASSERT_EQ(false, putIter.isValid());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // We explicitly initialize before the 'TEST_PROLOG' to circumvent a
    // case where the associated logging infrastructure triggers a default
    // allocation violation for no apparent reason.

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD: figure
    // out the right way to "fix" this.
    Data dummy(static_cast<bdlbb::BlobBufferFactory*>(0),
               bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data, bslma::UsesBslmaAllocator> >(
            dummy));

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    PV("Seed: " << s_seed);

    switch (_testCase) {
    case 0:
    case 7: test7_multiplePackMessage(); break;
    case 6: test6_emptyBuilder(); break;
    case 5: test5_putEventWithZeroLengthMessage(); break;
    case 4: test4_manipulators_two(); break;
    case 3: test3_eventTooBig(); break;
    case 2: test2_manipulators_one(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_decodeFromFile(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
