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

// bmqa_mocksession.t.cpp                                             -*-C++-*-
#include <bmqa_mocksession.h>

// BMQ
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_event.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_session.h>
#include <bmqimp_queue.h>
#include <bmqp_protocolutil.h>
#include <bmqt_queueoptions.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_variant.h>
#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bslmt_threadutil.h>
#include <bsls_platform.h>
#include <bsls_protocoltest.h>

#include <bmqsys_time.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// Disable some compiler warning for simplified write of the 'MockSession'.
#if defined(BSLS_PLATFORM_CMP_CLANG)
#pragma clang diagnostic ignored "-Wweak-vtables"
// Disabling 'weak-vtables' so that we can define all interface methods
// inline, without the following warning:
//..
//  bmqa_mocksession.t.cpp:48:8: error: 'EventHandler' has no out-of-line
//  virtual method definitions; its vtable will be emitted in every
//  translation unit [-Werror,-Wweak-vtables]
//..
#endif  // BSLS_PLATFORM_CMP_CLANG

// ============================================================================
//                 HELPER CLASSES AND FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

/// Mapping class.  Will be partially specialized below.
template <typename T>
struct ResultRegistry {};

/// Partial specialization for mapping to `bmqa::OpenQueueStatus`.
template <>
struct ResultRegistry<bmqa::OpenQueueStatus> {
    static const int TypeIndex = 1;
};

/// Partial specialization for mapping to `bmqa::ConfigureQueueStatus`.
template <>
struct ResultRegistry<bmqa::ConfigureQueueStatus> {
    static const int TypeIndex = 2;
};

/// Partial specialization for mapping to `bmqa::CloseQueueStatus`.
template <>
struct ResultRegistry<bmqa::CloseQueueStatus> {
    static const int TypeIndex = 3;
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

struct EventHandler : public bmqa::SessionEventHandler {
    typedef bdlb::Variant<bmqa::OpenQueueStatus,
                          bmqa::ConfigureQueueStatus,
                          bmqa::CloseQueueStatus>
        Result;

    bsl::deque<bmqa::SessionEvent> d_receivedSessionEvents;

    bsl::deque<Result> d_receivedResults;

    bsl::deque<bmqa::MessageEvent> d_receivedMessageEvents;

    size_t d_assertsInvoked;

    bslma::Allocator* d_allocator_p;

    // CLASS METHODS

    /// Successfully open a queue in the `e_WRITE` mode with the specified
    /// `mockSession_p`, `queueId` and `uri`.
    static void openQueue(bmqa::MockSession* mockSession_p,
                          bmqa::QueueId*     queueId,
                          const bmqt::Uri&   uri)
    {
        bmqa::MockSession& mockSession = *mockSession_p;

        mockSession.enqueueEvent(
            bmqa::MockSessionUtil::createQueueSessionEvent(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                queueId,
                queueId->correlationId(),
                0,
                "",
                bmqtst::TestHelperUtil::allocator()));

        BMQA_EXPECT_CALL(mockSession,
                         openQueueAsync(0,  // Dont care about queueId
                                        uri,
                                        bmqt::QueueFlags::e_WRITE))
            .returning(0);
        mockSession.openQueueAsync(queueId, uri, bmqt::QueueFlags::e_WRITE);
        mockSession.emitEvent();
    }

    /// Compare if the specified `lhs` event is equal to the specified `rhs`
    /// event.  This method only compares if they stream equal.
    static bool compareEvents(const bmqa::MessageEvent& lhs,
                              const bmqa::MessageEvent& rhs)
    {
        // This portion of comparing message should be completely
        // implementation dependent.  BlazingMQ provides no specific
        // 'operator==' because a message has multiple aspects to check and
        // this should be implemented according to application needs.  This may
        // imply checking payload, correlationId, queueId and messageProperties
        // of each message among others.  For brevity in our tests we have
        // opted to just compare the streams of the two messageEvents which is
        // markedly a weak test, but serves us well enough for basic testing.

        bmqu::MemOutStream lstr(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream rstr(bmqtst::TestHelperUtil::allocator());

        lhs.print(lstr);
        rhs.print(rstr);

        return lstr.str() == rstr.str();
    }

    // CREATORS
    EventHandler(bslma::Allocator* allocator)
    : d_receivedSessionEvents(allocator)
    , d_receivedMessageEvents(allocator)
    , d_assertsInvoked(0)
    , d_allocator_p(allocator)
    {
        // NOTHING
    }

    ~EventHandler() BSLS_KEYWORD_OVERRIDE
    {
        size_t unpoppedEvents = d_receivedMessageEvents.size() +
                                d_receivedSessionEvents.size() +
                                d_receivedResults.size();

        if (unpoppedEvents > 0) {
            bsl::cout << "Un-popped events:\n";
        }

        for (size_t i = 0; i != d_receivedMessageEvents.size(); ++i) {
            bsl::cout << d_receivedMessageEvents[i] << "\n";
        }
        for (size_t i = 0; i != d_receivedSessionEvents.size(); ++i) {
            bsl::cout << d_receivedSessionEvents[i] << "\n";
        }

        for (size_t i = 0; i != d_receivedResults.size(); ++i) {
            bsl::cout << d_receivedResults[i] << "\n";
        }

        BSLS_ASSERT((unpoppedEvents == 0) &&
                    "Un-popped events in event handler");
    }

    // MANIPULATORS
    void onSessionEvent(const bmqa::SessionEvent& event) BSLS_KEYWORD_OVERRIDE
    {
        d_receivedSessionEvents.push_back(event);
    }

    void onMessageEvent(const bmqa::MessageEvent& event) BSLS_KEYWORD_OVERRIDE
    {
        d_receivedMessageEvents.push_back(event);
    }

    void onOpenQueueStatus(const bmqa::OpenQueueStatus& result)
    {
        d_receivedResults.emplace_back(result);
    }

    void onConfigureQueueResult(const bmqa::ConfigureQueueStatus& result)
    {
        d_receivedResults.emplace_back(result);
    }

    void onCloseQueueStatus(const bmqa::CloseQueueStatus& result)
    {
        d_receivedResults.emplace_back(result);
    }

    bmqa::SessionEvent popSessionEvent()
    {
        BSLS_ASSERT(d_receivedSessionEvents.size() > 0);
        bmqa::SessionEvent ret(d_receivedSessionEvents.front());
        d_receivedSessionEvents.pop_front();
        return ret;
    }

    bmqa::MessageEvent popMessageEvent()
    {
        BSLS_ASSERT(d_receivedMessageEvents.size() > 0);
        bmqa::MessageEvent ret(d_receivedMessageEvents.front());
        d_receivedMessageEvents.erase(d_receivedMessageEvents.begin());
        return ret;
    }

    template <typename RESULT_TYPE>
    RESULT_TYPE popResult()
    {
        BSLS_ASSERT(d_receivedResults.size() > 0);
        BSLS_ASSERT(ResultRegistry<RESULT_TYPE>::TypeIndex ==
                    d_receivedResults.front().typeIndex());
        RESULT_TYPE result(d_receivedResults.front().the<RESULT_TYPE>());
        d_receivedResults.pop_front();
        return result;
    }

    void incrementAsserts(BSLS_ANNOTATION_UNUSED const char* desc,
                          BSLS_ANNOTATION_UNUSED const char* file,
                          BSLS_ANNOTATION_UNUSED int         line)
    {
        ++d_assertsInvoked;
    }

    void clearEvents()
    {
        d_receivedSessionEvents.clear();
        d_receivedMessageEvents.clear();
        d_receivedResults.clear();
    }
};

static void test1_staticMethods()
{
    bmqp::ProtocolUtil::initialize();

    bmqtst::TestHelper::printTestName("STATIC METHODS");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4 * 1024,
        bmqtst::TestHelperUtil::allocator());

