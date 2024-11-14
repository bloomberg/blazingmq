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

// bmqp_pusheventbuilder.t.cpp                                        -*-C++-*-
#include <bmqp_pusheventbuilder.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_messageguid.h>

#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_guidutil.h>
#include <bdlb_nullablevalue.h>
#include <bdlb_scopeexit.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bsl_cstdlib.h>
#include <bsl_cstring.h>  // for bsl::strlen
#include <bsl_ctime.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

#ifdef BMQ_ENABLE_MSG_GROUPID
typedef bdlb::NullableValue<bmqp::Protocol::MsgGroupId> NullableMsgGroupId;
#endif

struct Data {
    bmqt::MessageGUID                  d_guid;
    int                                d_qid;
    bmqp::Protocol::SubQueueInfosArray d_subQueueInfos;
#ifdef BMQ_ENABLE_MSG_GROUPID
    NullableMsgGroupId d_msgGroupId;
#endif
    bdlbb::Blob                          d_payload;
    int                                  d_flags;
    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

    // CREATORS
    Data(bdlbb::BlobBufferFactory* bufferFactory, bslma::Allocator* allocator);
    Data(const Data& other, bslma::Allocator* allocator);
};

// CREATORS
Data::Data(bdlbb::BlobBufferFactory* bufferFactory,
           bslma::Allocator*         allocator)
: d_qid(-1)
, d_subQueueInfos(allocator)
#ifdef BMQ_ENABLE_MSG_GROUPID
, d_msgGroupId(allocator)
#endif
, d_payload(bufferFactory, allocator)
, d_flags(0)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
{
    // NOTHING
}

Data::Data(const Data& other, bslma::Allocator* allocator)
: d_guid(other.d_guid)
, d_qid(other.d_qid)
, d_subQueueInfos(other.d_subQueueInfos, allocator)
#ifdef BMQ_ENABLE_MSG_GROUPID
, d_msgGroupId(other.d_msgGroupId, allocator)
#endif
, d_payload(other.d_payload, allocator)
, d_flags(other.d_flags)
, d_compressionAlgorithmType(other.d_compressionAlgorithmType)
{
    // NOTHING
}

// Create guid from valid hex rep

/// Above hex string represents a valid guid with these values:
/// TS = bdlb::BigEndianUint64::make(12345)
/// IP = 98765
/// ID = 9999
const char HEX_REP[] = "0000000000003039CD8101000000270F";

/// Return a 15-bit random number between the specified `min` and the
/// specified `max`, inclusive.  The behavior is undefined unless `min >= 0`
/// and `max >= min`.
int generateRandomInteger(int min, int max)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(min >= 0);
    BSLS_ASSERT_OPT(max >= min);

    return min + (bsl::rand() % (max - min + 1));
}

/// Populate the specified `subQueueInfos` with the specified
/// `numSubQueueInfos` number of randomly generated SubQueueInfos. Note that
/// `subQueueInfos` will be cleared.
void generateSubQueueInfos(bmqp::Protocol::SubQueueInfosArray* subQueueInfos,
                           int numSubQueueInfos)
{
    BSLS_ASSERT_SAFE(subQueueInfos);
    BSLS_ASSERT_SAFE(numSubQueueInfos >= 0);

    subQueueInfos->clear();

    for (int i = 0; i < numSubQueueInfos; ++i) {
        const unsigned int subQueueId = generateRandomInteger(0, 120);
        subQueueInfos->push_back(bmqp::SubQueueInfo(subQueueId));
    }

    BSLS_ASSERT_SAFE(subQueueInfos->size() ==
                     static_cast<unsigned int>(numSubQueueInfos));
}

#ifdef BMQ_ENABLE_MSG_GROUPID
/// Populate the specified `msgGroupId` with a random Group Id.
static void generateMsgGroupId(bmqp::Protocol::MsgGroupId* msgGroupId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(msgGroupId);

    bmqu::MemOutStream oss(bmqtst::TestHelperUtil::allocator());
    oss << "gid:" << generateRandomInteger(0, 120);
    *msgGroupId = oss.str();
}
#endif

