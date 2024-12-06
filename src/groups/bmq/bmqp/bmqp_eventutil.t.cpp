// Copyright 2023 Bloomberg Finance L.P.
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

// bmqp_eventutil.t.cpp                                               -*-C++-*-
#include <bmqp_eventutil.h>

// BMQ
#include <bmqp_event.h>
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_queueid.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct Data {
    // DATA
    bmqt::MessageGUID                    d_guid;
    int                                  d_qid;
    bmqp::Protocol::SubQueueInfosArray   d_subQueueInfos;
    bdlbb::Blob                          d_properties;
    bdlbb::Blob                          d_payload;
    int                                  d_flags;
    bmqt::CompressionAlgorithmType::Enum d_compressionAlgorithmType;
    bool                                 d_isSubQueueInfo;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Data, bslma::UsesBslmaAllocator)

    // CREATORS
    Data(bdlbb::BlobBufferFactory* bufferFactory, bslma::Allocator* allocator);
    Data(const Data& other, bslma::Allocator* allocator);
};

// CREATORS
Data::Data(bdlbb::BlobBufferFactory* bufferFactory,
           bslma::Allocator*         allocator)
: d_subQueueInfos(allocator)
, d_properties(bufferFactory, allocator)
, d_payload(bufferFactory, allocator)
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
{
    // NOTHING
}

Data::Data(const Data& other, bslma::Allocator* allocator)
: d_guid(other.d_guid)
, d_qid(other.d_qid)
, d_subQueueInfos(other.d_subQueueInfos, allocator)
, d_properties(other.d_properties, allocator)
, d_payload(other.d_payload, allocator)
, d_flags(other.d_flags)
, d_compressionAlgorithmType(other.d_compressionAlgorithmType)
, d_isSubQueueInfo(other.d_isSubQueueInfo)
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
/// `numSubQueueInfos` number of generated subQueueInfos. Clear the
/// `subQueueInfos` prior to populating it.  The behavior is undefined
/// unless `numSubQueueInfos >= 0` and `numSubQueueInfos <= 200`.
static void
generateSubQueueInfos(bmqp::Protocol::SubQueueInfosArray* subQueueInfos,
                      int                                 numSubQueueInfos)
{
    BSLS_ASSERT_OPT(subQueueInfos);
    BSLS_ASSERT_OPT(numSubQueueInfos >= 0);
    BSLS_ASSERT_OPT(numSubQueueInfos <= 200);

    subQueueInfos->clear();

    for (int i = 0; i < numSubQueueInfos; ++i) {
        subQueueInfos->push_back(bmqp::SubQueueInfo(i + 1));
    }
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

/// Append to the specified `data` an entry having the specified
/// `numSubQueueInfos` and `payloadLength` using the specified
/// `bufferFactory` and `allocator`.  The behavior is undefined unless
/// `numSubQueueInfos >= 0` and `payloadLength >= 0`.
static void appendDatum(bsl::vector<Data>*        data,
                        int                       numSubQueueInfos,
                        int                       payloadLength,
                        bdlbb::BlobBufferFactory* bufferFactory,
                        bslma::Allocator*         allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(data);
    BSLS_ASSERT_OPT(numSubQueueInfos >= 0);
    BSLS_ASSERT_OPT(payloadLength >= 0);

    Data datum(bufferFactory, allocator);
    datum.d_qid   = data->size();
    datum.d_flags = 0;
    // Use the new SubQueueInfo option
    datum.d_isSubQueueInfo = true;

    // Generate SubQueueInfos
    generateSubQueueInfos(&datum.d_subQueueInfos, numSubQueueInfos);

    // Populate blob
    int numPopulated = 0;
    populateBlob(&datum.d_payload, &numPopulated, payloadLength);
    BSLS_ASSERT_OPT(numPopulated < bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT);

    data->push_back(datum);
}

/// Add to the event being built by the specified `PushEventBuilder`
/// messages using the specified `data`.
static void appendMessages(bmqp::PushEventBuilder*  pushEventBuilder,
                           const bsl::vector<Data>& data)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(pushEventBuilder);

    bmqt::EventBuilderResult::Enum rc;

    for (bsl::vector<Data>::size_type i = 0; i < data.size(); ++i) {
        const Data& D = data[i];

        // Add SubQueueInfos
        rc = pushEventBuilder->addSubQueueInfosOption(D.d_subQueueInfos,
                                                      D.d_isSubQueueInfo);
        // packRdaCounter
        BSLS_ASSERT_OPT(rc == bmqt::EventBuilderResult::e_SUCCESS);

        // Pack Message
        rc = pushEventBuilder->packMessage(D.d_payload,
                                           D.d_qid,
                                           D.d_guid,
                                           D.d_flags,
                                           D.d_compressionAlgorithmType);
        BSLS_ASSERT_OPT(rc == bmqt::EventBuilderResult::e_SUCCESS);
    }
}