    {
        PVV("Create Session Event");
        bsl::string errorDescription("some random description");
        bmqa::Event event = bmqa::MockSessionUtil::createSessionEvent(
            bmqt::SessionEventType::e_CONNECTED,
            bmqt::CorrelationId(1),
            0,
            errorDescription,
            bmqtst::TestHelperUtil::allocator());
        bmqa::SessionEvent sessionEvent = event.sessionEvent();

        ASSERT_EQ(sessionEvent.type(), bmqt::SessionEventType::e_CONNECTED);
        ASSERT_EQ(sessionEvent.statusCode(), 0);
        ASSERT_EQ(sessionEvent.errorDescription(), errorDescription);
        ASSERT_EQ(sessionEvent.correlationId(), bmqt::CorrelationId(1));
    }

    {
        PVV("Create Queue Session Event using Session Event Method");

        ASSERT_FAIL(bmqa::MockSessionUtil::createSessionEvent(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            bmqt::CorrelationId(1),
            0,
            "",
            bmqtst::TestHelperUtil::allocator()));
    }

    {
        PVV("Create Ack Event");

        bmqa::QueueId       queueId(1);
        bmqt::CorrelationId corrId(1);

        bsl::string       errorDescription("some random description");
        const char        guidHex[] = "00000000000000000000000000000001";
        bmqt::MessageGUID guid;
        guid.fromHex(guidHex);

        bsl::vector<bmqa::MockSessionUtil::AckParams> acks(
            bmqtst::TestHelperUtil::allocator());
        acks.emplace_back(bmqt::AckResult::e_SUCCESS, corrId, guid, queueId);

        bmqa::Event event = bmqa::MockSessionUtil::createAckEvent(
            acks,
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

        bmqa::MessageEvent ackEvent = event.messageEvent();
        ASSERT_EQ(ackEvent.type(), bmqt::MessageEventType::e_ACK);

        bmqa::MessageIterator mIter = ackEvent.messageIterator();
        mIter.nextMessage();
        ASSERT_EQ(mIter.message().ackStatus(), bmqt::AckResult::e_SUCCESS);
        ASSERT_EQ(mIter.message().queueId(), queueId);
        ASSERT_EQ(mIter.message().correlationId(), corrId);
    }

    {
        PVV("Create Queue Session Event");
        bsl::string errorDescription("some random description");

        bmqa::QueueId       queueId(1);
        bmqt::CorrelationId corrId(1);

        bmqa::Event event = bmqa::MockSessionUtil::createQueueSessionEvent(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            &queueId,
            corrId,
            0,
            "",
            bmqtst::TestHelperUtil::allocator());

        bmqa::SessionEvent openQueueEvent = event.sessionEvent();
        ASSERT_EQ(openQueueEvent.type(),
                  bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
        ASSERT_EQ(openQueueEvent.statusCode(), 0);
        ASSERT_EQ(openQueueEvent.errorDescription(), "");
        ASSERT_EQ(openQueueEvent.correlationId(), corrId);
    }

    {
        PVV("Create Message Event");

        bmqa::QueueId queueId(1);

        bsl::vector<bmqa::MockSessionUtil::PushMessageParams> pushMsgs(
            bmqtst::TestHelperUtil::allocator());
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobUtil::append(&payload, "hello", 6);

        const char        guidHex[] = "00000000000000000000000000000001";
        bmqt::MessageGUID guid;
        guid.fromHex(guidHex);

        bmqa::MessageProperties properties;

        properties.setPropertyAsInt32("x", 11);

        pushMsgs.emplace_back(payload, queueId, guid, properties);
        bmqa::Event event = bmqa::MockSessionUtil::createPushEvent(
            pushMsgs,
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator());

        bmqa::MessageEvent pushMsgEvt = event.messageEvent();

        ASSERT_EQ(pushMsgEvt.type(), bmqt::MessageEventType::e_PUSH);

        bmqa::MessageIterator mIter = pushMsgEvt.messageIterator();
        mIter.nextMessage();
        ASSERT_EQ(mIter.message().queueId(), queueId);
        ASSERT_EQ(mIter.message().messageGUID(), guid);
        ASSERT_EQ(mIter.message().dataSize(), 6);

        bmqa::MessageProperties out;
        ASSERT_EQ(mIter.message().loadProperties(&out), 0);

        ASSERT_EQ(out.totalSize(), properties.totalSize());
        ASSERT_EQ(out.getPropertyAsInt32("x"),
                  properties.getPropertyAsInt32("x"));
    }

    bmqp::ProtocolUtil::shutdown();
}

static void test2_call()
{
    bmqtst::TestHelper::printTestName("CALL");

    bsl::shared_ptr<bmqa::MockSession> mockSession_sp;

    mockSession_sp.createInplace(
        bmqtst::TestHelperUtil::allocator(),
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bmqa::MockSession& mockSession = *mockSession_sp;

    {
        PVV("Incorrect call");
        BMQA_EXPECT_CALL(mockSession, start()).returning(0);
        ASSERT_FAIL(mockSession.stop());
        ASSERT_EQ(mockSession.start(), 0);
    }

    {
        PVV("Empty expected call queue");
        ASSERT_FAIL(mockSession.startAsync());
    }

    {
        PVV("Incorrect arguments");
        BMQA_EXPECT_CALL(mockSession, startAsync(bsls::TimeInterval(10)))
            .returning(0);
        ASSERT_FAIL(mockSession.startAsync(bsls::TimeInterval(1)));
        // To clear the expected queue.
        ASSERT_EQ(mockSession.startAsync(bsls::TimeInterval(10)), 0);
    }

    {
        PVV("Custom callback with non empty expect call queue");

        // Only used for the custom callback
        EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

        mockSession.setFailureCallback(
            bdlf::BindUtil::bind(&EventHandler::incrementAsserts,
                                 &eventHandler,
                                 bdlf::PlaceHolders::_1,    // description
                                 bdlf::PlaceHolders::_2,    // file
                                 bdlf::PlaceHolders::_3));  // line

        BMQA_EXPECT_CALL(mockSession, startAsync(bsls::TimeInterval(10)))
            .returning(0);
        BMQA_EXPECT_CALL(mockSession, stop()).returning(0);

        // The destructor should call our custom assert
        mockSession_sp.clear();

        // Our mockSession reference is also invalid at this point.
        ASSERT_EQ(eventHandler.d_assertsInvoked, 1u);
    }
}

static void test3_queueManagement()
{
    bmqtst::TestHelper::printTestName("QUEUE MANAGEMENT");

    EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
    handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);

    bmqa::MockSession mockSession(
        handlerMp,
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bsl::string error(bmqtst::TestHelperUtil::allocator());
    bsl::string input1("bmq://my.domain/queue1",
                       bmqtst::TestHelperUtil::allocator());
    bsl::string input2("bmq://my.domain/queue2",
                       bmqtst::TestHelperUtil::allocator());
    bmqt::Uri   uri1(bmqtst::TestHelperUtil::allocator());
    bmqt::Uri   uri2(bmqtst::TestHelperUtil::allocator());
    bmqt::UriParser::parse(&uri1, &error, input1);
    bmqt::UriParser::parse(&uri2, &error, input2);

    bsls::Types::Int64 nextCorrId = 1;

    bmqt::CorrelationId corrId1(nextCorrId++);
    bmqt::CorrelationId corrId2(nextCorrId++);

    bmqa::QueueId queueId1(corrId1, bmqtst::TestHelperUtil::allocator());
    bmqa::QueueId queueId2(corrId2, bmqtst::TestHelperUtil::allocator());

    bmqa::MockSession::OpenQueueCallback openQueueCallback =
        bdlf::MemFnUtil::memFn(&EventHandler::onOpenQueueStatus,
                               &eventHandler);

    bmqa::MockSession::ConfigureQueueCallback configureQueueCallback =
        bdlf::MemFnUtil::memFn(&EventHandler::onConfigureQueueResult,
                               &eventHandler);

    bmqa::MockSession::CloseQueueCallback closeQueueCallback(
        bdlf::MemFnUtil::memFn(&EventHandler::onCloseQueueStatus,
                               &eventHandler));

    {
        PVV("Verify state changes");

        {
            PVVV("Unsuccessful openQueueAsync response");
            bmqa::OpenQueueStatus openQueueResult =
                bmqa::MockSessionUtil::createOpenQueueStatus(
                    queueId1,
                    bmqt::OpenQueueResult::e_UNKNOWN,
                    "",
                    bmqtst::TestHelperUtil::allocator());
            BMQA_EXPECT_CALL(mockSession,
                             openQueueAsync(&queueId1,
                                            uri1,
                                            bmqt::QueueFlags::e_READ,
                                            openQueueCallback))
                .emitting(openQueueResult);

            mockSession.openQueueAsync(&queueId1,
                                       uri1,
                                       bmqt::QueueFlags::e_READ,
                                       openQueueCallback);

            typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
            QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId1);

            ASSERT_EQ(implPtr->uri(), uri1);
            ASSERT_EQ(implPtr->correlationId(), corrId1);

            ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENING_OPN);

            ASSERT_EQ(mockSession.emitEvent(), true);

            bmqa::OpenQueueStatus result =
                eventHandler.popResult<bmqa::OpenQueueStatus>();
            ASSERT_EQ(result.queueId(), openQueueResult.queueId());
            ASSERT_EQ(result.result(), openQueueResult.result());
            ASSERT_EQ(result.errorDescription(),
                      openQueueResult.errorDescription());

            ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_CLOSED);
        }

        {
            PVVV("Successful openQueue response");
            BMQA_EXPECT_CALL(mockSession,
                             openQueueAsync(&queueId1,
                                            uri1,
                                            bmqt::QueueFlags::e_READ,
                                            openQueueCallback))
                .emitting(bmqa::MockSessionUtil::createOpenQueueStatus(
                    queueId1,
                    bmqt::OpenQueueResult::e_SUCCESS,
                    "",
                    bmqtst::TestHelperUtil::allocator()));

            mockSession.openQueueAsync(&queueId1,
                                       uri1,
                                       bmqt::QueueFlags::e_READ,
                                       openQueueCallback);

            typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
            QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId1);

            ASSERT_EQ(implPtr->uri(), uri1);
            ASSERT_EQ(implPtr->correlationId(), corrId1);

            ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENING_OPN);