/// Append at least `atLeastLen` bytes to the specified `blob` and populate
/// the specified `payloadLen` with the number of bytes appended.
void populateBlob(bdlbb::Blob* blob, int* payloadLen, int atLeastLen)
{
    const char* FIXED_PAYLOAD = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRS"
                                "TUVWXYZ0123456789abcdefghijklmno";

    const int FIXED_PAYLOAD_LEN = bsl::strlen(FIXED_PAYLOAD);

    int numIters = atLeastLen / FIXED_PAYLOAD_LEN + 1;

    for (int i = 0; i < numIters; ++i) {
        bdlbb::BlobUtil::append(blob, FIXED_PAYLOAD, FIXED_PAYLOAD_LEN);
    }

    *payloadLen = blob->length();
}

bmqt::EventBuilderResult::Enum
appendMessage(size_t                    iteration,
              bmqp::PushEventBuilder*   peb,
              bsl::vector<Data>*        vec,
              bdlbb::BlobBufferFactory* bufferFactory,
              bslma::Allocator*         allocator)
{
    Data data(bufferFactory, allocator);
    data.d_guid.fromHex(HEX_REP);
    data.d_qid                      = iteration;
    data.d_flags                    = 0;
    data.d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;

    // Between [1, 15] SubQueueInfos
    int numSubQueueInfos = generateRandomInteger(1, 15);
    if (iteration % 7 == 0) {
        // Every 7th iteration we force a PushMessage with no SubQueueInfo
        // option
        numSubQueueInfos = 0;
    }

    generateSubQueueInfos(&data.d_subQueueInfos, numSubQueueInfos);

    bmqt::EventBuilderResult::Enum rc = peb->addSubQueueInfosOption(
        data.d_subQueueInfos);
    if (rc != bmqt::EventBuilderResult::e_SUCCESS) {
        return rc;  // RETURN
    }

#ifdef BMQ_ENABLE_MSG_GROUPID
    // Every 3rd iteration we don't add a Group Id.
    if (iteration % 3) {
        generateMsgGroupId(&data.d_msgGroupId.makeValue());
        rc = peb->addMsgGroupIdOption(data.d_msgGroupId.value());
        if (rc != bmqt::EventBuilderResult::e_SUCCESS) {
            return rc;  // RETURN
        }
    }
#endif

    bdlbb::Blob payload(bufferFactory, allocator);
    const int   blobSize = generateRandomInteger(0, 1024);
#ifdef BMQ_ENABLE_MSG_GROUPID
    const bmqp::Protocol::MsgGroupId str(blobSize, 'x', allocator);
    bdlbb::BlobUtil::append(&payload, str.c_str(), blobSize);
#endif

    data.d_payload = payload;

    vec->push_back(data);

    return peb->packMessage(data.d_payload,
                            data.d_qid,
                            data.d_guid,
                            data.d_flags,
                            data.d_compressionAlgorithmType);
}

bool isDefaultSubscription(
    const bmqp::Protocol::SubQueueInfosArray& subQueueInfos)
{
    return subQueueInfos.size() == 1U &&
           (subQueueInfos[0].id() ==
            bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID);
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
//  - Create a PUSH event with non compressed simple payload, an option
//    specifying subQueueIds and iterate over it using PushMessageIterator
//    iterating over the header, payload and verify the same.
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        bmqtst::TestHelperUtil::allocator());

    const int               queueId = 4321;
    const bmqt::MessageGUID guid;
    const char*             buffer = "abcdefghijklmnopqrstuvwxyz";
    const int               flags  = 0;
    const int               numSubQueueInfos =
        bmqp::Protocol::SubQueueInfosArray::static_size + 4;
    // Use a value for 'numSubQueueInfos' which extends beyond 'static' part of
    // the 'SubQueueInfosArray'.

    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));

    ASSERT_EQ(static_cast<unsigned int>(payload.length()),
              bsl::strlen(buffer));

    // Create PushEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add SubQueueInfo option
    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);
    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    // 'eventSize()' excludes unpacked messages
    ASSERT_LT(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    // But the option is written to the underlying blob
    rc = peb.packMessage(payload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(payload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);

    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(flags, pushIter.header().flags());

    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());
    ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
              pushIter.header().compressionAlgorithmType());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadMessagePayload(&payloadBlob));

    ASSERT_EQ(payload.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payloadBlob.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payload.length(), payloadBlob.length());

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payload, payloadBlob));
    ASSERT_EQ(true, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                  optionsView.end());

    bmqp::Protocol::SubQueueInfosArray retrievedSubQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, optionsView.loadSubQueueInfosOption(&retrievedSubQueueInfos));

    ASSERT_EQ(subQueueInfos.size(), retrievedSubQueueInfos.size());
    for (size_t i = 0; i < subQueueInfos.size(); ++i) {
        ASSERT_EQ_D(i, subQueueInfos[i], retrievedSubQueueInfos[i]);
    }

    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test2_buildEventBackwardsCompatibility()
