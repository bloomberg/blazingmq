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

// bmqimp_event.t.cpp                                                 -*-C++-*-
#include <bmqimp_event.h>

// BMQ
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqp_compression.h>
#include <bmqp_crc32c.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocolutil.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageeventtype.h>
#include <bmqt_messageguid.h>

#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_cstring.h>
#include <bsl_limits.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// CONSTANTS
const int  k_QUEUE_ID           = 1399;
const int  k_FLAGS              = 0;
const int  k_CORR_ID            = 123;
const char k_HEX_REP[]          = "0000000000003039CD8101000000270F";
const int  k_ACK_STATUS         = 3;
const int  k_EVENT_HEADER_SIZE  = sizeof(bmqp::EventHeader);
const int  k_EVENT_HEADER_WORDS = k_EVENT_HEADER_SIZE /
                                 bmqp::Protocol::k_WORD_SIZE;
const int k_ACK_HEADER_WORDS = sizeof(bmqp::AckHeader) /
                               bmqp::Protocol::k_WORD_SIZE;
const int k_PUSH_HEADER_WORDS = sizeof(bmqp::PushHeader) /
                                bmqp::Protocol::k_WORD_SIZE;
const int k_PUT_HEADER_WORDS = sizeof(bmqp::PutHeader) /
                               bmqp::Protocol::k_WORD_SIZE;
const int  k_OPTION_WORDS    = 0;
const char k_APP_DATA[]      = "AppData";
const int  k_APP_DATA_LENGTH = sizeof(k_APP_DATA) - 1;
// For a 0-terminated char string last
// zero byte is included by sizeof and
// therefore should be removed.
const int k_APP_DATA_WORDS = 1 +
                             k_APP_DATA_LENGTH / bmqp::Protocol::k_WORD_SIZE;
// 'k_APP_DATA_WORDS' also includes word
// possibly used for padding.
//: o In case when extra padding word is
//:   needed '+1' is needed too.
//: o In case when extra padding word is
//:   not needed last word with app data
//:   is not completed so division by
//:   'k_WORD_SIZE' will not take into
//:   consideration the last word.  So
//:   '+1' is needed in this case too.

static void dummy_doneCallback(int* input)
{
    *input += 1;
}

/// Append message event of particular `type` to given blob `eb`.  As a
/// result blob will contain general event header, custom message header
/// (PUT,PUSH or ACK) containing message GUID defined by parameter
/// `messageGUID` and message itself.
static void prepareBlobForMessageEvent(bmqp::EventHeader*    eh,
                                       bdlbb::Blob*          eb,
                                       bmqt::MessageGUID*    messageGUID,
                                       bmqp::EventType::Enum type)
{
    BMQTST_ASSERT(eh);
    BMQTST_ASSERT(eb);
    BMQTST_ASSERT(messageGUID);

    bmqt::MessageGUID& guid = *messageGUID;
    (*eh).setType(type).setHeaderWords(k_EVENT_HEADER_WORDS);
    bdlbb::BlobUtil::append(eb,
                            reinterpret_cast<const char*>(eh),
                            sizeof(bmqp::EventHeader));

    int eventLength = k_EVENT_HEADER_SIZE;

    if (type == bmqp::EventType::e_PUT) {
        // Put Header
        bmqp::PutHeader ph;
        ph.setOptionsWords(k_OPTION_WORDS)
            .setHeaderWords(k_PUT_HEADER_WORDS)
            .setQueueId(k_QUEUE_ID)
            .setMessageGUID(guid)
            .setFlags(k_FLAGS)
            .setMessageWords(ph.headerWords() + ph.optionsWords() +
                             k_APP_DATA_WORDS);

        eventLength += ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

        // Write Put Header
        bdlbb::BlobUtil::append(eb,
                                reinterpret_cast<const char*>(&ph),
                                ph.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Write application data
        bdlbb::BlobUtil::append(eb, k_APP_DATA, k_APP_DATA_LENGTH);

        // Write message payload
        bmqp::ProtocolUtil::appendPadding(eb, k_APP_DATA_LENGTH);
    }
    else if (type == bmqp::EventType::e_PUSH) {
        // Push Header
        bmqp::PushHeader ph;
        ph.setOptionsWords(k_OPTION_WORDS)
            .setHeaderWords(k_PUSH_HEADER_WORDS)
            .setQueueId(k_QUEUE_ID)
            .setMessageGUID(guid)
            .setFlags(k_FLAGS)
            .setMessageWords(ph.headerWords() + ph.optionsWords() +
                             k_APP_DATA_WORDS);

        eventLength += ph.messageWords() * bmqp::Protocol::k_WORD_SIZE;

        // Write Push Header
        bdlbb::BlobUtil::append(eb,
                                reinterpret_cast<const char*>(&ph),
                                ph.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        // Write application data
        bdlbb::BlobUtil::append(eb, k_APP_DATA, k_APP_DATA_LENGTH);

        // Write message payload
        bmqp::ProtocolUtil::appendPadding(eb, k_APP_DATA_LENGTH);
    }
    else if (type == bmqp::EventType::e_ACK) {
        // Put Header
        bmqp::AckHeader ah;
        ah.setHeaderWords(k_ACK_HEADER_WORDS);
        ah.setFlags(k_FLAGS);

        eventLength += ah.headerWords() * bmqp::Protocol::k_WORD_SIZE;

        // Write Ack Header
        bdlbb::BlobUtil::append(eb,
                                reinterpret_cast<const char*>(&ah),
                                ah.headerWords() *
                                    bmqp::Protocol::k_WORD_SIZE);

        bmqp::AckMessage ackMessage;
        ackMessage.setStatus(k_ACK_STATUS)
            .setCorrelationId(k_CORR_ID)
            .setMessageGUID(guid)
            .setQueueId(k_QUEUE_ID);

        // Write Ack Message
        bdlbb::BlobUtil::append(eb,
                                reinterpret_cast<const char*>(&ackMessage),
                                sizeof(bmqp::AckMessage));

        eventLength += sizeof(bmqp::AckMessage);
    }

    // Modify event header in blob
    bmqp::EventHeader* e = reinterpret_cast<bmqp::EventHeader*>(
        eb->buffer(0).data());
    (*e).setLength(eventLength).setType(type);

    // Set event length for local event header instance
    (*eh).setLength(eventLength).setType(type);
}

static unsigned int findExpectedCrc32(
    const char*                          messagePayload,
    const int                            messagePayloadLen,
    bmqp::MessageProperties*             msgProps,
    bool                                 hasProperties,
    bdlbb::BlobBufferFactory*            bufferFactory,
    bslma::Allocator*                    allocator,
    bmqt::CompressionAlgorithmType::Enum compressionAlgorithmType)
{
    bdlbb::Blob testApplicationData(bufferFactory,
                                    bmqtst::TestHelperUtil::allocator());

    if (hasProperties) {
        bdlbb::BlobUtil::append(
            &testApplicationData,
            msgProps->streamOut(
                bufferFactory,
                bmqp::MessagePropertiesInfo::makeInvalidSchema()));
        // New format.
    }

    bmqu::MemOutStream error(allocator);
    int                rc = bmqp::Compression::compress(&testApplicationData,
                                         bufferFactory,
                                         compressionAlgorithmType,
                                         messagePayload,
                                         messagePayloadLen,
                                         &error,
                                         allocator);
    BSLS_ASSERT_OPT(rc == 0);
    unsigned int expectedCrc32 = bmqp::Crc32c::calculate(testApplicationData);
    return expectedCrc32;
}
}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_REP);

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);

    PV("Configure as SessionEvent");
    obj.configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                -3,
                                bmqt::CorrelationId(11),
                                "testing");
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(obj.sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);
    BMQTST_ASSERT_EQ(obj.statusCode(), -3);
    BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(11));
    BMQTST_ASSERT_EQ(obj.errorDescription(), "testing");

    PV("Configure as MessageEvent");
    prepareBlobForMessageEvent(&eventHeader,
                               &eventBlob,
                               &guid,
                               bmqp::EventType::e_ACK);
    bmqp::Event event(&eventBlob, bmqtst::TestHelperUtil::allocator());

    // Fails to configure initialized event
    BMQTST_ASSERT_OPT_FAIL(obj.configureAsMessageEvent(event));

    obj.clear();
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);

    obj.configureAsMessageEvent(event);

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_MESSAGE);
    BMQTST_ASSERT(obj.rawEvent().isValid());
    BMQTST_ASSERT(obj.rawEvent().isAckEvent());

    PV("Configure as RawEvent");
    // Fails to configure initialized event
    BMQTST_ASSERT_OPT_FAIL(obj.configureAsRawEvent(event));

    obj.clear();
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);

    // Fails to configure as raw event with not cloned underlying event
    BMQTST_ASSERT_OPT_FAIL(obj.configureAsRawEvent(event));

    obj.configureAsRawEvent(event.clone(bmqtst::TestHelperUtil::allocator()));

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_RAW);

    BMQTST_ASSERT(obj.rawEvent().isValid());
    BMQTST_ASSERT(obj.rawEvent().isAckEvent());
}