            ASSERT_EQ(mockSession.emitEvent(), true);

            bmqa::OpenQueueStatus result =
                eventHandler.popResult<bmqa::OpenQueueStatus>();

            ASSERT_EQ(result.result(), 0);
            ASSERT_EQ(result.errorDescription(), "");
            ASSERT_EQ(result.queueId().correlationId(), corrId1);

            ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENED);
        }
    }

    {
        PVV("Get queue");
        // queueId1 is open

        {
            PVVV("Valid queue by uri");
            bmqa::QueueId queueIdFound(bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, uri1), 0);
            ASSERT_EQ(queueIdFound, queueId1);
        }

        {
            PVVV("Valid queue by uri");
            bmqa::QueueId queueIdFound(bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, corrId1), 0);
            ASSERT_EQ(queueIdFound, queueId1);
        }

        {
            PVVV("Registered but unused queue");
            bmqa::QueueId queueIdFound(bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, uri2), -1);
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, corrId2), -1);
        }

        {
            PVVV("Get closed queue");

            bmqa::CloseQueueStatus closeResult1 =
                bmqa::MockSessionUtil::createCloseQueueStatus(
                    queueId1,
                    bmqt::CloseQueueResult::e_SUCCESS,
                    "",
                    bmqtst::TestHelperUtil::allocator());

            // Close queue and then attempt to get queue
            BMQA_EXPECT_CALL(mockSession, closeQueueSync(&queueId1))
                .returning(closeResult1);
            ASSERT_EQ(mockSession.closeQueueSync(&queueId1), closeResult1);
            ASSERT_EQ(closeResult1.queueId(), queueId1);
            ASSERT_EQ(closeResult1.result(),
                      bmqt::CloseQueueResult::e_SUCCESS);

            bmqa::QueueId queueIdFound(bmqtst::TestHelperUtil::allocator());
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, uri1), -1);
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, corrId1), -1);

            // Close queue successfully and then attempt to get queue
            bmqa::CloseQueueStatus closeResult2 =
                bmqa::MockSessionUtil::createCloseQueueStatus(
                    queueId1,
                    bmqt::CloseQueueResult::e_SUCCESS,
                    "",
                    bmqtst::TestHelperUtil::allocator());

            BMQA_EXPECT_CALL(mockSession, closeQueueSync(&queueId1))
                .returning(closeResult2);
            ASSERT_EQ(mockSession.closeQueueSync(&queueId1), closeResult2);

            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, uri1), -1);
            ASSERT_EQ(mockSession.getQueueId(&queueIdFound, corrId1), -1);
        }
    }

    {
        PVV("Queue re-open with new queueId object");

        bmqa::OpenQueueStatus testOpenQueueResult =
            bmqa::MockSessionUtil::createOpenQueueStatus(
                queueId1,
                bmqt::OpenQueueResult::e_SUCCESS,
                "",
                bmqtst::TestHelperUtil::allocator());

        // Generate a new queueId with the same correlationId.  The underlying
        // impl is different so the queueIds are different.  Ensure correct
        // behaviour with new queueId.

        // Save the original queueId to compare.
        bmqa::QueueId savedQueueId(queueId1,
                                   bmqtst::TestHelperUtil::allocator());

        // Overwrite the queueId we are going to use.
        queueId1 = bmqa::QueueId(corrId1, bmqtst::TestHelperUtil::allocator());

        BMQA_EXPECT_CALL(
            mockSession,
            openQueueSync(&queueId1, uri1, bmqt::QueueFlags::e_READ))
            .returning(testOpenQueueResult);

        ASSERT_EQ(mockSession.openQueueSync(&queueId1,
                                            uri1,
                                            bmqt::QueueFlags::e_READ),
                  testOpenQueueResult);

        typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
        QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId1);

        ASSERT_EQ(implPtr->uri(), uri1);
        ASSERT_EQ(implPtr->correlationId(), corrId1);
        ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENED);

        bmqa::QueueId queueIdFound(bmqtst::TestHelperUtil::allocator());
        ASSERT_EQ(mockSession.getQueueId(&queueIdFound, uri1), 0);

        ASSERT(queueId1 == queueIdFound);
        ASSERT_NE(queueId1, savedQueueId);

        bmqa::CloseQueueStatus closeResult1 =
            bmqa::MockSessionUtil::createCloseQueueStatus(
                queueId1,
                bmqt::CloseQueueResult::e_SUCCESS,
                "",
                bmqtst::TestHelperUtil::allocator());

        // Close queue and then attempt to get queue
        BMQA_EXPECT_CALL(mockSession, closeQueueSync(&queueId1))
            .returning(closeResult1);
        ASSERT_EQ(mockSession.closeQueueSync(&queueId1), closeResult1);
    }
}