// ------------------------------------------------------------------------
// BACKWARDS COMPATIBILITY
//
// Concerns:
//   Exercise adding SubQueueInfos without packing the rdaCounter.
//
// Plan:
//  - Create a PUSH event with non compressed simple payload, an option
//    specifying subQueueIds (but not packing the rdaCounter) and iterate
//    over it using PushMessageIterator iterating over the header, payload
//    and verify the same.
//
// Testing:
//   Backwards compatibility
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BACKWARDS COMPATIBILITY");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        bmqtst::TestHelperUtil::allocator());

    const int               queueId = 4321;
    const bmqt::MessageGUID guid;
    const char*             buffer = "abcdefghijklmnopqrstuvwxyz";
    const int               flags  = 0;
    const int               numSubQueueInfos =
        bmqp::Protocol::SubQueueInfosArray::static_size + 4;
    // Use a value for 'numSubQueueInfos' which extends beyond 'static' part of
    // the 'SubQueueInfosArray'.

    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));

    ASSERT_EQ(static_cast<unsigned int>(payload.length()),
              bsl::strlen(buffer));

    // Create PushEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add SubQueueInfo option
    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

    bmqt::EventBuilderResult::Enum rc =
        peb.addSubQueueInfosOption(subQueueInfos, false);
    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    // 'eventSize()' excludes unpacked messages
    ASSERT_LT(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    // But the option is written to the underlying blob
    rc = peb.packMessage(payload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(payload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);

    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(flags, pushIter.header().flags());

    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());
    ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
              pushIter.header().compressionAlgorithmType());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadMessagePayload(&payloadBlob));

    ASSERT_EQ(payload.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payloadBlob.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payload.length(), payloadBlob.length());

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payload, payloadBlob));
    ASSERT_EQ(true, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) !=
                  optionsView.end());

    bmqp::Protocol::SubQueueInfosArray retrievedSubQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, optionsView.loadSubQueueInfosOption(&retrievedSubQueueInfos));

    ASSERT_EQ(subQueueInfos.size(), retrievedSubQueueInfos.size());
    for (size_t i = 0; i < subQueueInfos.size(); ++i) {
        ASSERT_EQ_D(i, subQueueInfos[i], retrievedSubQueueInfos[i]);
    }

    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test3_buildEventWithPackedOption()
// ------------------------------------------------------------------------
// PACKED OPTION
//
// Concerns:
//   Exercise adding a packed option, such as having SubQueueInfos for the
//   default subQueueId with an rdaCounter.
//
// Plan:
//  - Create a PUSH event with non compressed simple payload, an option
//    specifying the default subQueueId and some 'rdaCounter' and iterate
//    over it using PushMessageIterator iterating over the header, payload
//    and verify the same.
//
// Testing:
//   Packed Option
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PACKED OPTION");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        bmqtst::TestHelperUtil::allocator());

    const int               queueId = 4321;
    const bmqt::MessageGUID guid;
    const char*             buffer = "abcdefghijklmnopqrstuvwxyz";
    const int               flags  = 0;

    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));

    ASSERT_EQ(static_cast<unsigned int>(payload.length()),
              bsl::strlen(buffer));

    // Create PushEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add SubQueueInfo option
    subQueueInfos.push_back(
        bmqp::SubQueueInfo(bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID));
    subQueueInfos.back().rdaInfo().setCounter(10);

    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);

    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    // 'eventSize()' excludes unpacked messages
    ASSERT_LT(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    // But the option is written to the underlying blob
    rc = peb.packMessage(payload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(payload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);

    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(flags, pushIter.header().flags());

    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());
    ASSERT_EQ(bmqt::CompressionAlgorithmType::e_NONE,
              pushIter.header().compressionAlgorithmType());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadMessagePayload(&payloadBlob));

    ASSERT_EQ(payload.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payloadBlob.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payload.length(), payloadBlob.length());

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payload, payloadBlob));
    ASSERT_EQ(true, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                  optionsView.end());

    bmqp::Protocol::SubQueueInfosArray retrievedSubQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, optionsView.loadSubQueueInfosOption(&retrievedSubQueueInfos));

    ASSERT_EQ(subQueueInfos.size(), retrievedSubQueueInfos.size());
    for (size_t i = 0; i < subQueueInfos.size(); ++i) {
        ASSERT_EQ_D(i, subQueueInfos[i], retrievedSubQueueInfos[i]);
    }

    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test4_buildEventWithMultipleMessages()