static void test2_setterGetterTest()
{
    // ------------------------------------------------------------------------
    // SETTER GETTER TEST
    //
    // Concerns:
    //   1. Check that getter and setters methods of bmqimp::Event class works
    //      as expected.
    //
    // Plan:
    //   For set of getters and setters check that getter return value
    //   established by corresponding setter during the test.
    //
    // Testing accessors:
    //   - type
    //   - doneCallback
    //   - queues
    //   - lookupQueue
    //
    // Testing manipulators:
    //   - setType;
    //   - setDoneCallback
    //   - insertQueue
    //   ----------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("GENERAL SETTER GETTER TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);

    PV("Configure as SessionEvent");
    obj.configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                -3,
                                bmqt::CorrelationId(11),
                                "testing");

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(obj.sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);
    BMQTST_ASSERT_EQ(obj.statusCode(), -3);
    BMQTST_ASSERT_EQ(obj.correlationId(), bmqt::CorrelationId(11));
    BMQTST_ASSERT_EQ(obj.errorDescription(), "testing");

    // setType / type
    bmqimp::Event::EventType::Enum et = bmqimp::Event::EventType::e_MESSAGE;
    obj.setType(et);
    BMQTST_ASSERT_EQ(et, obj.type());

    // setDoneCallback / doneCallback / lookupQueue
    int input = 7;
    obj.setDoneCallback(bdlf::BindUtil::bind(dummy_doneCallback, &input));
    obj.doneCallback()();
    BMQTST_ASSERT_EQ(8, input);

    // insertQueue / queues
    bsl::shared_ptr<bmqimp::Queue> queue =
        bsl::allocate_shared<bmqimp::Queue, bslma::Allocator>(
            bmqtst::TestHelperUtil::allocator());

    const char         k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";
    bmqt::Uri          uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const unsigned int k_SQID         = 2U;
    const unsigned int k_ID           = 12345;
    const int          k_PENDING_ID   = 65432;
    bmqimp::QueueState::Enum  k_STATE = bmqimp::QueueState::e_OPENED;
    const bmqt::CorrelationId k_CORID = bmqt::CorrelationId::autoValue();

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);
    bmqt::QueueFlagsUtil::setAdmin(&flags);

    bmqt::QueueOptions options(bmqtst::TestHelperUtil::allocator());
    options.setMaxUnconfirmedBytes(123);

    (*queue)
        .setUri(uri)
        .setSubQueueId(k_SQID)
        .setState(k_STATE)
        .setId(k_ID)
        .setCorrelationId(k_CORID)
        .setFlags(flags)
        .setAtMostOnce(true)
        .setHasMultipleSubStreams(true)
        .setOptions(options)
        .setPendingConfigureId(k_PENDING_ID);

    obj.insertQueue(queue);

    BMQTST_ASSERT_EQ(1, static_cast<int>(obj.queues().size()));
    bmqimp::Queue* queue2 = obj.queues().begin()->second.get();
    BMQTST_ASSERT_EQ(queue2->uri(), uri);
    BMQTST_ASSERT_EQ(queue2->state(), k_STATE);
    BMQTST_ASSERT_EQ(queue2->subQueueId(), k_SQID);
    BMQTST_ASSERT_EQ(queue2->correlationId(), k_CORID);
    BMQTST_ASSERT_EQ(queue2->flags(), flags);
    BMQTST_ASSERT_EQ(queue2->options(), options);
    BMQTST_ASSERT_EQ(queue2->isValid(), true);
    BMQTST_ASSERT_EQ(queue2->atMostOnce(), true);
    BMQTST_ASSERT_EQ(queue2->id(), static_cast<int>(k_ID));

    PV("Clear event");
    obj.clear();
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);
}

static void test3_sessionEvent_setterGetterTest()
{
    // ------------------------------------------------------------------------
    // SETTER GETTER TEST FOR SESSION EVENT
    //
    // Concerns:
    //   1. Check that getter and setters methods of bmqimp::Event class
    //      specific for session event works as expected.
    //
    // Plan:
    //   For set of getters and setters check that getter return value
    //   established by corresponding setter during the test.
    //
    // Testing accessors:
    //   - statusCode
    //   - sessionEventType
    //   - errorDescription
    //   - correlationId
    //
    // Testing manipulators:
    //   - setSessionEventType
    //   - setErrorDescription
    //   - setCorrelationId
    //   - setStatusCode
    // ------------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("SETTER GETTER TEST FOR SESSION EVENT");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED)
    PV("Configure as SessionEvent");

    bmqt::SessionEventType::Enum sessionType =
        bmqt::SessionEventType::e_TIMEOUT;
    bmqimp::Event::EventType::Enum eventType =
        bmqimp::Event::EventType::e_SESSION;
    int                 statusCode = -4;
    bmqt::CorrelationId correlationId(123);
    bsl::string         errorDescription("Error");

    obj.setType(eventType)
        .setSessionEventType(sessionType)
        .setCorrelationId(correlationId)
        .setErrorDescription(errorDescription)
        .setStatusCode(statusCode);

    BMQTST_ASSERT_EQ(eventType, obj.type());
    BMQTST_ASSERT_EQ(sessionType, obj.sessionEventType());
    BMQTST_ASSERT_EQ(correlationId, obj.correlationId());
    BMQTST_ASSERT_EQ(errorDescription, obj.errorDescription());
    BMQTST_ASSERT_EQ(statusCode, obj.statusCode());
}