bool find(bmqp::EventUtilEventInfo& eventInfo,
          int                       qId,
          unsigned int              subcriptionId)
{
    size_t i = 0;
    for (; i < eventInfo.d_ids.size(); ++i) {
        if (eventInfo.d_ids[i].d_subscriptionId == subcriptionId &&
            eventInfo.d_ids[i].d_header.queueId() == qId) {
            break;
        }
    }
    return (i < eventInfo.d_ids.size());
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
//   1) Create an event composed of 5 messages. Msg1 has one SubQueueId,
//      Msg2 has three SubQueueIds, Msg3 has one SubQueueId, Msg4 has no
//      SubQueueId, Msg5 has one SubQueueId.
//   2) Flatten the event.
//   3) Verify that the flattened event has 7 messages.  The first message
//      should have the payload of Msg1.  The second, third, and fourth
//      messages should have the payload of Msg2.  The fifth message should
//      have the payload of Msg3.  The sixth message should have the
//      payload of Msg4.  And the seventh message should have the payload
//      of Msg5.
//
// Testing:
//   Basic functionality of 'flattenPushEvent(...)'.
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
    bmqp::PushEventBuilder pushEventBuilder(
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>      data(bmqtst::TestHelperUtil::allocator());
    int                    payloadLength    = 0;
    int                    numSubQueueInfos = 0;
    int                    rc               = 0;

    // 1) Event composed of 5 messages
    // Msg1
    payloadLength    = generateRandomInteger(1, 120);
    numSubQueueInfos = 1;
    appendDatum(&data,
                numSubQueueInfos,
                payloadLength,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

    // Msg2
    payloadLength    = generateRandomInteger(1, 120);
    numSubQueueInfos = 3;
    appendDatum(&data,
                numSubQueueInfos,
                payloadLength,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

    // Msg3
    payloadLength    = generateRandomInteger(1, 120);
    numSubQueueInfos = 1;
    appendDatum(&data,
                numSubQueueInfos,
                payloadLength,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

    // Msg4
    payloadLength    = generateRandomInteger(1, 120);
    numSubQueueInfos = 0;
    appendDatum(&data,
                numSubQueueInfos,
                payloadLength,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

    // Msg5
    payloadLength    = generateRandomInteger(1, 120);
    numSubQueueInfos = 1;
    appendDatum(&data,
                numSubQueueInfos,
                payloadLength,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator());

    // Create event
    appendMessages(&pushEventBuilder, data);
    bmqp::Event event(pushEventBuilder.blob().get(),
                      bmqtst::TestHelperUtil::allocator());

    // 2) Flatten the event
    bsl::vector<bmqp::EventUtilEventInfo> eventInfos(
        bmqtst::TestHelperUtil::allocator());
    rc = bmqp::EventUtil::flattenPushEvent(
        &eventInfos,
        event,
        &bufferFactory,
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(eventInfos.size(), 1u);

    // 3) Verify that the flattened event has the expected messages.
    bmqp::Event               flattenedEvent(&(eventInfos[0].d_blob),
                               bmqtst::TestHelperUtil::allocator());
    bmqp::PushMessageIterator msgIterator(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
    flattenedEvent.loadPushMessageIterator(&msgIterator, true);

    for (bsl::vector<Data>::size_type i = 0; i < data.size(); ++i) {
        const Data& D = data[i];

        // Check matching SubQueueInfos and payload
        BSLS_ASSERT_OPT(msgIterator.isValid());

        if (D.d_subQueueInfos.size() == 0) {
            // No SubQueueInfos in original message, so verify that it appears
            // once in the flattened event and only in the message if the new
            // SubQueueInfo is being used.
            BSLS_ASSERT_OPT(msgIterator.hasOptions() && D.d_isSubQueueInfo);
            rc = msgIterator.next();
            BSLS_ASSERT_OPT(rc == 1);

            bdlbb::Blob payload(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
            rc = msgIterator.loadMessagePayload(&payload);
            BSLS_ASSERT_OPT(rc == 0);

            BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(D.d_payload, payload),
                             0);

            // Verify that 'eventInfo' contains the queueId pair (id, subId)
            // corresponding to this message
            const int          id = D.d_qid;
            const unsigned int subcriptionId =
                bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;

            BMQTST_ASSERT(find(eventInfos[0], id, subcriptionId));

            continue;  // CONTINUE
        }

        // Original message has one or more SubQueueInfo, so verify that the
        // message appears exactly once for each of the SubQueueInfo, and
        // moreover that the order of the SubQueueInfos in the original message
        // determines the ordering of the replicated messages in the flattened
        // event.
        for (bmqp::Protocol::SubQueueInfosArray::size_type j = 0;
             j < D.d_subQueueInfos.size();
             ++j) {
            rc = msgIterator.next();
            BSLS_ASSERT_OPT(rc == 1);
            BSLS_ASSERT_OPT(msgIterator.hasOptions());

            bdlbb::Blob payload(&bufferFactory,
                                bmqtst::TestHelperUtil::allocator());
            rc = msgIterator.loadMessagePayload(&payload);
            BSLS_ASSERT_OPT(rc == 0);

            BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(D.d_payload, payload),
                             0);

            bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
            rc = msgIterator.loadOptionsView(&optionsView);
            BSLS_ASSERT_OPT(rc == 0);
            BSLS_ASSERT_OPT(optionsView.isValid());
            BSLS_ASSERT_OPT(
                optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
                optionsView.end());
            bmqp::Protocol::SubQueueInfosArray subQueueInfos(
                bmqtst::TestHelperUtil::allocator());
            rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
            BSLS_ASSERT_OPT(rc == 0);
            BSLS_ASSERT_OPT(subQueueInfos.size() == 1);

            BMQTST_ASSERT_EQ(D.d_subQueueInfos[j], subQueueInfos[0]);

            // Verify that 'eventInfo' contains the queueId pair (id, subId)
            // corresponding to this message
            const int          id            = D.d_qid;
            const unsigned int subcriptionId = subQueueInfos[0].id();

            BMQTST_ASSERT(find(eventInfos[0], id, subcriptionId));
        }
    }

    {
        // Temporary workaround to suppress the 'unused operator
        // NestedTraitDeclaration' warning/error generated by clang.
        //
        // TBD: figure out the right way to "fix" this.

        Data dummy(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        static_cast<void>(
            static_cast<
                bslmf::NestedTraitDeclaration<Data,
                                              bslma::UsesBslmaAllocator> >(
                dummy));
    }
}

static void test2_flattenExplodesEvent()
// ------------------------------------------------------------------------
// FLATTEN EXPLODES EVENT
//
// Concerns:
//   If converting the messages with multiple SubQueueIds in an event to
//   multiple messages with each one of the SubQueueId would yield an event
//   that is larger than the enforced maximum, then flattening the event
//   results in multiple blobs constituting separate, yet equivalently
//   ordered, events.
//
// Plan:
//   1) Create an event composed of one message having a payload of size
//      a little over the quarter of the maximum enforced size and four
//      SubQueueIds.
//   2) Flatten the event.
//   3) Verify that the flattening results in two event blobs, first having
//      15 messages and the second 1 with one SubQueueId each.
//
// Testing:
//   Flattening an event having a message with more than one SubQueueId
//   and a big enough payload to cause the flattening to spill over to
//   multiple event blobs.
//     - 'flattenPushEvent(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FLATTEN EXPLODES EVENT");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PushEventBuilder pushEventBuilder(
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    bsl::vector<Data>      data(bmqtst::TestHelperUtil::allocator());
    int                    payloadLength  = 0;
    int                    numSubQueueIds = 0;
    int                    rc             = 0;

    // 1) Event composed of one message having a payload of size one third the
    //    maximum enforced size and four SubQueueIds.
    // Msg1
    payloadLength  = bmqp::PushHeader::k_MAX_PAYLOAD_SIZE_SOFT / 2;
    numSubQueueIds = 4;

    int count = 0;
    int total = 0;

    while (total < bmqp::EventHeader::k_MAX_SIZE_SOFT / 4) {
        appendDatum(&data,
                    numSubQueueIds,
                    payloadLength,
                    &bufferFactory,
                    bmqtst::TestHelperUtil::allocator());
        total += data[count].d_payload.length();
        ++count;
    }

    // Create event
    appendMessages(&pushEventBuilder, data);
    bmqp::Event event(pushEventBuilder.blob().get(),
                      bmqtst::TestHelperUtil::allocator());

    // 2) Flatten the event
    bsl::vector<bmqp::EventUtilEventInfo> eventInfos(
        bmqtst::TestHelperUtil::allocator());
    rc = bmqp::EventUtil::flattenPushEvent(
        &eventInfos,
        event,
        &bufferFactory,
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(eventInfos.size(), 2u);

    // 3) Verify that the flattening results in two event blobs, each having
    //    two messages with one SubQueueId each.
    bmqp::PushMessageIterator msgIterator(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
    int                       idx = 0;

    // 1st flattened event
    bmqp::Event flattenedEvent1(&(eventInfos[0].d_blob),
                                bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(eventInfos[0].d_ids.size(),
                     static_cast<size_t>(count * numSubQueueIds - 1));

    flattenedEvent1.loadPushMessageIterator(&msgIterator, true);
    BSLS_ASSERT_OPT(msgIterator.isValid());

    // First message
    rc = msgIterator.next();
    BSLS_ASSERT_OPT(rc == 1);
    BSLS_ASSERT_OPT(msgIterator.hasOptions());

    // Verify payload
    {
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadMessagePayload(&payload);
        BSLS_ASSERT_OPT(rc == 0);

        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(data[0].d_payload, payload),
                         0);
    }

    // Verify SubQueueInfos
    {
        bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadOptionsView(&optionsView);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(optionsView.isValid());
        BSLS_ASSERT_OPT(
            optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
            optionsView.end());
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(subQueueInfos.size() == 1);

        BMQTST_ASSERT_EQ(data[0].d_subQueueInfos[0], subQueueInfos[0]);
    }

    // Verify that 'eventInfo' contains the queueId pair (id, subId)
    // corresponding to this message

    for (size_t i = 0; i < eventInfos[0].d_ids.size(); ++i) {
        BMQTST_ASSERT_EQ(eventInfos[0].d_ids[i].d_subscriptionId,
                         i % count + 1);
        BMQTST_ASSERT_EQ(size_t(eventInfos[0].d_ids[i].d_header.queueId()),
                         i / count);
    }

    ++idx;

    // Second message
    rc = msgIterator.next();
    BSLS_ASSERT_OPT(rc == 1);
    BSLS_ASSERT_OPT(msgIterator.hasOptions());

    // Verify payload
    {
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadMessagePayload(&payload);
        BSLS_ASSERT_OPT(rc == 0);

        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(data[0].d_payload, payload),
                         0);
    }

    // Verify SubQueueInfos
    {
        bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadOptionsView(&optionsView);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(optionsView.isValid());
        BSLS_ASSERT_OPT(
            optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
            optionsView.end());
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(subQueueInfos.size() == 1);

        BMQTST_ASSERT_EQ(data[0].d_subQueueInfos[idx], subQueueInfos[0]);
    }

    idx = count - 1;  // the last one did not fit the first event

    // 2nd flattened event
    bmqp::Event flattenedEvent2(&(eventInfos[1].d_blob),
                                bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(eventInfos[1].d_ids.size(), 1u);

    flattenedEvent2.loadPushMessageIterator(&msgIterator, true);
    BSLS_ASSERT_OPT(msgIterator.isValid());

    // 1st message in tne second event
    rc = msgIterator.next();
    BSLS_ASSERT_OPT(rc == 1);
    BSLS_ASSERT_OPT(msgIterator.hasOptions());

    // Verify payload
    {
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadMessagePayload(&payload);
        BSLS_ASSERT_OPT(rc == 0);

        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(data[idx].d_payload,
                                                  payload),
                         0);
    }

    // Verify SubQueueInfos
    {
        bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadOptionsView(&optionsView);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(optionsView.isValid());
        BSLS_ASSERT_OPT(
            optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
            optionsView.end());
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(subQueueInfos.size() == 1);

        BMQTST_ASSERT_EQ(data[idx].d_subQueueInfos[idx].id(),
                         subQueueInfos[0].id());

        const int          qId           = data[idx].d_qid;
        const unsigned int subcriptionId = data[idx].d_subQueueInfos[idx].id();
        BMQTST_ASSERT(find(eventInfos[1], qId, subcriptionId));
    }

    // Verify that 'eventInfo' contains the queueId (queueId, subQueueId)
    // pair corresponding to this message

    BMQTST_ASSERT_EQ(eventInfos[1].d_ids[0].d_subscriptionId,
                     data[count - 1].d_subQueueInfos.back().id());
    BMQTST_ASSERT_EQ(eventInfos[1].d_ids[0].d_header.queueId(), count - 1);

    // No more messages
    rc = msgIterator.next();
}

static void test3_flattenWithMessageProperties()
// ------------------------------------------------------------------------
// FLATTEN WITH MESSAGE PROPERTIES
//
// Concerns:
//   If we have an event having a message with message properties and
//   multiple SubQueueIds, then flattening the event results in an event
//   blob containing the original message with the original message
//   properties, per SubQueueId (in other words, message properties are
//   copied over during flattening).
//
// Plan:
//   1) Create an event composed of one message having message properties
//      and two SubQueueIds.
//   2) Flatten the event.
//   3) Verify that the flattening results in one event blob containing
//      two messages with the original message properties and message
//      payload and corresponding to the respective SubQueueIds.
//
// Testing:
//   Flattening an event having a message with message properties and
//   multiple SubQueueIds.
//     - 'flattenPushEvent(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("FLATTEN WITH MESSAGE PROPERTIES");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::MessageProperties msgProperties(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob             appData(bmqtst::TestHelperUtil::allocator());
    bmqp::PushEventBuilder  pushEventBuilder(
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    bmqt::EventBuilderResult::Enum result;
    int                            payloadLength    = 0;
    int                            numSubQueueInfos = 0;
    int                            rc               = 0;
    int                            qid   = generateRandomInteger(1, 200);
    int                            flags = 0;

    // 1) Create an event composed of one message having message properties
    //    and two SubQueueIds.
    Data datum(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bmqp::PushHeaderFlagUtil::setFlag(
        &flags,
        bmqp::PushHeaderFlags::e_MESSAGE_PROPERTIES);

    datum.d_qid   = qid;
    datum.d_flags = flags;

    // Set and append the msgProperties
    rc = msgProperties.setPropertyAsBool("bool", true);
    BSLS_ASSERT_OPT(rc == 0);

    rc = msgProperties.setPropertyAsChar("char", 'c');
    BSLS_ASSERT_OPT(rc == 0);

    rc = msgProperties.setPropertyAsShort("short", 2);
    BSLS_ASSERT_OPT(rc == 0);

    rc = msgProperties.setPropertyAsInt32("int32", 32);
    BSLS_ASSERT_OPT(rc == 0);

    rc = msgProperties.setPropertyAsInt64("int64", 64);
    BSLS_ASSERT_OPT(rc == 0);

    rc = msgProperties.setPropertyAsString("string", "value");
    BSLS_ASSERT_OPT(rc == 0);

    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();
    datum.d_properties = msgProperties.streamOut(&bufferFactory, logic);
    P(datum.d_properties.length());

    // Set and append the payload
    payloadLength = generateRandomInteger(1, 120);
    populateBlob(&datum.d_payload, &payloadLength, payloadLength);
    P(datum.d_payload.length());

    // Append properties and payload to application data
    bdlbb::BlobUtil::append(&appData, datum.d_properties);
    bdlbb::BlobUtil::append(&appData, datum.d_payload);
    P(appData.length());

    // Populate subQueueInfos
    numSubQueueInfos = 2;
    generateSubQueueInfos(&datum.d_subQueueInfos, numSubQueueInfos);

    // Build PUSH event
    result = pushEventBuilder.addSubQueueInfosOption(datum.d_subQueueInfos,
                                                     false);  // packRdaCounter
    BSLS_ASSERT_OPT(result == bmqt::EventBuilderResult::e_SUCCESS);

    result = pushEventBuilder.packMessage(appData,
                                          datum.d_qid,
                                          datum.d_guid,
                                          datum.d_flags,
                                          datum.d_compressionAlgorithmType,
                                          logic);
    BSLS_ASSERT_OPT(result == bmqt::EventBuilderResult::e_SUCCESS);

    bmqp::Event event(pushEventBuilder.blob().get(),
                      bmqtst::TestHelperUtil::allocator());

    // 2) Flatten the event.
    bsl::vector<bmqp::EventUtilEventInfo> eventInfos(
        bmqtst::TestHelperUtil::allocator());
    rc = bmqp::EventUtil::flattenPushEvent(
        &eventInfos,
        event,
        &bufferFactory,
        &blobSpPool,
        bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(eventInfos.size(), 1U);

    // 3) Verify that the flattening results in one event blob containing
    //    two messages with the original message properties and message
    //    payload and corresponding to the respective SubQueueIds.
    bmqp::Event flattenedEvent(&(eventInfos[0].d_blob),
                               bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(flattenedEvent.isPushEvent());

    bmqp::PushMessageIterator msgIterator(&bufferFactory,
                                          bmqtst::TestHelperUtil::allocator());
    flattenedEvent.loadPushMessageIterator(&msgIterator, true);

    for (bmqp::Protocol::SubQueueInfosArray::size_type i = 0;
         i < datum.d_subQueueInfos.size();
         ++i) {
        rc = msgIterator.next();
        BSLS_ASSERT_OPT(rc == 1);
        BSLS_ASSERT_OPT(msgIterator.hasOptions());

        // Verify msgProperties
        bdlbb::Blob properties(&bufferFactory,
                               bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadMessageProperties(&properties);
        P(msgIterator.messagePropertiesSize());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(datum.d_properties,
                                                  properties),
                         0);

        // Verify msgPayload
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadMessagePayload(&payload);
        P(msgIterator.messagePayloadSize());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(datum.d_payload, payload),
                         0);

        // Verify applicationData (msgProperties + msgPayload)
        bdlbb::Blob applicationData(&bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());
        rc = msgIterator.loadApplicationData(&applicationData);
        P(msgIterator.applicationDataSize());
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(bdlbb::BlobUtil::compare(appData, applicationData),
                         0);

        // Verify subQueueInfo
        bmqp::OptionsView optionsView(bmqtst::TestHelperUtil::allocator());
        msgIterator.loadOptionsView(&optionsView);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(optionsView.isValid());
        BSLS_ASSERT_OPT(
            optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS) !=
            optionsView.end());
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(
            bmqtst::TestHelperUtil::allocator());
        rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
        BSLS_ASSERT_OPT(rc == 0);
        BSLS_ASSERT_OPT(subQueueInfos.size() == 1);
        P(subQueueInfos[0].id());
        BMQTST_ASSERT_EQ_D(i, datum.d_subQueueInfos[i], subQueueInfos[0]);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    unsigned int seed = bsl::time(0);
    bsl::srand(seed);
    PV("Seed: " << seed);

    switch (_testCase) {
    case 0:
    case 3: test3_flattenWithMessageProperties(); break;
    case 2: test2_flattenExplodesEvent(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