// --------------------------------------------------------------------
// Build an event with multiple PUSH msgs. Iterate and verify
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BUILD EVENT WITH MULTIPLE MESSAGES");

    // Create PushEventBuilder
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());

    bsl::vector<Data> data(bmqtst::TestHelperUtil::allocator());

    const size_t k_NUM_MSGS = 1000;

    for (size_t dataIdx = 0; dataIdx < k_NUM_MSGS; ++dataIdx) {
        bmqt::EventBuilderResult::Enum rc = appendMessage(
            dataIdx,
            &peb,
            &data,
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

        ASSERT_EQ_D(dataIdx, rc, bmqt::EventBuilderResult::e_SUCCESS);
    }

    // Iterate and check
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);
    ASSERT_EQ(true, pushIter.isValid());

    size_t dataIndex = 0;

    while (pushIter.next() && dataIndex < k_NUM_MSGS) {
        const Data& D = data[dataIndex];

        ASSERT_EQ_D(dataIndex, true, pushIter.isValid());
        ASSERT_EQ_D(dataIndex, 0, pushIter.header().flags());
        ASSERT_EQ_D(dataIndex, D.d_guid, pushIter.header().messageGUID());
        ASSERT_EQ_D(dataIndex, D.d_qid, pushIter.header().queueId());

        bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
        ASSERT_EQ_D(dataIndex, 0, pushIter.loadMessagePayload(&payloadBlob));

        ASSERT_EQ_D(dataIndex,
                    payloadBlob.length(),
                    pushIter.messagePayloadSize());

        ASSERT_EQ_D(dataIndex,
                    0,
                    bdlbb::BlobUtil::compare(payloadBlob, D.d_payload));

        bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
        ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
        ASSERT_EQ(true, optionsView.isValid());

        const bool hasSubQueueInfos = D.d_subQueueInfos.size() > 0;
        ASSERT_EQ(hasSubQueueInfos,
                  optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                      optionsView.end());
        if (hasSubQueueInfos) {
            bmqp::Protocol::SubQueueInfosArray retrievedSQInfos(
                bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(0,
                      optionsView.loadSubQueueInfosOption(&retrievedSQInfos));
            ASSERT_EQ(D.d_subQueueInfos.size(), retrievedSQInfos.size());
            for (size_t i = 0; i < D.d_subQueueInfos.size(); ++i) {
                ASSERT_EQ_D(i, D.d_subQueueInfos[i], retrievedSQInfos[i]);
            }
        }
#ifdef BMQ_ENABLE_MSG_GROUPID
        const bool hasMsgGroupId = !D.d_msgGroupId.isNull();
        ASSERT_EQ(hasMsgGroupId,
                  optionsView.find(bmqp::OptionType::e_MSG_GROUP_ID) !=
                      optionsView.end());
        if (hasMsgGroupId) {
            bmqp::Protocol::MsgGroupId retrievedMsgGroupId(
                bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(0,
                      optionsView.loadMsgGroupIdOption(&retrievedMsgGroupId));
            ASSERT_EQ(D.d_msgGroupId.value(), retrievedMsgGroupId);
        }
#endif
        ++dataIndex;
    }

    ASSERT_EQ(dataIndex, data.size());
    ASSERT_EQ(false, pushIter.isValid());
}

static void test5_buildEventWithPayloadTooBig()
// ------------------------------------------------------------------------
// PAYLOAD TOO BIG TEST
//
// Concerns:
//   Test behavior when trying to build *one* big message with a
//   payload that is larger than the max allowed.
//
// Plan:
//   - Generate a message with a payload that is larger than the max
//     allowed.
//
// Testing:
//   Behavior when trying to add a message with a large payload.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PAYLOAD TOO BIG");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    const bmqt::MessageGUID        guid;
    bdlbb::Blob                        bigMsgPayload(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        bmqtst::TestHelperUtil::allocator());
    const int                          queueId = 4321;
    int numSubQueueInfos = bmqp::Protocol::SubQueueInfosArray::static_size + 4;
    // Use a value for 'numSubQueueInfos' which extends beyond 'static' part of
    // the 'SubQueueInfosArray'.
    int       payloadLen = 0;
    const int flags      = 0;

    populateBlob(&bigMsgPayload,
                 &payloadLen,
                 bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT + 1);

    BSLS_ASSERT_SAFE(bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT <
                     bigMsgPayload.length());

    // Create PutEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add a valid size option
    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);
    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);

    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_LT(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    // We expect 'addSubQueueInfosOption' to write directly to the underlying
    // blob
    ASSERT_EQ(0, peb.messageCount());

    // Append a message with payload that is bigger than the max allowed
    rc = peb.packMessage(bigMsgPayload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    // Already-written options have to be removed if packing a message
    // fails
    ASSERT_EQ(0, peb.messageCount());

    // Now append a "regular"-sized message and make sure event builder
    // behaves as expected
    bdlbb::Blob regularPayload(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
    const char* regularBuffer = "abcedefghijklmnopqrstuv";

    bdlbb::BlobUtil::append(&regularPayload,
                            regularBuffer,
                            bsl::strlen(regularBuffer));

    rc = peb.packMessage(regularPayload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(regularPayload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(flags, pushIter.header().flags());
    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());

    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadMessagePayload(&payloadBlob));
    ASSERT_EQ(regularPayload.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payloadBlob.length(), pushIter.messagePayloadSize());

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payloadBlob, regularPayload));
    ASSERT_EQ(false, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) ==
                  optionsView.end());
    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test6_buildEventWithImplicitPayload()