static void test4_messageEvent_setterGetterTest()
{
    // ------------------------------------------------------------------------
    // SETTER GETTER TEST FOR MESSAGE EVENT
    //
    // Concerns:
    //   1. Check that getter and setters methods of bmqimp::Event class
    //      specific for message event works as expected.
    //
    // Plan:
    //   For set of getters and setters check that getter return value
    //   established by corresponding setter during the test.
    //
    // Testing accessors:
    //   - correlationId(int)
    //   - numCorrrelationIds
    //
    // Testing manipulators:
    //   - addCorrelationId
    // ------------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("SETTER GETTER TEST FOR MESSAGE EVENT");

    PV("Configure as MesageEvent");
    bmqt::CorrelationId corrId1(123);
    bmqt::CorrelationId corrId2(234);

    // addCorrelationId / correlationId / numCorrrelationIds
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    bmqimp::Event     obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED)

    guid.fromHex(k_HEX_REP);
    prepareBlobForMessageEvent(&eventHeader,
                               &eventBlob,
                               &guid,
                               bmqp::EventType::e_ACK);
    bmqp::Event event(&eventBlob, bmqtst::TestHelperUtil::allocator());
    obj.configureAsMessageEvent(event);
    obj.addContext(corrId1);
    BMQTST_ASSERT_EQ(1, obj.numCorrrelationIds());

    obj.addContext(corrId2);
    BMQTST_ASSERT_EQ(2, obj.numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrId1, obj.context(0).d_correlationId);
    BMQTST_ASSERT_EQ(corrId2, obj.context(1).d_correlationId);

    // setMessageCorrelationIdContainer / messageCorrelationIdContainer
    bmqimp::MessageCorrelationIdContainer container(
        bmqtst::TestHelperUtil::allocator());
    obj.setMessageCorrelationIdContainer(&container);
    bmqimp::MessageCorrelationIdContainer* container_p =
        obj.messageCorrelationIdContainer();
    BMQTST_ASSERT(container_p);
    BMQTST_ASSERT_EQ(&container, container_p);
}