static void test4_queueManagementSync()
{
    bmqtst::TestHelper::printTestName("QUEUE MANAGEMENT SYNC MODE");

    bmqa::MockSession mockSession(
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bsl::string input1 = "bmq://my.domain/queue1";
    bsl::string input2 = "bmq://my.domain/queue2";
    bmqt::Uri   uri1(bmqtst::TestHelperUtil::allocator());
    bmqt::Uri   uri2(bmqtst::TestHelperUtil::allocator());
    bsl::string error;

    bmqt::UriParser::parse(&uri1, &error, input1);
    bmqt::UriParser::parse(&uri2, &error, input2);

    bmqt::CorrelationId corrId1(1);
    bmqt::CorrelationId corrId2(2);

    bmqa::QueueId queueId1(corrId1);
    bmqa::QueueId queueId2(corrId2);

    PVV("Verify state changes");

    {
        PVVV("Unsuccessful openQueue response");
        BMQA_EXPECT_CALL(mockSession, openQueueAsync(&queueId1, uri1, 10))
            .returning(0);
        BMQA_EXPECT_CALL(mockSession, nextEvent())
            .returning(bmqa::MockSessionUtil::createQueueSessionEvent(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                &queueId1,
                queueId1.correlationId(),
                1,
                "",
                bmqtst::TestHelperUtil::allocator()));

        ASSERT_EQ(mockSession.openQueueAsync(&queueId1, uri1, 10), 0);
        typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
        QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId1);

        ASSERT_EQ(implPtr->uri(), uri1);
        ASSERT_EQ(implPtr->correlationId(), corrId1);
        ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENING_OPN);

        bmqa::SessionEvent openQueueEvent =
            mockSession.nextEvent().sessionEvent();

        ASSERT_EQ(openQueueEvent.type(),
                  bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
        ASSERT_EQ(openQueueEvent.statusCode(), 1);
        ASSERT_EQ(openQueueEvent.errorDescription(), "");
        ASSERT_EQ(openQueueEvent.correlationId(), corrId1);

        ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_CLOSED);
    }

    {
        PVVV("Successful openQueue response");
        BMQA_EXPECT_CALL(mockSession, openQueueAsync(&queueId1, uri1, 10))
            .returning(0);
        BMQA_EXPECT_CALL(mockSession, nextEvent())
            .returning(bmqa::MockSessionUtil::createQueueSessionEvent(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                &queueId1,
                queueId1.correlationId(),
                0,
                "",
                bmqtst::TestHelperUtil::allocator()));

        ASSERT_EQ(mockSession.openQueueAsync(&queueId1, uri1, 10), 0);
        typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
        QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId1);

        ASSERT_EQ(implPtr->uri(), uri1);
        ASSERT_EQ(implPtr->correlationId(), corrId1);
        ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENING_OPN);

        bmqa::SessionEvent openQueueEvent =
            mockSession.nextEvent().sessionEvent();
        ASSERT_EQ(openQueueEvent.type(),
                  bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
        ASSERT_EQ(openQueueEvent.statusCode(), 0);
        ASSERT_EQ(openQueueEvent.errorDescription(), "");
        ASSERT_EQ(openQueueEvent.correlationId(), corrId1);

        ASSERT_EQ(implPtr->state(), bmqimp::QueueState::e_OPENED);
    }
}