// --------------------------------------------------------------------
// Implicit payload test
// --------------------------------------------------------------------
{
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    const bmqt::MessageGUID        guid;
    const int                      queueId = 4321;
    const int flags = bmqp::PushHeaderFlags::e_IMPLICIT_PAYLOAD;

    // Create PutEventBuilder
    bmqp::PushEventBuilder         peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());
    bmqt::EventBuilderResult::Enum rc = peb.packMessage(
        queueId,
        guid,
        flags,
        bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(sizeof(bmqp::EventHeader) + sizeof(bmqp::PushHeader),
              static_cast<unsigned int>(peb.eventSize()));
    // 'ASSERT_LESS' because of 4byte padding, even for empty payload.  If that
    // padding is removed, 'ASSERT_LESS' should change to 'ASSERT_EQ'.
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and bmqp
    // iterators are lower than bmqp builders, and thus, can be used to test
    // them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(true, pushIter.isApplicationDataImplicit());
    ASSERT_EQ(flags, pushIter.header().flags());
    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());

    bdlbb::Blob dummy(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(false, 0 == pushIter.loadMessagePayload(&dummy));
    ASSERT_EQ(0, pushIter.messagePayloadSize());
    ASSERT_EQ(false, 0 == pushIter.loadApplicationData(&dummy));
    ASSERT_EQ(0, pushIter.applicationDataSize());
    ASSERT_EQ(false, pushIter.hasMessageProperties());
    ASSERT_EQ(0, pushIter.messagePropertiesSize());
    ASSERT_EQ(false, 0 == pushIter.loadMessageProperties(&dummy));
    ASSERT_EQ(false, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_IDS_OLD) ==
                  optionsView.end());
    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test7_buildEventOptionTooBig()