static void test5_configureAsMessageEventTest()
{
    // ------------------------------------------------------------------------
    // CONFIGURE AS MESSAGE EVENT / MESSAGE ITERATOR
    //
    // Concerns:
    //   1. ConfigureAsMessageEvent method should configure event by
    //      setting event type to MESSAGEEVENT and event mode to READ, when
    //      accept correct raw event(bmqp::Event).
    //   2. Configured for reading message event should return correct message
    //      iterators for PUT, PUSH and ACK messages.
    //
    // Plan:
    //   For messages of types PUT, PUSH, ACK do following:
    //     1. Create and init blob containing event header, message header of
    //        considered type and also message or padding.
    //     2. Create raw(bmqp::Event) event over prepared blob.
    //     3. Create bmqimp::Event instance and initialize it by
    //        configureAsMessageEvent method taking raw event as argument.
    //     4. Verify event fields according to event type.
    //     5. Create iterator of corresponding type and iterate over single
    //        message.
    //     6. Verify iterator fields, header and message fields held by
    //       iterator.
    //
    // Testing:
    //   - configureAsMessageEvent(const bmqp::Event& rawEvent)
    //   - putMessageIterator
    //   - pushMessageIterator
    //   - ackMessageIterator
    //   ----------------------------------------------------------------------

    // Type aliases
    typedef bmqimp::Event::MessageEventMode::Enum MessageEventMode;
    typedef bmqimp::Event::EventType::Enum        EventType;

    // Variables
    bmqt::MessageGUID              guid;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    guid.fromHex(k_HEX_REP);

    // Test data
    struct Test {
        int                   d_line;
        bmqp::EventType::Enum d_type;
        MessageEventMode      d_messageEventMode;
        EventType             d_eventType;
        bool                  d_isValid;
        bool                  d_isControlEvent;
        bool                  d_isPutEvent;
        bool                  d_isConfirmEvent;
        bool                  d_isPushEvent;
        bool                  d_isAckEvent;
        bool                  d_isElectorEvent;
        bool                  d_isStorageEvent;
        bool                  d_isRecoveryEvent;
        bool                  d_isPartitionSyncEvent;
        bool                  d_isHeartbeatReqEvent;
        bool                  d_isHeartbeatRspEvent;
    } k_DATA[]              = {{L_,
                                bmqp::EventType::e_PUT,
                                bmqimp::Event::MessageEventMode::e_READ,
                                bmqimp::Event::EventType::e_MESSAGE,
                                true,
                                false,
                                true,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false},
                               {L_,
                                bmqp::EventType::e_PUSH,
                                bmqimp::Event::MessageEventMode::e_READ,
                                bmqimp::Event::EventType::e_MESSAGE,
                                true,
                                false,
                                false,
                                false,
                                true,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false},
                               {L_,
                                bmqp::EventType::e_ACK,
                                bmqimp::Event::MessageEventMode::e_READ,
                                bmqimp::Event::EventType::e_MESSAGE,
                                true,
                                false,
                                false,
                                false,
                                false,
                                true,
                                false,
                                false,
                                false,
                                false,
                                false,
                                false}};
    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    bmqtst::TestHelper::printTestName(
        "CONFIGURE AS MESSAGE EVENT / MESSAGE ITERATOR TEST");

    // Iterate over test data grouped by message event types
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];
        PVV("Testing " << bmqp::EventType::toAscii(test.d_type)
                       << " message event.");

        // 1. Create and init blob containing event header, message header of
        // considered type and also message or padding.
        bdlbb::Blob       eventBlob(&bufferFactory,
                              bmqtst::TestHelperUtil::allocator());
        bmqp::EventHeader eventHeader;
        prepareBlobForMessageEvent(&eventHeader,
                                   &eventBlob,
                                   &guid,
                                   test.d_type);

        // 2. Create raw(bmqp::Event) event over prepared blob
        bmqp::Event event(&eventBlob, bmqtst::TestHelperUtil::allocator());

        // 3. Create bmqimp::Event instance and initialize it by
        //    configureAsMessageEvent method taking raw event as argument.
        bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        obj.configureAsMessageEvent(event);

        // 4. Verify event fields according to event type.
        BMQTST_ASSERT_EQ(test.d_messageEventMode, obj.messageEventMode());
        BMQTST_ASSERT_EQ(test.d_eventType, obj.type());
        BMQTST_ASSERT_EQ(test.d_isPutEvent, obj.rawEvent().isPutEvent());
        BMQTST_ASSERT_EQ(test.d_isPushEvent, obj.rawEvent().isPushEvent());
        BMQTST_ASSERT_EQ(test.d_isValid, obj.rawEvent().isValid());
        BMQTST_ASSERT_EQ(test.d_isControlEvent,
                         obj.rawEvent().isControlEvent());
        BMQTST_ASSERT_EQ(test.d_isPutEvent, obj.rawEvent().isPutEvent());
        BMQTST_ASSERT_EQ(test.d_isConfirmEvent,
                         obj.rawEvent().isConfirmEvent());
        BMQTST_ASSERT_EQ(test.d_isPushEvent, obj.rawEvent().isPushEvent());
        BMQTST_ASSERT_EQ(test.d_isAckEvent, obj.rawEvent().isAckEvent());
        BMQTST_ASSERT_EQ(test.d_isElectorEvent,
                         obj.rawEvent().isElectorEvent());
        BMQTST_ASSERT_EQ(test.d_isStorageEvent,
                         obj.rawEvent().isStorageEvent());
        BMQTST_ASSERT_EQ(test.d_isRecoveryEvent,
                         obj.rawEvent().isRecoveryEvent());
        BMQTST_ASSERT_EQ(test.d_isPartitionSyncEvent,
                         obj.rawEvent().isPartitionSyncEvent());
        BMQTST_ASSERT_EQ(test.d_isHeartbeatReqEvent,
                         obj.rawEvent().isHeartbeatReqEvent());
        BMQTST_ASSERT_EQ(test.d_isHeartbeatRspEvent,
                         obj.rawEvent().isHeartbeatRspEvent());

        // 5. Create iterator of corresponding type and iterate over single
        //    message.
        // 6. Verify iterator fields, header and message fields held by
        //    iterator.
        if (test.d_isPutEvent) {
            bmqp::PutMessageIterator* iter = obj.putMessageIterator();
            iter->reset(&eventBlob, eventHeader, true);
            BMQTST_ASSERT_EQ(iter->isValid(), true);
            BMQTST_ASSERT_EQ(1, iter->next())
            BMQTST_ASSERT_EQ(iter->header().optionsWords(), 0);
            BMQTST_ASSERT_EQ(iter->header().messageWords(),
                             k_PUT_HEADER_WORDS + k_APP_DATA_WORDS);
            BMQTST_ASSERT_EQ(iter->header().queueId(), k_QUEUE_ID);
            BMQTST_ASSERT_EQ(iter->header().messageGUID(), guid);
            BMQTST_ASSERT_EQ(iter->header().flags(), k_FLAGS);
            BMQTST_ASSERT_EQ(0, iter->next())
        }
        else if (test.d_isPushEvent) {
            bmqp::PushMessageIterator* iter = obj.pushMessageIterator();
            iter->reset(&eventBlob, eventHeader, true);
            BMQTST_ASSERT_EQ(iter->isValid(), true);
            BMQTST_ASSERT_EQ(1, iter->next())
            BMQTST_ASSERT_EQ(iter->header().optionsWords(), 0);
            BMQTST_ASSERT_EQ(iter->header().messageWords(),
                             k_PUSH_HEADER_WORDS + k_APP_DATA_WORDS);
            BMQTST_ASSERT_EQ(iter->header().queueId(), k_QUEUE_ID);
            BMQTST_ASSERT_EQ(iter->header().messageGUID(), guid);
            BMQTST_ASSERT_EQ(iter->header().flags(), k_FLAGS);
            BMQTST_ASSERT_EQ(0, iter->next())
        }
        else if (test.d_isAckEvent) {
            bmqp::AckMessageIterator* iter = obj.ackMessageIterator();
            BMQTST_ASSERT_EQ(iter->isValid(), true);
            BMQTST_ASSERT_EQ(1, iter->next());
            BMQTST_ASSERT_EQ(iter->header().headerWords(), k_ACK_HEADER_WORDS);
            BMQTST_ASSERT_EQ(iter->header().flags(), k_FLAGS);
            BMQTST_ASSERT_EQ(iter->message().correlationId(), k_CORR_ID);
            BMQTST_ASSERT_EQ(iter->message().queueId(), k_QUEUE_ID);
            BMQTST_ASSERT_EQ(iter->message().messageGUID(), guid);
            BMQTST_ASSERT_EQ(iter->message().status(), k_ACK_STATUS);
            BMQTST_ASSERT_EQ(0, iter->next())
        }
    }
}

