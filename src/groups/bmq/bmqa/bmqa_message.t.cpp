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

// bmqa_message.t.cpp                                                 -*-C++-*-
#include <bmqa_message.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_memory.h>

// BMQ
#include <bmqa_event.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_mocksession.h>
#include <bmqimp_event.h>
#include <bmqimp_queue.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_crc32c.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_puteventbuilder.h>
#include <bmqt_subscription.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {
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
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_messageOnStackIsInvalid()
// ------------------------------------------------------------------------
// MESSAGE ON STACK IS INVALID
//
// Concerns:
//   A 'bmqa::Message' constructed on the stack (as opposed to obtained
//   from a 'bmqa::MessageEventBuilder') is "invalid", in the sense that
//   every method requiring the object to be valid fails to execute).
//
// Testing:
//   'explicit bmqa::Message();'
//   'Message clone(bslma::Allocator *basicAllocator = 0)' of invalid msg
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    PV("Default constructor - uninitialized");

    bmqa::Message msg;

    ASSERT_SAFE_FAIL(msg.queueId());
    ASSERT_SAFE_FAIL(msg.correlationId());
#ifdef BMQ_ENABLE_MSG_GROUPID
    ASSERT_SAFE_FAIL(msg.groupId());
#endif
    ASSERT_SAFE_FAIL(msg.messageGUID());
    ASSERT_SAFE_FAIL(msg.confirmationCookie());
    ASSERT_SAFE_FAIL(msg.ackStatus());
    ASSERT_SAFE_FAIL(msg.dataSize());
    ASSERT_SAFE_FAIL(msg.hasProperties());
#ifdef BMQ_ENABLE_MSG_GROUPID
    ASSERT_SAFE_FAIL(msg.hasGroupId());
#endif

    PV("Cloned object - uninitialized");

    bmqa::Message clone = msg.clone();

    ASSERT_SAFE_FAIL(clone.queueId());
    ASSERT_SAFE_FAIL(clone.correlationId());
#ifdef BMQ_ENABLE_MSG_GROUPID
    ASSERT_SAFE_FAIL(clone.groupId());
#endif
    ASSERT_SAFE_FAIL(clone.messageGUID());
    ASSERT_SAFE_FAIL(clone.confirmationCookie());
    ASSERT_SAFE_FAIL(clone.ackStatus());
    ASSERT_SAFE_FAIL(clone.dataSize());
    ASSERT_SAFE_FAIL(clone.hasProperties());
#ifdef BMQ_ENABLE_MSG_GROUPID
    ASSERT_SAFE_FAIL(clone.hasGroupId());
#endif
}