// ------------------------------------------------------------------------
// OPTION TOO BIG
//
// Concerns:
//   Behavior when trying to build a message with *one* big option.
//
// Plan:
//   - Generate an option with size greater than
//     'bmqp::OptionHeader::k_MAX_SIZE' (8MB at time of writing) and
//     verify failure to add option
//   - Generate an option of valid size and a payload of valid size and
//     verify that PushEventBuilder behaves correctly.
//
// Testing:
//   Behavior of adding an option that is larger than the max allowed.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("OPTION TOO BIG TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    int                                numSubQueueInfos;
    int                                optionSize;
    bmqp::Protocol::SubQueueInfosArray subQueueInfos(
        bmqtst::TestHelperUtil::allocator());

    // No. of SubQueueInfos is exactly one more than would exceed the option
    // size limit.
    numSubQueueInfos = (bmqp::OptionHeader::k_MAX_SIZE -
                        sizeof(bmqp::OptionHeader)) /
                       bmqp::Protocol::k_WORD_SIZE;
    numSubQueueInfos += 1;

    optionSize = sizeof(bmqp::OptionHeader) +
                 numSubQueueInfos * bmqp::Protocol::k_WORD_SIZE;

    BSLS_ASSERT_SAFE(optionSize > bmqp::OptionHeader::k_MAX_SIZE);

    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);

    // Create PutEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());

    // Add option larger than maximum allowed
    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_OPTION_TOO_BIG);

#ifdef BMQ_ENABLE_MSG_GROUPID
    // Add another option larger than maximum allowed
    bmqp::Protocol::MsgGroupId msgGrIdBig1(
        bmqp::OptionHeader::k_MAX_SIZE + 1,
        'x',
        bmqtst::TestHelperUtil::allocator());

    rc = peb.addMsgGroupIdOption(msgGrIdBig1);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_OPTION_TOO_BIG);

    bmqp::Protocol::MsgGroupId msgGrIdBig2(
        bmqp::Protocol::k_MSG_GROUP_ID_MAX_LENGTH + 1,
        'x',
        bmqtst::TestHelperUtil::allocator());

    rc = peb.addMsgGroupIdOption(msgGrIdBig2);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_INVALID_MSG_GROUP_ID);
#endif

    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob()->length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add option of valid size
    numSubQueueInfos = 1;

    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);
    if (isDefaultSubscription(subQueueInfos)) {
        optionSize = sizeof(bmqp::OptionHeader);
    }
    else {
        optionSize = sizeof(bmqp::OptionHeader) +
                     numSubQueueInfos * bmqp::Protocol::k_WORD_SIZE;
    }

    BSLS_ASSERT_SAFE(optionSize <= bmqp::OptionHeader::k_MAX_SIZE);

    rc = peb.addSubQueueInfosOption(subQueueInfos);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_LE(sizeof(bmqp::EventHeader) + sizeof(bmqp::PushHeader) +
                  optionSize,
              static_cast<size_t>(peb.blob()->length()));
    // Less than or equal due to possible padding
    ASSERT_EQ(0, peb.messageCount());

    // Now append a valid-sized message and make sure event builder behaves as
    // expected
    bdlbb::Blob             regularPayload(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
    const char*             regularBuffer = "abcedefghijklmnopqrstuv";
    const int               queueId       = 1399;
    const bmqt::MessageGUID guid;
    const int               flags = 0;

    bdlbb::BlobUtil::append(&regularPayload,
                            regularBuffer,
                            bsl::strlen(regularBuffer));

    bmqp::PushHeader ph;
    ph.setFlags(flags).setQueueId(queueId).setMessageGUID(guid);

    rc = peb.packMessage(regularPayload, ph);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    int evtSizeUnpadded = sizeof(bmqp::EventHeader) +
                          sizeof(bmqp::PushHeader) + optionSize +
                          regularPayload.length();
    ASSERT_LE(evtSizeUnpadded, peb.eventSize());
    // Less than or equal due to possible padding
    ASSERT_LE(evtSizeUnpadded, peb.blob()->length());
    // Less than or equal due to possible padding
    ASSERT_EQ(1, peb.messageCount());

    // Get blob and use bmqp iterator to test.  Note that bmqp event and bmqp
    // iterators are lower than bmqp builders, and thus, can be used to test
    // them.
    bmqp::Event rawEvent(peb.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BSLS_ASSERT_SAFE(true == rawEvent.isValid());
    BSLS_ASSERT_SAFE(true == rawEvent.isPushEvent());

    bmqp::PushMessageIterator pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPushMessageIterator(&pushIter, true);
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(flags, pushIter.header().flags());
    ASSERT_EQ(queueId, pushIter.header().queueId());
    ASSERT_EQ(guid, pushIter.header().messageGUID());

    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadMessagePayload(&payloadBlob));
    ASSERT_EQ(regularPayload.length(), pushIter.messagePayloadSize());
    ASSERT_EQ(payloadBlob.length(), pushIter.messagePayloadSize());

    ASSERT_EQ(0, bdlbb::BlobUtil::compare(payloadBlob, regularPayload));
    ASSERT_EQ(true, pushIter.hasOptions());

    bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, pushIter.loadOptionsView(&optionsView));
    ASSERT_EQ(true, optionsView.isValid());
    ASSERT_EQ(true,
              optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                  optionsView.end());

    bmqp::Protocol::SubQueueInfosArray retrievedSQInfos(
        bmqtst::TestHelperUtil::allocator());
    ASSERT_EQ(0, optionsView.loadSubQueueInfosOption(&retrievedSQInfos));
    ASSERT_EQ(subQueueInfos.size(), retrievedSQInfos.size());
    for (size_t i = 0; i < subQueueInfos.size(); ++i) {
        ASSERT_EQ_D(i, subQueueInfos[i], retrievedSQInfos[i]);
    }

    ASSERT_NE(1, pushIter.next());  // we added only 1 msg
}