static void test6_comparisonOperatorTest()
{
    // ------------------------------------------------------------------------
    // COMPARISON OPERATORS
    //
    // Concerns:
    //   1. Comparison operators == and != should work as expected.
    //
    // Plan:
    //   1.  Create two objects of bmqimp::Event.
    //   2.  Configure both objects as session event with the same session
    //       event type correlation id and error description.
    //   3.  Check if 'operator==' between two bmqimp::Event objects return
    //       'true'.
    //   4.  Change 'errorDescription' of one of the objects.
    //   5.  Check if 'operator!=' between two bmqimp::Event objects return
    //       'true'.
    //   6.  Change 'type' of one of the objects to
    //       'bmqimp::Event::EventType::e_UNINITIALIZED'.
    //   7.  Check if 'operator!=' between two bmqimp::Event objects return
    //       'true'.
    //   8.  Change 'type' of the second object to
    //       'bmqimp::Event::EventType::e_UNINITIALIZED'.
    //   9.  Check if 'operator==' between two bmqimp::Event objects return
    //       'true'.
    //   10. Create two objects of bmqimp::Event.
    //   11. Configure both objects as message event.
    //   12. Check if 'operator!=' between two bmqimp::Event objects return
    //       'true'.
    //   13. Create two objects of bmqimp::Event.
    //   14. Configure both objects as raw event.
    //   15. Check if 'operator!=' between two bmqimp::Event objects return
    //       'true'.
    //
    // Testing:
    //   - operator==
    //   - operator!=
    //   ----------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("COMPARISON OPERATORS");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqimp::Event obj1(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj2(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqt::SessionEventType::Enum sessionType =
        bmqt::SessionEventType::e_TIMEOUT;
    bmqimp::Event::EventType::Enum eventType =
        bmqimp::Event::EventType::e_SESSION;
    int                 statusCode = -3;
    bmqt::CorrelationId correlationId(123);
    bsl::string         errorDescription("testing");
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_REP);

    PV("Configure as SessionEvent");
    obj1.configureAsSessionEvent(sessionType,
                                 statusCode,
                                 correlationId,
                                 errorDescription);

    obj2.setType(eventType)
        .setSessionEventType(sessionType)
        .setCorrelationId(correlationId)
        .setErrorDescription(errorDescription)
        .setStatusCode(statusCode);

    BMQTST_ASSERT(obj1 == obj2);

    obj2.setErrorDescription("Different description");
    BMQTST_ASSERT(obj1 != obj2);

    obj1.setType(bmqimp::Event::EventType::e_UNINITIALIZED);
    BMQTST_ASSERT(obj1 != obj2);

    obj2.setType(bmqimp::Event::EventType::e_UNINITIALIZED);
    BMQTST_ASSERT(obj1 == obj2);

    PV("Configure as MesageEvent");
    bmqimp::Event obj3(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj4(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj3.configureAsMessageEvent(blobSpPool.get());
    obj4.configureAsMessageEvent(blobSpPool.get());

    // NOTE: Message event can not be equal. Is it expected?
    BMQTST_ASSERT(obj3 != obj4);

    PV("Configure as RawEvent");
    prepareBlobForMessageEvent(&eventHeader,
                               &eventBlob,
                               &guid,
                               bmqp::EventType::e_ACK);
    bmqp::Event   event(&eventBlob, bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj5(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj6(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj5.configureAsRawEvent(event.clone(bmqtst::TestHelperUtil::allocator()));
    obj6.configureAsRawEvent(event.clone(bmqtst::TestHelperUtil::allocator()));

    // NOTE: Raw event can not be equal
    BMQTST_ASSERT(obj5 != obj6);
}

static void test7_printing()
// --------------------------------------------------------------------
// PRINT
//
// Concerns:
//   'bmqimp::Event' print method works as expected.
//
// Plan:
//   1. For every valid 'bmqimp::Event' do the following:
//     1.1. Create a 'bmqimp::Event' object, configure by test data.
//     1.2. Check that the output of the 'print' method and '<<' operator
//          matches the expected value.
//   2. Check that 'bmqimp::Event' object directed to output stream with
//      error doesn't make impact on stream (output is empty).
//   3. Check that 'bmqimp::Event' object with incorrect event type
//      directed to output stream causes 'bsls::AssertTestException'
//      exception.
//
// Testing:
//   bmqimp::Event::print()
//
//   bsl::ostream&
//   bmqimp::operator<<(bsl::ostream& stream,
//                      const bmqp::Event& rhs)
// --------------------------------------------------------------------
{
    // bmqu::PrintUtil::prettyTimeInterval uses default allocator
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("PRINT");
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_REP);

    struct Test {
        bmqimp::Event::EventType::Enum        d_eventType;
        bsl::string                           d_age;
        bmqt::SessionEventType::Enum          d_sessionEventType;
        int                                   d_statusCode;
        bmqt::CorrelationId                   d_correlationId;
        bsl::string                           d_errorDescription;
        bmqimp::Event::MessageEventMode::Enum d_messageEventMode;
        const char*                           d_uri;
        bmqp::EventType::Enum                 d_rawEventType;
    } k_DATA[] = {{bmqimp::Event::EventType::e_SESSION,
                   "NULL",
                   bmqt::SessionEventType::e_TIMEOUT,
                   -3,
                   bmqt::CorrelationId(11),
                   "testing",
                   bmqimp::Event::MessageEventMode::e_UNINITIALIZED,
                   "bmq://ts.trades.myapp/my.queue?id=my.app",
                   bmqp::EventType::e_UNDEFINED},
                  {bmqimp::Event::EventType::e_MESSAGE,
                   "NULL",
                   bmqt::SessionEventType::e_UNDEFINED,
                   -3,
                   bmqt::CorrelationId(11),
                   "testing",
                   bmqimp::Event::MessageEventMode::e_READ,
                   "bmq://ts.trades.myapp/my.queue?id=my.app",
                   bmqp::EventType::e_PUT},
                  {bmqimp::Event::EventType::e_MESSAGE,
                   "NULL",
                   bmqt::SessionEventType::e_UNDEFINED,
                   -3,
                   bmqt::CorrelationId(11),
                   "testing",
                   bmqimp::Event::MessageEventMode::e_UNINITIALIZED,
                   "bmq://ts.trades.myapp/my.queue?id=my.app",
                   bmqp::EventType::e_PUT},
                  {bmqimp::Event::EventType::e_UNINITIALIZED,
                   "NULL",
                   bmqt::SessionEventType::e_UNDEFINED,
                   -3,
                   bmqt::CorrelationId(11),
                   "testing",
                   bmqimp::Event::MessageEventMode::e_UNINITIALIZED,
                   "bmq://ts.trades.myapp/my.queue?id=my.app",
                   bmqp::EventType::e_UNDEFINED},
                  {
                      bmqimp::Event::EventType::e_RAW,
                      "NULL",
                      bmqt::SessionEventType::e_UNDEFINED,
                      -1,
                      bmqt::CorrelationId(11),
                      "testing",
                      bmqimp::Event::MessageEventMode::e_READ,
                      "bmq://ts.trades.myapp/my.queue?id=my.app",
                      bmqp::EventType::e_PUSH,
                  }};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    // Iterate over test data.
    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        Test&         data = k_DATA[idx];
        bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

        // Prepare bmqimp::event object based on data.
        switch (data.d_eventType) {
        case bmqimp::Event::EventType::e_SESSION: {
            obj.setType(data.d_eventType)
                .setSessionEventType(data.d_sessionEventType)
                .setStatusCode(data.d_statusCode)
                .setCorrelationId(data.d_correlationId)
                .setErrorDescription(data.d_errorDescription);

            bmqt::Uri uri(data.d_uri, bmqtst::TestHelperUtil::allocator());
            bsl::shared_ptr<bmqimp::Queue> queue =
                bsl::allocate_shared<bmqimp::Queue, bslma::Allocator>(
                    bmqtst::TestHelperUtil::allocator());
            queue->setUri(uri);
            obj.insertQueue(queue);
        } break;
        case bmqimp::Event::EventType::e_MESSAGE: {
            if (data.d_messageEventMode ==
                bmqimp::Event::MessageEventMode::e_UNINITIALIZED) {
                obj.setType(bmqimp::Event::EventType::e_MESSAGE);
            }
            else {
                eventBlob.removeAll();
                prepareBlobForMessageEvent(&eventHeader,
                                           &eventBlob,
                                           &guid,
                                           bmqp::EventType::e_PUT);
                bmqp::Event rawEvent(&eventBlob,
                                     bmqtst::TestHelperUtil::allocator());
                obj.configureAsMessageEvent(rawEvent);
            }

            bmqt::Uri uri(data.d_uri, bmqtst::TestHelperUtil::allocator());
            bsl::shared_ptr<bmqimp::Queue> queue =
                bsl::allocate_shared<bmqimp::Queue, bslma::Allocator>(
                    bmqtst::TestHelperUtil::allocator());
            queue->setUri(uri);
            obj.insertQueue(queue);
        } break;
        case bmqimp::Event::EventType::e_RAW: {
            eventBlob.removeAll();
            prepareBlobForMessageEvent(&eventHeader,
                                       &eventBlob,
                                       &guid,
                                       data.d_rawEventType);
            bmqp::Event rawEvent(&eventBlob,
                                 bmqtst::TestHelperUtil::allocator());
            obj.configureAsRawEvent(
                rawEvent.clone(bmqtst::TestHelperUtil::allocator()));
            break;
        }
        case bmqimp::Event::EventType::e_REQUEST:
        case bmqimp::Event::EventType::e_UNINITIALIZED: {
            break;
        }
        default: {
            BMQTST_ASSERT(false && "Unknown bmqimp::Event::EventType");
            break;
        }
        }

        // Prepare output stream based on data.
        expected << "[";
        switch (data.d_eventType) {
        case bmqimp::Event::EventType::e_UNINITIALIZED: {
            expected << " type = \"UNINITIALIZED\"";
        } break;
        case bmqimp::Event::EventType::e_SESSION: {
            expected << " type = \"SESSION\""
                     << " sessionEventType = " << data.d_sessionEventType
                     << " statusCode = " << data.d_statusCode
                     << " correlationId = " << data.d_correlationId;
            if (!data.d_errorDescription.empty()) {
                expected << " errorDescription = \"" << data.d_errorDescription
                         << "\"";
            }
            if (!bsl::string(data.d_uri, bmqtst::TestHelperUtil::allocator())
                     .empty()) {
                expected << " queue = " << data.d_uri;
            }
        } break;
        case bmqimp::Event::EventType::e_MESSAGE: {
            expected << " type = \"MESSAGE\"";
            if (data.d_messageEventMode ==
                bmqimp::Event::MessageEventMode::e_UNINITIALIZED) {
                expected << " msgEventMode = \"UNINITIALIZED\"";
            }
            else if (data.d_messageEventMode ==
                     bmqimp::Event::MessageEventMode::e_READ) {
                expected << " rawEventType = " << data.d_rawEventType;
            }
            if (!bsl::string(data.d_uri, bmqtst::TestHelperUtil::allocator())
                     .empty()) {
                expected << " queue = " << data.d_uri;
            }
        } break;
        case bmqimp::Event::EventType::e_RAW: {
            expected << " type = \"RAW\""
                     << " rawEventType = " << data.d_rawEventType;
        } break;
        case bmqimp::Event::EventType::e_REQUEST:
        default: {
            BMQTST_ASSERT(false && "Unknown Event type");
        }
        }
        expected << " ]";

        // Print object to 'out' stream.
        out << obj;

        // Compare expected and actual output stream.
        BMQTST_ASSERT_EQ(expected.str(), out.str());

        // Reset streams before next iteration
        expected.reset();
        out.reset();
    }

    {
        PV("Bad stream test");
        bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        obj.setType(bmqimp::Event::EventType::e_SESSION);
        out << "NO LAYOUT";
        out.clear(bsl::ios_base::badbit);
        out << obj;
        BMQTST_ASSERT_EQ(out.str(), "NO LAYOUT");
        out.clear();
        out.reset();
    }

    {
        PV("Bad enum value test");
        bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
        obj.setType(static_cast<bmqimp::Event::EventType::Enum>(
            bsl::numeric_limits<int>::min()));

        BMQTST_ASSERT_OPT_FAIL(out << obj);
        out.reset();
    }
}
static void test8_putEventBuilder()
{
    // ------------------------------------------------------------------------
    // PUT EVENT BUILDER TEST
    //
    // Concerns:
    //   Exercise the basic functionality of the component.
    //
    // Plan:
    //   - Create 'bmqimp::event'
    //   - Configure as message event in write mode
    //   - Get 'bmqp::PutEventBuilder' and test basic functionality
    //
    // Testing:
    //   - putEventBuilder
    // ------------------------------------------------------------------------
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // 'putIter.loadMessageProperties' allocates into a temporary blob
    // using the default allocator

    bmqtst::TestHelper::printTestName("PUT EVENT BUILDER TEST");

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
    const char*              k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int                k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);
    const char*              k_HEX_GUID1 = "40000000000000000000000000000001";
    const char*              k_HEX_GUID2 = "40000000000000000000000000000002";
    const char*              k_HEX_GUID3 = "40000000000000000000000000000003";

    bmqp::MessageProperties msgProps(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0,
                     msgProps.setPropertyAsInt32("encoding",
                                                 k_PROPERTY_VAL_ENCODING));
    BMQTST_ASSERT_EQ(0, msgProps.setPropertyAsString("id", k_PROPERTY_VAL_ID));
    BMQTST_ASSERT_EQ(0,
                     msgProps.setPropertyAsInt64("timestamp", k_TIME_STAMP));

    BMQTST_ASSERT_EQ(k_NUM_PROPERTIES, msgProps.numProperties());

    // Create PutEventBuilder
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj.configureAsMessageEvent(blobSpPool.get());
    bmqp::PutEventBuilder& builder = *(obj.putEventBuilder());

    builder.startMessage();
    builder.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setMessageProperties(&msgProps);
#ifdef BMQ_ENABLE_MSG_GROUPID
    builder.setMsgGroupId(k_MSG_GROUP_ID);
#endif

    struct Test {
        int                d_line;
        int                d_queueId;
        const char*        d_guidHex;
        bsls::Types::Int64 d_timeStamp;
        bool               d_hasProperties;
        bool               d_hasNewTimeStamp;
    } k_DATA[] = {{L_, 9876, k_HEX_GUID1, k_TIME_STAMP, true, false},
                  {L_, 5432, k_HEX_GUID2, 9876543210LL, true, true},
                  {L_, 3333, k_HEX_GUID3, 0LL, false, false}};

    // Pack messages
    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);
    unsigned int expectedCrc32[k_NUM_DATA];

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test   = k_DATA[idx];
        const int   msgNum = idx + 1;

        builder.setCrc32c(k_CRC32).setMessageGUID(
            bmqp::MessageGUIDGenerator::testGUID());

        if (test.d_hasNewTimeStamp) {
            BMQTST_ASSERT_EQ(0,
                             msgProps.setPropertyAsInt64("timestamp",
                                                         test.d_timeStamp));
        }

        if (!test.d_hasProperties) {
            builder.clearMessageProperties();
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
        BMQTST_ASSERT_EQ(builder.msgGroupId().isNull(), false);
        BMQTST_ASSERT_EQ(builder.msgGroupId().value(), k_MSG_GROUP_ID);
#endif

        BMQTST_ASSERT_EQ(builder.unpackedMessageSize(), k_PAYLOAD_LEN);

        bmqt::EventBuilderResult::Enum rc = builder.packMessage(
            test.d_queueId);

        BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        BMQTST_ASSERT_EQ(builder.messageCount(), msgNum);
        BMQTST_ASSERT_EQ(builder.unpackedMessageSize(), k_PAYLOAD_LEN);
        BMQTST_ASSERT_EQ(builder.messageGUID(), bmqt::MessageGUID());

        BMQTST_ASSERT_GT(builder.eventSize(),
                         k_PAYLOAD_LEN * msgNum + msgProps.totalSize());
    }

    // Get blob and use bmqp iterator to test.  Note that bmqp event and
    // bmqp iterators are lower than bmqp builders, and thus, can be used
    // to test them.
    bmqp::Event rawEvent(builder.blob().get(),
                         bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(rawEvent.isValid());
    BMQTST_ASSERT(rawEvent.isPutEvent());

    bmqp::PutMessageIterator putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT(putIter.isValid());
    bdlbb::Blob payloadBlob(bmqtst::TestHelperUtil::allocator());

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&       test = k_DATA[idx];
        bmqt::MessageGUID guid;
        guid.fromHex(test.d_guidHex);

        BMQTST_ASSERT_EQ(1, putIter.next());
        BMQTST_ASSERT_EQ(test.d_queueId, putIter.header().queueId());
        BMQTST_ASSERT_EQ(guid, putIter.header().messageGUID());
        BMQTST_ASSERT_EQ(expectedCrc32[idx], putIter.header().crc32c());

        payloadBlob.removeAll();

        BMQTST_ASSERT(putIter.loadMessagePayload(&payloadBlob) == 0);
        BMQTST_ASSERT(putIter.messagePayloadSize() == k_PAYLOAD_LEN);

        int res, compareResult;
        res = bmqu::BlobUtil::compareSection(&compareResult,
                                             payloadBlob,
                                             bmqu::BlobPosition(),
                                             k_PAYLOAD,
                                             k_PAYLOAD_LEN);

        BMQTST_ASSERT_EQ(res, 0);
        BMQTST_ASSERT_EQ(compareResult, 0);

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
}

static void test9_copyTest()
{
    // ------------------------------------------------------------------------
    // COPY CONSTRUCTOR TEST
    //
    // Concerns:
    //   Exercise the behavior of copy constructor
    //
    // Plan:
    //   - Create 'bmqimp::event' instance
    //   - Configure as message event in write mode
    //   - Copy instance of 'bmqimp::event'
    //   - Check that expected subset of getters of copied instance return
    //     the same value as the original instance.
    //   - Repeat steps for cloned event.
    //   - Configure the instance as session event.
    //   - Copy instance of 'bmqimp::event'
    //   - Check that the copied instance equals to the original value.
    //   - Configure the instance as raw event.
    //   - Copy instance of 'bmqimp::event'
    //   - Check that event type getters of copied instance return
    //     the same value as the original instance.
    //
    // Testing:
    //   - Event::Event(const Event& other, bslma::Allocator *allocator)
    // ------------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("COPY CONSTRUCTOR TEST");

    PV("Configure as MesageEvent");
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_REP);

    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED)

    prepareBlobForMessageEvent(&eventHeader,
                               &eventBlob,
                               &guid,
                               bmqp::EventType::e_ACK);
    bmqp::Event event(&eventBlob, bmqtst::TestHelperUtil::allocator());
    obj.configureAsMessageEvent(event);

    bmqimp::Event obj2(obj, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(obj2.type(), bmqimp::Event::EventType::e_MESSAGE);
    BMQTST_ASSERT_EQ(obj2.rawEvent().type(), bmqp::EventType::e_ACK);
    BMQTST_ASSERT_EQ(obj2.rawEvent().isAckEvent(), true);
    BMQTST_ASSERT_EQ(obj2.messageEventMode(), obj.messageEventMode());
    BMQTST_ASSERT_EQ(obj2.queues(), obj.queues());
    BMQTST_ASSERT_EQ(obj2.messageCorrelationIdContainer(),
                     obj.messageCorrelationIdContainer());

    // The same steps for cloned event
    bmqimp::Event obj3(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj3.configureAsMessageEvent(
        event.clone(bmqtst::TestHelperUtil::allocator()));
    bmqimp::Event obj4(obj3, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(obj4.rawEvent().type(), bmqp::EventType::e_ACK);
    BMQTST_ASSERT_EQ(obj4.rawEvent().isAckEvent(), true);
    BMQTST_ASSERT_EQ(obj4.messageEventMode(), obj.messageEventMode());
    BMQTST_ASSERT_EQ(obj4.queues(), obj.queues());
    BMQTST_ASSERT_EQ(obj4.messageCorrelationIdContainer(),
                     obj.messageCorrelationIdContainer());

    PV("Configure as SessionEvent");
    obj.reset();
    obj.configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                -3,
                                bmqt::CorrelationId(2),
                                "testing");

    bmqimp::Event obj5(obj, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(obj5 == obj);

    PV("Configure as RawEvent");
    obj.reset();
    obj.configureAsRawEvent(event.clone(bmqtst::TestHelperUtil::allocator()));

    bmqimp::Event obj6(obj, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(obj6.type(), bmqimp::Event::EventType::e_RAW);
    BMQTST_ASSERT_EQ(obj6.rawEvent().type(), obj.rawEvent().type());
}

static void test10_assignmentTest()
{
    // ------------------------------------------------------------------------
    // ASSIGNMENT OPERATOR TEST
    //
    // Concerns:
    //   Exercise the behavior of assignment operator
    //
    // Plan:
    //   - Create 'bmqimp::event' instance
    //   - Configure as message event in write mode
    //   - Create one more 'bmqimp::event' instance
    //   - Assign the first instance of 'bmqimp::event' to the second one.
    //   - Check that expected subset of getters of the second instance return
    //     the same value as the original instance.
    //   - Configure the 'bmqimp::event' instance as session event
    //   - Create one more 'bmqimp::event' instance and assign it
    //     to the configured instance.
    //   - Check that these two instances are equal.
    //   - Configure the 'bmqimp::event' instance as raw event
    //   - Create one more 'bmqimp::event' instance and assign it
    //     to the configured instance.
    //   - Check that event type getters of these instances return
    //     the same value.
    //
    // Testing:
    //   - Event& Event::operator=(const Event& rhs)
    // ------------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("ASSIGNMENT OPERATOR TEST");

    PV("ASSIGNMENT OPERATOR - Configure as MesageEvent");
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob eventBlob(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::EventHeader eventHeader;
    bmqt::MessageGUID guid;
    guid.fromHex(k_HEX_REP);

    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED)

    prepareBlobForMessageEvent(&eventHeader,
                               &eventBlob,
                               &guid,
                               bmqp::EventType::e_ACK);
    bmqp::Event event(&eventBlob, bmqtst::TestHelperUtil::allocator());
    obj.configureAsMessageEvent(event);

    PV("ASSIGNMENT OPERATOR - Assign to the empty event");
    bmqimp::Event obj2(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj2 = obj;
    BMQTST_ASSERT_EQ(obj2.type(), bmqimp::Event::EventType::e_MESSAGE);
    BMQTST_ASSERT_EQ(obj2.rawEvent().type(), bmqp::EventType::e_ACK);
    BMQTST_ASSERT_EQ(obj2.rawEvent().isAckEvent(), true);
    BMQTST_ASSERT_EQ(obj2.messageEventMode(), obj.messageEventMode());
    BMQTST_ASSERT_EQ(obj2.queues(), obj.queues());
    BMQTST_ASSERT_EQ(obj2.messageCorrelationIdContainer(),
                     obj.messageCorrelationIdContainer());

    PV("ASSIGNMENT OPERATOR - Assign cloned event to the empty event");
    bmqimp::Event obj3(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj3.configureAsMessageEvent(
        event.clone(bmqtst::TestHelperUtil::allocator()));
    bmqimp::Event obj4(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj4 = obj3;
    BMQTST_ASSERT_EQ(obj4.type(), bmqimp::Event::EventType::e_MESSAGE);
    BMQTST_ASSERT_EQ(obj4.rawEvent().type(), bmqp::EventType::e_ACK);
    BMQTST_ASSERT_EQ(obj4.rawEvent().isAckEvent(), true);
    BMQTST_ASSERT_EQ(obj4.messageEventMode(), obj.messageEventMode());
    BMQTST_ASSERT_EQ(obj4.queues(), obj.queues());
    BMQTST_ASSERT_EQ(obj4.messageCorrelationIdContainer(),
                     obj.messageCorrelationIdContainer());

    PV("ASSIGNMENT OPERATOR - Self assignment");
    BMQTST_ASSERT_EQ(&(obj2.operator=(obj2)), &obj2);

    PV("ASSIGNMENT OPERATOR - Configure as SessionEvent");
    obj.reset();
    obj.configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                -3,
                                bmqt::CorrelationId(5),
                                "testingAssignment");

    bmqimp::Event obj5(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    obj5 = obj;

    BMQTST_ASSERT(obj5 == obj);

    PV("ASSIGNMENT OPERATOR - Configure as RawEvent");
    obj.reset();
    obj.configureAsRawEvent(event.clone(bmqtst::TestHelperUtil::allocator()));

    bmqimp::Event obj6(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    obj6 = obj;

    BMQTST_ASSERT_EQ(obj6.type(), bmqimp::Event::EventType::e_RAW);
    BMQTST_ASSERT_EQ(obj6.rawEvent().type(), obj.rawEvent().type());
}

static void test11_doneCallbackTest()
{
    // ------------------------------------------------------------------------
    // DONE CALLBACK TEST
    //
    // Concerns:
    //   1. Check that 'd_doneCallback' executes during call of 'clear' method
    //      applicable to session event.
    //   2. Check that 'clear' called multiple times only executes the
    //      'd_doneCallback' once
    //
    // Plan:
    //   1. Create session event
    //   2. Set up callback
    //   3. Clear event
    //   4. Check if callback was executed
    //   5. Clear the event again and ensure the callback did NOT get
    //      re-executed
    //
    // Testing manipulators:
    //   - setDoneCallback
    //   - clear
    //   ----------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("DONE CALLBACK TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.type(), bmqimp::Event::EventType::e_UNINITIALIZED);

    PV("Configure as SessionEvent");
    obj.configureAsSessionEvent(bmqt::SessionEventType::e_TIMEOUT,
                                -3,
                                bmqt::CorrelationId(11),
                                "testing");

    // setDoneCallback / clear
    int input = 7;
    obj.setDoneCallback(bdlf::BindUtil::bind(dummy_doneCallback, &input));
    obj.clear();
    BMQTST_ASSERT_EQ(8, input);

    obj.clear();
    BMQTST_ASSERT_EQ(8, input);
}

static void test12_upgradeDowngradeMessageEvent()
{
    // ------------------------------------------------------------------------
    // UPGRADE DOWNGRADE MESSAGE EVENT TEST
    //
    // Concerns:
    //   1. Check that 'upgradeMessageEventModeToWrite' set 'bmqimp::event'
    //      object to WRITE 'MessageEventMode'.
    //   2. Check that 'downgradeMessageEventModeToRead' set 'bmqimp::event'
    //      object to READ 'MessageEventMode'.
    //
    // Plan:
    //   1.  Create session event
    //   2.  Configure event as message event in WRITE mode.
    //   3.  Downgrade to READ mode via 'downgradeMessageEventModeToRead'.
    //   4.  Check event 'MessageEventMode' is READ.
    //   5.  Call 'downgradeMessageEventModeToRead' again.
    //   6.  Check MessageEventMode' is the same as after last call of
    //       'upgradeMessageEventModeToWrite'.
    //   7.  Upgrade to WRITE mode via 'upgradeMessageEventModeToWrite'.
    //   8.  Check event 'MessageEventMode' is WRITE.
    //   9.  Call 'upgradeMessageEventModeToWrite' again.
    //   10. Check MessageEventMode' is the same as after last call of
    //       'upgradeMessageEventModeToWrite'.
    //
    // Testing manipulators:
    //   - downgradeMessageEventModeToRead
    //   - upgradeMessageEventModeToWrite
    //   ----------------------------------------------------------------------
    bmqtst::TestHelper::printTestName("UPGRADE DOWNGRADE MESSAGE EVENT TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));

    // 1. Create session event
    bmqimp::Event obj(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    // 2. Configure event as message event in WRITE mode.
    obj.configureAsMessageEvent(blobSpPool.get());
    BMQTST_ASSERT_EQ(bmqimp::Event::MessageEventMode::e_WRITE,
                     obj.messageEventMode());

    // 3. Downgrade to READ mode via 'downgradeMessageEventModeToRead'.
    obj.downgradeMessageEventModeToRead();

    // 4. Check event 'MessageEventMode' is READ.
    BMQTST_ASSERT_EQ(bmqimp::Event::MessageEventMode::e_READ,
                     obj.messageEventMode());

    // 5. Call 'downgradeMessageEventModeToRead' again.
    obj.downgradeMessageEventModeToRead();

    // 6. Check MessageEventMode' is the same as after last call of
    //    'upgradeMessageEventModeToWrite'.
    BMQTST_ASSERT_EQ(bmqimp::Event::MessageEventMode::e_READ,
                     obj.messageEventMode());

    // 7. Upgrade to WRITE mode via 'upgradeMessageEventModeToWrite'.
    obj.upgradeMessageEventModeToWrite();

    // 8. Check event 'MessageEventMode' is WRITE.
    BMQTST_ASSERT_EQ(bmqimp::Event::MessageEventMode::e_WRITE,
                     obj.messageEventMode());

    // 9. Call 'upgradeMessageEventModeToWrite' again.
    obj.upgradeMessageEventModeToWrite();

    //   10. Check MessageEventMode' is the same as after last call of
    //       'upgradeMessageEventModeToWrite'.
    BMQTST_ASSERT_EQ(bmqimp::Event::MessageEventMode::e_WRITE,
                     obj.messageEventMode());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 12: test12_upgradeDowngradeMessageEvent(); break;
    case 11: test11_doneCallbackTest(); break;
    case 10: test10_assignmentTest(); break;
    case 9: test9_copyTest(); break;
    case 8: test8_putEventBuilder(); break;
    case 7: test7_printing(); break;
    case 6: test6_comparisonOperatorTest(); break;
    case 5: test5_configureAsMessageEventTest(); break;
    case 4: test4_messageEvent_setterGetterTest(); break;
    case 3: test3_sessionEvent_setterGetterTest(); break;
    case 2: test2_setterGetterTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