static void test5_confirmingMessages()
{
    bmqtst::TestHelper::printTestName("CONSUME AND CONFIRM");

    EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
    handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);

    bmqa::MockSession mockSession(
        handlerMp,
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4 * 1024,
        bmqtst::TestHelperUtil::allocator());

    bmqt::CorrelationId corrId(1);
    bmqa::QueueId       queueId(corrId);

    typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
    QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId);

    // Quickly make a valid queue available.
    implPtr->setState(bmqimp::QueueState::e_OPENED);
    implPtr->setId(1);  // arbitrary id for queue

    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&payload, "hello", 6);

    const char        guidHex1[] = "00000000000000000000000000000001";
    bmqt::MessageGUID guid1;
    guid1.fromHex(guidHex1);

    const char        guidHex2[] = "00000000000000000000000000000002";
    bmqt::MessageGUID guid2;
    guid2.fromHex(guidHex2);

    const char        guidHex3[] = "00000000000000000000000000000003";
    bmqt::MessageGUID guid3;
    guid3.fromHex(guidHex3);

    bmqa::MessageProperties properties;
    mockSession.loadMessageProperties(&properties);

    bsl::vector<bmqa::MockSessionUtil::PushMessageParams> pushMsgs(
        bmqtst::TestHelperUtil::allocator());
    pushMsgs.emplace_back(payload, queueId, guid1, properties);
    pushMsgs.emplace_back(payload, queueId, guid2, properties);
    pushMsgs.emplace_back(payload, queueId, guid3, properties);

    mockSession.enqueueEvent(bmqa::MockSessionUtil::createPushEvent(
        pushMsgs,
        &bufferFactory,
        bmqtst::TestHelperUtil::allocator()));

    ASSERT_EQ(mockSession.emitEvent(), true);

    ASSERT_EQ(mockSession.unconfirmedMessages(), 3u);
    bmqa::MessageEvent messageEvent = eventHandler.popMessageEvent();
    {
        PVV("Confirm push message");

        {
            PVVV("Unknown GUID confirm");

            // Build an invalid message with an unknown GUID and try to confirm
            // it.  We have to do it this way since we have no way to create a
            // push message or a message cookie on the stack.
            const char invalidGuidHex[] = "00000000000000000000000000000004";
            bmqt::MessageGUID invalidGUID;
            invalidGUID.fromHex(invalidGuidHex);

            bsl::vector<bmqa::MockSessionUtil::PushMessageParams> invalidMsg(
                bmqtst::TestHelperUtil::allocator());
            invalidMsg.emplace_back(payload, queueId, invalidGUID, properties);

            bmqa::MessageEvent invalidEvent =
                bmqa::MockSessionUtil ::createPushEvent(
                    invalidMsg,
                    &bufferFactory,
                    bmqtst::TestHelperUtil::allocator())
                    .messageEvent();

            bmqa::ConfirmEventBuilder confirmBuilder;
            mockSession.loadConfirmEventBuilder(&confirmBuilder);

            bmqa::MessageIterator mIter = messageEvent.messageIterator();

            mIter.nextMessage();
            int rc = confirmBuilder.addMessageConfirmation(mIter.message());

            ASSERT_EQ(rc, 0);
            ASSERT_EQ(confirmBuilder.messageCount(), 1);

            // we know the guid is invalid so we say the return value is -1.
            BMQA_EXPECT_CALL(mockSession, confirmMessages(&confirmBuilder))
                .returning(bmqt::GenericResult::e_INVALID_ARGUMENT);

            rc = mockSession.confirmMessages(&confirmBuilder);
            ASSERT_EQ(rc, bmqt::GenericResult::e_INVALID_ARGUMENT);

            // we know the guid is invalid so we say the return value is -1.
            BMQA_EXPECT_CALL(mockSession, confirmMessage(mIter.message()))
                .returning(bmqt::GenericResult::e_INVALID_ARGUMENT);

            rc = mockSession.confirmMessage(mIter.message());
            ASSERT_EQ(rc, bmqt::GenericResult::e_INVALID_ARGUMENT);

            // Finally ensure that no messages were confirmed. (3 messages were
            // consumed/received from the broker)
            ASSERT_EQ(mockSession.unconfirmedMessages(), 3u);
        }

        {
            PVVV("Valid GUID confirm");
            bmqa::ConfirmEventBuilder confirmBuilder;
            mockSession.loadConfirmEventBuilder(&confirmBuilder);

            bmqa::MessageIterator mIter = messageEvent.messageIterator();

            mIter.nextMessage();
            confirmBuilder.addMessageConfirmation(mIter.message());

            ASSERT_EQ(confirmBuilder.messageCount(), 1);

            BMQA_EXPECT_CALL(mockSession, confirmMessages(&confirmBuilder))
                .returning(0);
            int rc = mockSession.confirmMessages(&confirmBuilder);
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(mockSession.unconfirmedMessages(), 2u);
        }

        {
            PVVV("Re-confirm confirmed GUID");

            bmqa::ConfirmEventBuilder confirmBuilder;
            mockSession.loadConfirmEventBuilder(&confirmBuilder);

            bmqa::MessageIterator mIter = messageEvent.messageIterator();

            mIter.nextMessage();
            confirmBuilder.addMessageConfirmation(mIter.message());

            ASSERT_EQ(confirmBuilder.messageCount(), 1);

            BMQA_EXPECT_CALL(mockSession, confirmMessages(&confirmBuilder))
                .returning(0);
            int rc = mockSession.confirmMessages(&confirmBuilder);
            ASSERT_EQ(rc, 0);

            ASSERT_EQ(mockSession.unconfirmedMessages(), 2u);
        }

        {
            PVV("Confirm message without builder");

            bmqa::MessageIterator mIter = messageEvent.messageIterator();

            // Get to second message in the event.
            // NOTE: Changing the order of the test cases in this unit test
            //       will break the tests
            mIter.nextMessage();
            mIter.nextMessage();

            BMQA_EXPECT_CALL(mockSession, confirmMessage(mIter.message()))
                .returning(0);
            int rc = mockSession.confirmMessage(mIter.message());
            ASSERT_EQ(rc, 0);
            ASSERT_EQ(mockSession.unconfirmedMessages(), 1u);
        }
    }
}