static void test8_buildEventTooBig()
// ------------------------------------------------------------------------
// // EVENT TOO BIG
//
// Concerns:
//   Behavior when trying to build an event with multiple msgs that
//   together would constitute an event with a size that is larger
//   than the enforced maximum.
//
// Plan:
//   - Generate a valid size message.
//   - Generate another message such that adding this message must fail
//     due to event size exceeding the maximum size allowed.
//
// Testing:
//   Behavior of adding adding multiple messages that together would
//   constitute an event with a size that is larger than the enforced
//   maximum.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EVENT TOO BIG");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    const bmqt::MessageGUID        guid;
    bdlbb::Blob                    validPayload1(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
    const int   queueId  = 4321;
    int         validLen = 0;
    const int   flags    = 0;

    validLen = bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT -
               sizeof(bmqp::EventHeader) - sizeof(bmqp::PushHeader) -
               bmqp::Protocol::k_WORD_SIZE;  // max padding

    bsl::string s(validLen, 'x', bmqtst::TestHelperUtil::allocator());

    bdlbb::BlobUtil::append(&validPayload1, s.c_str(), validLen);

    // Create PutEventBuilder
    bmqp::PushEventBuilder peb(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());

    // Add message with valid payload size
    bmqt::EventBuilderResult::Enum rc = peb.packMessage(
        validPayload1,
        queueId,
        guid,
        flags,
        bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    int evtSize = peb.eventSize();
    ASSERT_LT(validPayload1.length(), evtSize);
    ASSERT_EQ(1, peb.messageCount());

    // Add message with valid payload size such that it would make the event
    // have a size that is larger than the enforced maximum.
    bdlbb::Blob validPayload2(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&validPayload2, s.c_str(), validLen);

    int count = 1;
    while ((evtSize + sizeof(bmqp::PushHeader) + validLen) <
           bmqp::EventHeader::k_MAX_SIZE_SOFT) {
        rc = peb.packMessage(validPayload2,
                             queueId,
                             guid,
                             flags,
                             bmqt::CompressionAlgorithmType::e_NONE);
        ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
        evtSize += sizeof(bmqp::PushHeader) + validLen;
        ++count;
    }
    evtSize = peb.eventSize();  // not calculating padding
    rc      = peb.packMessage(validPayload2,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_EVENT_TOO_BIG);
    ASSERT_EQ(evtSize, peb.eventSize());
    ASSERT_EQ(count, peb.messageCount());
}

static void testN1_decodeFromFile()
// --------------------------------------------------------------------
// DECODE FROM FILE
//
// Concerns:
//   bmqp::PushEventBuilder encodes bmqp::Event so that binary data
//   can be stored into a file and then restored and decoded back.
//
// Plan:
//   1. Using bmqp::PushEventBuilder encode bmqp::EventType::e_PUSH event.
//   2. Store binary representation of this event into a file.
//   3. Read this file, decode event and verify that it contains a message
//      with expected properties and payload.
// --------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DECODE FROM FILE");

    const char  k_VALID_HEX_REP[] = "ABCDEF0123456789ABCDEF0123456789";
    const char* k_PAYLOAD         = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN     = bsl::strlen(k_PAYLOAD);
    const int   k_QID             = 9876;
    const int   k_SIZE            = 128;
    char        buf[k_SIZE]       = {0};
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bdlbb::Blob outBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payloadBlob(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream             os(bmqtst::TestHelperUtil::allocator());
    bmqp::PushMessageIterator      pushIter(&bufferFactory,
                                       bmqtst::TestHelperUtil::allocator());
    bmqt::MessageGUID              messageGUID;
    bdlb::Guid                     guid;

    messageGUID.fromHex(k_VALID_HEX_REP);
    bdlbb::BlobUtil::append(&payloadBlob, k_PAYLOAD, k_PAYLOAD_LEN);

    // Create PutEventBuilder
    bmqp::PushEventBuilder obj(&blobSpPool,
                               bmqtst::TestHelperUtil::allocator());

    // Pack one msg
    obj.packMessage(payloadBlob,
                    k_QID,
                    messageGUID,
                    0,
                    bmqt::CompressionAlgorithmType::e_NONE);
    guid = bdlb::GuidUtil::generate();

    os << "msg_push_" << guid << ".bin" << bsl::ends;

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
    bsl::ofstream ofile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ofile.good() == true);

    bdlbb::BlobUtil::copy(buf, *obj.blob(), 0, obj.blob()->length());
    ofile.write(buf, k_SIZE);
    ofile.close();
    bsl::memset(buf, 0, k_SIZE);

    // Read blob from file
    bsl::ifstream ifile(os.str().data(), bsl::ios_base::binary);

    BSLS_ASSERT(ifile.good() == true);

    ifile.read(buf, k_SIZE);
    ifile.close();

    bsl::shared_ptr<char> dataBufferSp(buf,
                                       bslstl::SharedPtrNilDeleter(),
                                       bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobBuffer     dataBlobBuffer(dataBufferSp, k_SIZE);

    outBlob.appendDataBuffer(dataBlobBuffer);
    outBlob.setLength(obj.blob()->length());

    ASSERT_EQ(bdlbb::BlobUtil::compare(*obj.blob(), outBlob), 0);

    // Decode event
    bmqp::Event rawEvent(&outBlob, bmqtst::TestHelperUtil::allocator());

    ASSERT_EQ(rawEvent.isPushEvent(), true);

    rawEvent.loadPushMessageIterator(&pushIter, true);

    ASSERT_EQ(1, pushIter.next());
    ASSERT_EQ(k_QID, pushIter.header().queueId());
    ASSERT_EQ(messageGUID,
              pushIter.header().messageGUID());  // k_VALID_HEX_REP

    ASSERT_EQ(pushIter.loadMessagePayload(&payloadBlob), 0);
    ASSERT_EQ(pushIter.messagePayloadSize(), k_PAYLOAD_LEN);

    bmqp::MessageProperties prop(bmqtst::TestHelperUtil::allocator());
    int                     res, compareResult;
    res = bmqu::BlobUtil::compareSection(&compareResult,
                                         payloadBlob,
                                         bmqu::BlobPosition(),
                                         k_PAYLOAD,
                                         k_PAYLOAD_LEN);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, compareResult);
    ASSERT_EQ(false, pushIter.hasMessageProperties());
    ASSERT_EQ(0, pushIter.loadMessageProperties(&prop));
    ASSERT_EQ(0, prop.numProperties());
    ASSERT_EQ(true, pushIter.isValid());
    ASSERT_EQ(0, pushIter.next());  // we added only 1 msg
    ASSERT_EQ(false, pushIter.isValid());
}
// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.  TBD:
    // figure out the right way to "fix" this.
    Data dummy(static_cast<bdlbb::BlobBufferFactory*>(0),
               bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<
            bslmf::NestedTraitDeclaration<Data, bslma::UsesBslmaAllocator> >(
            dummy));

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    unsigned int seed = bsl::time(NULL);
    bsl::srand(seed);
    PV("Seed: " << seed);

    // TODO_POISON_PILL Test new addSubQueueInfosOption() once it starts
    //                  encoding RDA counters.
    switch (_testCase) {
    case 0:
    case 8: test8_buildEventTooBig(); break;
    case 7: test7_buildEventOptionTooBig(); break;
    case 6: test6_buildEventWithImplicitPayload(); break;
    case 5: test5_buildEventWithPayloadTooBig(); break;
    case 4: test4_buildEventWithMultipleMessages(); break;
    case 3: test3_buildEventWithPackedOption(); break;
    case 2: test2_buildEventBackwardsCompatibility(); break;
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