static void test2_validPushMessagePrint()
// ------------------------------------------------------------------------
// BASIC CHECK OF PRINTING VALID PUSH MESSAGE
//
// Concerns:
//   Use bmqp::PushEventBuilder and pack it with PUSH messages. Construct
//   valid bmqa::Message and set corresponding values, in the sense that
//   accessors have correct instance variables.
//
// Testing:
//   'explicit bmqa::Message();'
//   'Message clone(bslma::Allocator *basicAllocator = 0)' of invalid msg
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    typedef bsl::shared_ptr<bmqimp::Event> EventImplSp;

    bdlbb::PooledBlobBufferFactory bufferFactory(4 * 1024, s_allocator_p);
    bmqa::Event                    event;

    EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
    implPtr = bsl::make_shared<bmqimp::Event>(&bufferFactory, s_allocator_p);

    const int               queueId = 4321;
    const bmqt::MessageGUID guid;
    const char*             buffer = "abcdefghijklmnopqrstuvwxyz";
    const int               flags  = 0;
    const int               numSubQueueInfos =
        bmqp::Protocol::SubQueueInfosArray::static_size + 4;

    bmqp::Protocol::SubQueueInfosArray subQueueInfos(s_allocator_p);
    bdlbb::Blob                        payload(&bufferFactory, s_allocator_p);
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));
    ASSERT_EQ(static_cast<unsigned int>(payload.length()),
              bsl::strlen(buffer));

    // Create PushEventBuilder
    bmqp::PushEventBuilder peb(&bufferFactory, s_allocator_p);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    ASSERT_EQ(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob().length()));
    ASSERT_EQ(0, peb.messageCount());

    // Add SubQueueInfo option
    generateSubQueueInfos(&subQueueInfos, numSubQueueInfos);
    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);
    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
    ASSERT_EQ(sizeof(bmqp::EventHeader), static_cast<size_t>(peb.eventSize()));
    // 'eventSize()' excludes unpacked messages
    ASSERT_LT(sizeof(bmqp::EventHeader),
              static_cast<size_t>(peb.blob().length()));
    // But the option is written to the underlying blob
    rc = peb.packMessage(payload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(payload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    bmqp::Event bmqpEvent(&peb.blob(), s_allocator_p, true);
    implPtr->configureAsMessageEvent(bmqpEvent);

    implPtr->addCorrelationId(bmqt::CorrelationId());

    bmqa::MessageEvent    pushMsgEvt = event.messageEvent();
    bmqa::MessageIterator mIter      = pushMsgEvt.messageIterator();
    mIter.nextMessage();
    bmqa::Message message = mIter.message();
    ASSERT_EQ(message.compressionAlgorithmType(),
              bmqt::CompressionAlgorithmType::e_NONE);
}

static void test3_messageProperties()
// ------------------------------------------------------------------------
// CHECK OF MESASGEPROPERTIES PARSING
//
// Concerns:
//   Check access to message properties in combination with modifications.
//
// Testing:
//   'explicit bmqa::Message();'
//   'Message clone(bslma::Allocator *basicAllocator = 0)' of invalid msg
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    bdlbb::PooledBlobBufferFactory bufferFactory(4 * 1024, s_allocator_p);

    const int               queueId = 4321;
    const bmqt::MessageGUID guid;
    const char*             buffer = "abcdefghijklmnopqrstuvwxyz";
    const int               flags  = 0;

    bmqp::Protocol::SubQueueInfosArray subQueueInfos(s_allocator_p);
    bdlbb::Blob                        payload(&bufferFactory, s_allocator_p);

    const unsigned int subQueueId = 1234;
    subQueueInfos.push_back(bmqp::SubQueueInfo(subQueueId));

    bmqp::MessageProperties     in(s_allocator_p);
    bmqp::MessagePropertiesInfo input(true, 1, false);
    const char                  x[]   = "x";
    const char                  y[]   = "y";
    const char                  z[]   = "z";
    const char                  mod[] = "mod";

    in.setPropertyAsString("z", z);
    in.setPropertyAsString("y", y);
    in.setPropertyAsString("x", x);

    const bdlbb::Blob blob = in.streamOut(&bufferFactory, input);

    bdlbb::BlobUtil::append(&payload, blob);
    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));

    // Create PushEventBuilder
    bmqp::PushEventBuilder peb(&bufferFactory, s_allocator_p);

    // Add SubQueueInfo option
    bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
        subQueueInfos);
    ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);

    rc = peb.packMessage(payload,
                         queueId,
                         guid,
                         flags,
                         bmqt::CompressionAlgorithmType::e_NONE,
                         input);

    ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
    ASSERT_LT(payload.length(), peb.eventSize());
    ASSERT_EQ(1, peb.messageCount());

    bmqa::Event                     event;
    bsl::shared_ptr<bmqimp::Event>& implPtr =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);

    implPtr = bsl::make_shared<bmqimp::Event>(&bufferFactory, s_allocator_p);

    // For the ScheamLearner
    bsl::shared_ptr<bmqimp::Queue> queue =
        bsl::allocate_shared<bmqimp::Queue, bslma::Allocator>(s_allocator_p);
    queue->setId(queueId);
    implPtr->insertQueue(subQueueId, queue);

    bmqp::Event bmqpEvent(&peb.blob(), s_allocator_p, true);

    implPtr->configureAsMessageEvent(bmqpEvent);
    implPtr->addCorrelationId(bmqt::CorrelationId());

    bmqa::MessageEvent    pushMsgEvt = event.messageEvent();
    bmqa::MessageIterator mIter      = pushMsgEvt.messageIterator();
    mIter.nextMessage();
    bmqa::Message message = mIter.message();

    bmqa::MessageProperties out1(s_allocator_p);
    ASSERT_EQ(0, message.loadProperties(&out1));

    // 1st setProperty w/o getProperty and then getProperty
    {
        bmqa::MessageProperties out2(s_allocator_p);

        // The second read is/was optimized (only one MPS header)
        ASSERT_EQ(0, message.loadProperties(&out2));

        ASSERT_EQ(0, out2.setPropertyAsString("y", mod));
        ASSERT_EQ(out1.totalSize() + sizeof(mod) - sizeof(y),
                  out2.totalSize());

        ASSERT_EQ(out2.getPropertyAsString("z"), z);
    }

    // 2nd getProperty, setProperty and then load all
    {
        bmqa::MessageProperties out3(s_allocator_p);

        // The third read is/was optimized (only one MPS header)
        ASSERT_EQ(0, message.loadProperties(&out3));

        ASSERT_EQ(y, out3.getPropertyAsString("y"));
        ASSERT_EQ(0, out3.setPropertyAsString("y", mod));

        bmqu::MemOutStream os(s_allocator_p);
        out3.print(os, 0, -1);

        PV(os.str());

        bmqa::MessagePropertiesIterator it(&out3);

        ASSERT(it.hasNext());
        ASSERT_EQ(it.getAsString(), x);
        ASSERT(it.hasNext());
        ASSERT_EQ(it.getAsString(), mod);
        ASSERT(it.hasNext());
        ASSERT_EQ(it.getAsString(), z);
    }

    // 3rd getProperty, setProperty and then getProperty
    {
        bmqa::MessageProperties out4(s_allocator_p);

        // The fourth read is/was optimized (only one MPS header)
        ASSERT_EQ(0, message.loadProperties(&out4));

        ASSERT_EQ(y, out4.getPropertyAsString("y"));
        ASSERT_EQ(0, out4.setPropertyAsString("y", mod));
        ASSERT_EQ(out1.totalSize() + sizeof(mod) - sizeof(y),
                  out4.totalSize());

        ASSERT_EQ(out4.getPropertyAsString("z"), z);
    }
}