static void test6_runThrough()
{
    bmqtst::TestHelper::printTestName("RUN THROUGH");

    EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
    handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);

    bmqa::MockSession mockSession(
        handlerMp,
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bdlbb::PooledBlobBufferFactory bufferFactory(
        4 * 1024,
        bmqtst::TestHelperUtil::allocator());
    bmqt::CorrelationId            corrId(1);
    bmqa::QueueId                  queueId(corrId);
    bsl::string                    input = "bmq://my.domain/queue";
    bmqt::Uri                      uri(bmqtst::TestHelperUtil::allocator());
    bsl::string                    error;
    bmqt::UriParser::parse(&uri, &error, input);

    // Test events and results to be emitted
    bmqa::Event testEvent(bmqa::MockSessionUtil::createSessionEvent(
        bmqt::SessionEventType::e_CONNECTED,
        bmqt::CorrelationId(1),
        0,
        "",
        bmqtst::TestHelperUtil::allocator()));

    bmqa::OpenQueueStatus testOpenQueueResult =
        bmqa::MockSessionUtil::createOpenQueueStatus(
            queueId,
            bmqt::OpenQueueResult::e_SUCCESS,
            "",
            bmqtst::TestHelperUtil::allocator());

    bmqa::ConfigureQueueStatus testConfigureQueueResult =
        bmqa::MockSessionUtil::createConfigureQueueStatus(
            queueId,
            bmqt::ConfigureQueueResult::e_SUCCESS,
            "",
            bmqtst::TestHelperUtil::allocator());

    bmqa::CloseQueueStatus testCloseQueueResult =
        bmqa::MockSessionUtil::createCloseQueueStatus(
            queueId,
            bmqt::CloseQueueResult::e_SUCCESS,
            "",
            bmqtst::TestHelperUtil::allocator());

    // Callback setup
    bmqa::MockSession::OpenQueueCallback openQueueCallback =
        bdlf::MemFnUtil::memFn(&EventHandler::onOpenQueueStatus,
                               &eventHandler);

    bmqa::MockSession::ConfigureQueueCallback configureQueueCallback =
        bdlf::MemFnUtil::memFn(&EventHandler::onConfigureQueueResult,
                               &eventHandler);

    bmqa::MockSession::CloseQueueCallback closeQueueCallback(
        bdlf::MemFnUtil::memFn(&EventHandler::onCloseQueueStatus,
                               &eventHandler));

    {
        PVV("Functions requiring BMQA_EXPECT_CALL, rc and event emission");

        BMQA_EXPECT_CALL(mockSession, start())
            .returning(0)
            .emitting(testEvent);
        int rc = mockSession.start();
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, startAsync())
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.startAsync();
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, stop()).emitting(testEvent);
        mockSession.stop();
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, stopAsync()).emitting(testEvent);
        mockSession.stopAsync();
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, finalizeStop()).emitting(testEvent);
        mockSession.finalizeStop();
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, openQueue(&queueId, uri, 0))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.openQueue(&queueId, uri, 0);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, closeQueue(&queueId))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.closeQueue(&queueId);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, openQueueAsync(&queueId, uri, 0))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.openQueueAsync(&queueId, uri, 0);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, closeQueueAsync(&queueId))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.closeQueueAsync(&queueId);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession,
                         closeQueueAsync(&queueId, closeQueueCallback))
            .emitting(testCloseQueueResult);
        mockSession.closeQueueAsync(&queueId, closeQueueCallback);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession,
                         openQueueAsync(&queueId, uri, 0, openQueueCallback))
            .emitting(testOpenQueueResult);
        mockSession.openQueueAsync(&queueId, uri, 0, openQueueCallback);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, configureQueue(&queueId))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.configureQueue(&queueId);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession,
                         configureQueueSync(&queueId,
                                            bmqt::QueueOptions(),
                                            bsls::TimeInterval()))
            .returning(testConfigureQueueResult)
            .emitting(testEvent);
        ASSERT_EQ(mockSession.configureQueueSync(&queueId,
                                                 bmqt::QueueOptions(),
                                                 bsls::TimeInterval()),
                  testConfigureQueueResult);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, configureQueueAsync(&queueId))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.configureQueueAsync(&queueId);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession,
                         configureQueueAsync(&queueId,
                                             bmqt::QueueOptions(),
                                             configureQueueCallback))
            .emitting(testConfigureQueueResult);
        mockSession.configureQueueAsync(&queueId,
                                        bmqt::QueueOptions(),
                                        configureQueueCallback);
        ASSERT_EQ(mockSession.emitEvent(), true);

        BMQA_EXPECT_CALL(mockSession, post(bmqa::MessageEvent()))
            .returning(0)
            .emitting(testEvent);
        rc = mockSession.post(bmqa::MessageEvent());
        ASSERT_EQ(rc, 0);
        ASSERT_EQ(mockSession.emitEvent(), true);
    }

    {
        PVV("Confirm message, rc and event emission");

        const char        msgGUIDHex[] = "00000000000000000000000000000004";
        bmqt::MessageGUID messageGUID;
        messageGUID.fromHex(msgGUIDHex);

        // Create payload
        bdlbb::Blob payload(&bufferFactory,
                            bmqtst::TestHelperUtil::allocator());
        bdlbb::BlobUtil::append(&payload, "hello", 6);

        // Quickly make a valid queue available.
        typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
        QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId);

        implPtr->setState(bmqimp::QueueState::e_OPENED);
        implPtr->setId(1);  // arbitrary id for queue

        // Now create params to build push message
        bsl::vector<bmqa::MockSessionUtil::PushMessageParams> pushParams(
            bmqtst::TestHelperUtil::allocator());
        pushParams.emplace_back(payload,
                                queueId,
                                messageGUID,
                                bmqa::MessageProperties());
        bmqa::MessageEvent pushMsgEvt =
            bmqa::MockSessionUtil::createPushEvent(
                pushParams,
                &bufferFactory,
                bmqtst::TestHelperUtil::allocator())
                .messageEvent();

        bmqa::MessageIterator mIter = pushMsgEvt.messageIterator();

        mIter.nextMessage();
        BMQA_EXPECT_CALL(mockSession, confirmMessage(mIter.message()))
            .returning(0);
        int rc = mockSession.confirmMessage(mIter.message());
        ASSERT_EQ(rc, 0);

        BMQA_EXPECT_CALL(mockSession,
                         confirmMessage(mIter.message().confirmationCookie()))
            .returning(0);
        rc = mockSession.confirmMessage(mIter.message());
        ASSERT_EQ(rc, 0);

        // Create confirm builder and confirm messages
        bmqa::ConfirmEventBuilder confirmBuilder;
        mockSession.loadConfirmEventBuilder(&confirmBuilder);
        rc = confirmBuilder.addMessageConfirmation(mIter.message());
        ASSERT_EQ(rc, 0);
        BMQA_EXPECT_CALL(mockSession, confirmMessages(&confirmBuilder))
            .returning(0);
        rc = mockSession.confirmMessages(&confirmBuilder);
        ASSERT_EQ(rc, 0);
    }

    {
        PVV("Functions not requiring BMQA_EXPECT_CALL");

        bmqa::MessageEventBuilder eventBuilder;
        mockSession.loadMessageEventBuilder(&eventBuilder);

        bmqa::MessageProperties messageProperties;
        mockSession.loadMessageProperties(&messageProperties);

        // 'closeQueueAsync' is called on queue therefore it has not been
        // removed from the two key hash map yet and can still be looked up.
        bmqa::QueueId foundId;
        int           rc = mockSession.getQueueId(&foundId, uri);
        ASSERT_EQ(rc, 0);

        rc = mockSession.getQueueId(&foundId, corrId);
        ASSERT_EQ(rc, 0);
    }

    // Clear the handler since we dont care about the events emitted
    eventHandler.clearEvents();
}

static void test7_postAndAccess()
{
    bmqtst::TestHelper::printTestName("POST AND ACCESS");

    EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
    handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);

    bmqa::MockSession mockSession(
        handlerMp,
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bsl::string input = "bmq://my.domain/queue";
    bmqt::Uri   uri(bmqtst::TestHelperUtil::allocator());
    bsl::string error;

    bmqt::UriParser::parse(&uri, &error, input);

    bmqa::QueueId queueId(bmqt::CorrelationId(1));

    // Open a queue
    EventHandler::openQueue(&mockSession, &queueId, uri);

    // Build payloads
    bdlbb::PooledBlobBufferFactory bufferFactory(
        4 * 1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload1(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload2(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload3(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload4(&bufferFactory, bmqtst::TestHelperUtil::allocator());

    bdlbb::BlobUtil::append(&payload1, "hello!", 7);
    bdlbb::BlobUtil::append(&payload2, "hola!", 6);
    bdlbb::BlobUtil::append(&payload3, "bonjour!", 9);
    bdlbb::BlobUtil::append(&payload4, "namaste!", 9);

    bmqa::MessageEventBuilder builder;
    mockSession.loadMessageEventBuilder(&builder);

    bmqa::Message& bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload1);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload2);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload3);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqa::MessageEvent retrievedPostedEvent;
    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent), false);

    bmqa::MessageEvent postedEvent(builder.messageEvent());
    BMQA_EXPECT_CALL(mockSession, post(builder.messageEvent())).returning(0);
    ASSERT_EQ(mockSession.post(postedEvent), 0);

    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent), true);

    // Please see description of 'compareEvents' for additional details on
    // messageEvent comparison.
    // NOTE: Comparison is implementation specific.
    ASSERT_EQ(EventHandler::compareEvents(retrievedPostedEvent, postedEvent),
              true);

    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent), false);

    // Append another 2 events
    builder.reset();
    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload1);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload2);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload3);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload4);
    ASSERT_EQ(builder.packMessage(queueId), 0);

    bmqa::MessageEvent postedEvent2(builder.messageEvent());
    bmqa::MessageEvent postedEvent3(builder.messageEvent());

    BMQA_EXPECT_CALL(mockSession, post(postedEvent2)).returning(0);
    ASSERT_EQ(mockSession.post(postedEvent2), 0);

    BMQA_EXPECT_CALL(mockSession, post(postedEvent3)).returning(0);
    ASSERT_EQ(mockSession.post(postedEvent3), 0);

    bmqa::MessageEvent retrievedPostedEvent2;
    bmqa::MessageEvent retrievedPostedEvent3;

    // ASSERT that the compare fails for different events
    ASSERT_EQ(EventHandler::compareEvents(retrievedPostedEvent2, postedEvent),
              false);

    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent2), true);
    ASSERT_EQ(EventHandler::compareEvents(retrievedPostedEvent2, postedEvent2),
              true);

    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent3), true);
    ASSERT_EQ(EventHandler::compareEvents(retrievedPostedEvent3, postedEvent3),
              true);

    // We are out of posted messages again.
    ASSERT_EQ(mockSession.popPostedEvent(&retrievedPostedEvent), false);

    eventHandler.clearEvents();

    // Ensure that the builder is clear to ensure that the blob held by the
    // builder is released and destroyed in the correct order.
    builder.reset();
}