static void test4_subscriptionHandle()
// ------------------------------------------------------------------------
// BASIC CHECK OF MESSAGE SUBSCRIPTION HANDLE ACCESSOR
//
// Concerns:
//   Use bmqp::PushEventBuilder and pack it with PUSH messages. Construct
//   valid bmqa::Message and set corresponding values, in the sense that
//   accessor has correct instance variable.
//   In addition check the behavior for ACK and PUT messages.
//
// Testing:
//   'const bmqt::SubscriptionHandle& subscriptionHandle()' of PUSH, ACK
//   and PUT messages
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // Can't ensure no default memory is allocated because a default
    // QueueId is instantiated and that uses the default allocator to
    // allocate memory for an automatically generated CorrelationId.

    bmqtst::TestHelper::printTestName("SUBSCRIPTION HANDLE ACCESSOR TEST");

    // Initialize Crc32c
    bmqp::Crc32c::initialize();

    typedef bsl::shared_ptr<bmqimp::Event> EventImplSp;

    const int                 queueId = 4321;
    const bmqt::MessageGUID   guid    = bmqp::MessageGUIDGenerator::testGUID();
    const char*               buffer  = "abcdefghijklmnopqrstuvwxyz";
    const int                 flags   = 0;
    const bmqt::CorrelationId cId(queueId);

    bsl::shared_ptr<bmqimp::Queue> queueSp = bsl::make_shared<bmqimp::Queue>(
        s_allocator_p);
    bdlbb::PooledBlobBufferFactory bufferFactory(4 * 1024, s_allocator_p);
    bdlbb::Blob                    payload(&bufferFactory, s_allocator_p);

    queueSp->setId(queueId);

    bdlbb::BlobUtil::append(&payload, buffer, bsl::strlen(buffer));
    ASSERT_EQ(static_cast<unsigned int>(payload.length()),
              bsl::strlen(buffer));

    PV("PUSH MESSAGE - SUBSCRIPTION")
    {
        const bmqt::SubscriptionHandle sHandle(cId);
        const unsigned int             sId = sHandle.id();

        bmqa::Event                        event;
        bmqp::Protocol::SubQueueInfosArray subQueueInfos(s_allocator_p);

        EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
        implPtr              = bsl::make_shared<bmqimp::Event>(&bufferFactory,
                                                  s_allocator_p);

        // Create PushEventBuilder
        bmqp::PushEventBuilder peb(&bufferFactory, s_allocator_p);
        ASSERT_EQ(0, peb.messageCount());

        // Add SubQueueInfo option (subscription Id)
        subQueueInfos.push_back(bmqp::SubQueueInfo(sId));

        bmqt::EventBuilderResult::Enum rc = peb.addSubQueueInfosOption(
            subQueueInfos);
        ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS, rc);
        ASSERT_EQ(sizeof(bmqp::EventHeader),
                  static_cast<size_t>(peb.eventSize()));
        // 'eventSize()' excludes unpacked messages
        ASSERT_LT(sizeof(bmqp::EventHeader),
                  static_cast<size_t>(peb.blob().length()));
        // But the option is written to the underlying blob

        // Add message
        rc = peb.packMessage(payload,
                             queueId,
                             guid,
                             flags,
                             bmqt::CompressionAlgorithmType::e_NONE);

        ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
        ASSERT_LT(payload.length(), peb.eventSize());
        ASSERT_EQ(1, peb.messageCount());

        bmqp::Event bmqpEvent(&peb.blob(), s_allocator_p, true);
        implPtr->configureAsMessageEvent(bmqpEvent);

        implPtr->insertQueue(sId, queueSp);
        implPtr->addCorrelationId(cId, sId);

        bmqa::MessageEvent    pushMsgEvt = event.messageEvent();
        bmqa::MessageIterator mIter      = pushMsgEvt.messageIterator();

        ASSERT(mIter.nextMessage());
        bmqa::Message message = mIter.message();
        PVVV("Message: " << message);

        const bmqt::SubscriptionHandle& actualHandle =
            message.subscriptionHandle();
        PVV("Non-empty subscription handle: " << actualHandle);
        ASSERT_EQ(actualHandle.id(), sId);
        ASSERT_EQ(actualHandle.correlationId(), sHandle.correlationId());
    }

    PV("PUSH MESSAGE - NO SUBSCRIPTION")
    {
        // Empty correlation Id
        const bmqt::CorrelationId emptyCorrelationId;
        const unsigned int        defaultSubscriptionId =
            bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID;

        bmqa::Event event;

        EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
        implPtr              = bsl::make_shared<bmqimp::Event>(&bufferFactory,
                                                  s_allocator_p);

        // Create PushEventBuilder
        bmqp::PushEventBuilder peb(&bufferFactory, s_allocator_p);
        ASSERT_EQ(0, peb.messageCount());

        // Add message
        bmqt::EventBuilderResult::Enum rc = peb.packMessage(
            payload,
            queueId,
            guid,
            flags,
            bmqt::CompressionAlgorithmType::e_NONE);

        ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
        ASSERT_LT(payload.length(), peb.eventSize());
        ASSERT_EQ(1, peb.messageCount());

        bmqp::Event bmqpEvent(&peb.blob(), s_allocator_p, true);
        implPtr->configureAsMessageEvent(bmqpEvent);

        implPtr->insertQueue(defaultSubscriptionId, queueSp);
        implPtr->addCorrelationId(emptyCorrelationId, defaultSubscriptionId);

        bmqa::MessageEvent    pushMsgEvt = event.messageEvent();
        bmqa::MessageIterator mIter      = pushMsgEvt.messageIterator();

        ASSERT(mIter.nextMessage());
        bmqa::Message message = mIter.message();
        PVVV("Message: " << message);

        const bmqt::SubscriptionHandle& actualHandle =
            message.subscriptionHandle();
        PVV("Empty subscription handle: " << actualHandle);
        ASSERT_EQ(actualHandle.id(), defaultSubscriptionId);
        ASSERT_EQ(actualHandle.correlationId(), emptyCorrelationId);
    }

    PV("PUT MESSAGE - FAIL")
    {
        bmqa::Event event;

        EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
        implPtr              = bsl::make_shared<bmqimp::Event>(&bufferFactory,
                                                  s_allocator_p);

        // Create PutEventBuilder
        bmqp::PutEventBuilder builder(&bufferFactory, s_allocator_p);
        ASSERT_EQ(0, builder.messageCount());

        // Add message
        builder.startMessage();
        builder.setMessagePayload(&payload);
        builder.setMessageGUID(guid);

        bmqt::EventBuilderResult::Enum rc = builder.packMessage(queueId);

        ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
        ASSERT_EQ(1, builder.messageCount());

        bmqp::Event bmqpEvent(&builder.blob(), s_allocator_p);
        implPtr->configureAsMessageEvent(bmqpEvent);

        implPtr->insertQueue(queueSp);
        implPtr->addCorrelationId(cId);

        bmqa::MessageEvent    putMsgEvt = event.messageEvent();
        bmqa::MessageIterator mIter     = putMsgEvt.messageIterator();

        ASSERT(mIter.nextMessage());
        bmqa::Message message = mIter.message();
        PVVV("Message: " << message);

        ASSERT_OPT_FAIL(message.subscriptionHandle());
    }

    PV("ACK MESSAGE - FAIL")
    {
        bmqa::Event event;

        EventImplSp& implPtr = reinterpret_cast<EventImplSp&>(event);
        implPtr              = bsl::make_shared<bmqimp::Event>(&bufferFactory,
                                                  s_allocator_p);

        // Create AckEventBuilder
        bmqp::AckEventBuilder builder(&bufferFactory, s_allocator_p);
        ASSERT_EQ(0, builder.messageCount());

        bmqt::EventBuilderResult::Enum rc =
            builder.appendMessage(0, queueId, guid, queueId);

        ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);
        ASSERT_EQ(1, builder.messageCount());

        bmqp::Event bmqpEvent(&builder.blob(), s_allocator_p);
        implPtr->configureAsMessageEvent(bmqpEvent);

        implPtr->insertQueue(queueSp);
        implPtr->addCorrelationId(cId);

        bmqa::MessageEvent    ackMsgEvt = event.messageEvent();
        bmqa::MessageIterator mIter     = ackMsgEvt.messageIterator();

        ASSERT(mIter.nextMessage());
        bmqa::Message message = mIter.message();
        PVVV("Message: " << message);

        ASSERT_OPT_FAIL(message.subscriptionHandle());
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);
    bmqp::ProtocolUtil::initialize(s_allocator_p);
    switch (_testCase) {
    case 0:
    case 1: test1_messageOnStackIsInvalid(); break;
    case 2: test2_validPushMessagePrint(); break;
    case 3: test3_messageProperties(); break;
    case 4: test4_subscriptionHandle(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