static void test8_postBlockedToSuspendedQueue()
{
    bmqtst::TestHelper::printTestName("POST BLOCKED TO SUSPENDED QUEUE");

    EventHandler eventHandler(bmqtst::TestHelperUtil::allocator());

    bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
    handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);

    bmqa::MockSession mockSession(
        handlerMp,
        bmqt::SessionOptions(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    bsl::string input = "bmq://my.domain/queue";
    bmqt::Uri   uri(bmqtst::TestHelperUtil::allocator());
    bsl::string error;

    bmqt::UriParser::parse(&uri, &error, input);

    bmqa::QueueId queueId(bmqt::CorrelationId(1));

    // Open a queue
    EventHandler::openQueue(&mockSession, &queueId, uri);

    // Set the queue to suspended.
    typedef bsl::shared_ptr<bmqimp::Queue>& QueueImplPtr;
    QueueImplPtr implPtr = reinterpret_cast<QueueImplPtr>(queueId);
    implPtr->setIsSuspended(true);

    // Build payload
    bdlbb::PooledBlobBufferFactory bufferFactory(
        4 * 1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob payload(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bdlbb::BlobUtil::append(&payload, "hello!", 7);

    bmqa::MessageEventBuilder builder;
    mockSession.loadMessageEventBuilder(&builder);

    // Ensure that the message cannot be packed.
    bmqa::Message& bmqMessage = builder.startMessage();
    bmqMessage.setDataRef(&payload);
    ASSERT_EQ(builder.packMessage(queueId),
              bmqt::EventBuilderResult::e_QUEUE_SUSPENDED);

    // Unsuspend the queue, and try again.
    implPtr->setIsSuspended(false);
    ASSERT_EQ(builder.packMessage(queueId), 0);
    eventHandler.clearEvents();

    // Ensure that the builder is clear to ensure that the blob held by the
    // builder is released and destroyed in the correct order.
    builder.reset();
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 8: test8_postBlockedToSuspendedQueue(); break;
    case 7: test7_postAndAccess(); break;
    case 6: test6_runThrough(); break;
    case 5: test5_confirmingMessages(); break;
    case 4: test4_queueManagementSync(); break;
    case 3: test3_queueManagement(); break;
    case 2: test2_call(); break;
    case 1: test1_staticMethods(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
}
