// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqa_mocksession.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQA_MOCKSESSION
#define INCLUDED_BMQA_MOCKSESSION

/// @file bmqa_mocksession.h
///
/// @brief Provide a mock session, implementing @bbref{bmqa::AbstractSession}.
///
/// This component provides a mechanism implementing the
/// @bbref{bmqa::AbstractSession} protocol, for mocking a @bbref{bmqa::Session}
/// and can be used to write a test for an application that uses BMQ.  The
/// @bbref{bmqa::MockSession} provides all the methods that
/// @bbref{bmqa::Session} provides, with added methods to specify return codes
/// and emitted events and expected calls.  This can be used to test BlazingMQ
/// application code without a connection to the broker.
/// @bbref{bmqa::MockSessionUtil} is a utility namespace providing useful
/// methods to build @bbref{bmqa::Event} objects that are typically only
/// emitted from the broker.
///
/// The following documentation elucidates the API that this component provides
/// and some simple use cases to get you started.
///
///
/// Disclaimer                                   {#bmqa_mocksession_disclaimer}
/// ==========
///
/// @warning THIS COMPONENT SHOULD ONLY BE USED IN TEST DRIVERS.  IT WILL NOT
/// WORK WITH PRODUCTION CODE.
///
/// Usable Components                                {#bmqa_mocksession_usable}
/// =================
///
///   - **BMQA_EXPECT_CALL**: Macro to specify an expected call to a
///     @bbref{bmqa::MockSession} object. This macro is used to specify which
///     is the next expected call on the @bbref{bmqa::MockSession}.  If an
///     incorrect call is invoked or incorrect parameters are used, an assert
///     will be invoked.
///
///   - **returning**: Specify a return value for the expected call.  This is
///     the value that will be returned when the method on
///     @bbref{bmqa::MockSession} is invoked.
///
///   - **emitting**: Specify an event to be `emitted` when the expected call
///     is invoked.  The events specified are enqueued to the internal event
///     queue and are delivered to the application when `emitEvent` is invoked.
///
/// Static Helper Methods                            {#bmqa_mocksession_helper}
/// =====================
///
///   - **createAckEvent**: Create an acknowledgment message event for messages
///     posted to BMQ.
///
///   - **createPushEvent**: Create a push message event for messages to be
///     consumed from BMQ.
///
///   - **createOpenQueueStatus**: Create an openQueue result (relating to an
///     async open queue operation)
///
///   - **createConfigureQueueStatus**: Create a configureQueue result
///     (relating to an async configure queue operation)
///
///   - **createCloseQueueStatus**: Create a closeQueue result (relating to an
///     async close queue operation)
///
///   - **createSessionEvent**: Create a specified type of session event except
///     for events related to open, close and configure queue.
///
/// The static event builder specified above are typically built inside the
/// broker but are now available to be built in the SDK.  The expected use of
/// such events is to build them and specify them to either the @ref
/// BMQA_EXPECT_CALL macro in the `emitting` parameter, or enqueued to the
/// @bbref{bmqa::MockSession} directly through the `enqueueEvent` method.  They
/// can then be emitted by invoking the `emitEvent` method, which in turn would
/// be processed through the application-provided
/// @bbref{bmqa::SessionEventHandler}.
///
/// Additional Note                              {#bmqa_mocksession_additional}
/// ===============
///
/// @bbref{bmqa::MockSession} does not check if methods have been invoked
/// in the correct order.  The user is responsible for ensuring that the
/// methods are invoked and events enqueued in the correct order.
///
/// The following methods do not emit events:
///
///   - `getQueueId`
///   - `loadMessageEventBuilder`
///   - `loadConfirmEventBuilder`
///   - `loadMessageProperties`
///   - `confirmMessage`
///   - `confirmMessages`
///
/// Calls to the following methods do not require an expect:
///
///   - `getQueueId`
///   - `loadMessageEventBuilder`
///   - `loadConfirmEventBuilder`
///   - `loadMessageProperties`
///
///
/// Creating a mock session in asynchronous mode   {#bmqa_mocksession_creating}
/// ============================================
///
/// The @bbref{bmqa::MockSession} is created in asynchronous mode when a
/// @bbref{bmqa::SessionEventHandler} is provided to it.  If it is not provided
/// a handler, the @bbref{bmqa::MockSession} is started in synchronous mode,
/// requiring the application to call `nextEvent` to access enqueued events.  A
/// sample handler could look like this:
///
/// ```
/// class MyEventHandler : public bmqa::SessionEventHandler {
///
///   private:
///     // DATA
///     bsl::deque<bmqa::SessionEvent>    d_sessionEventsQueue;
///     bsl::deque<bmqa::MessageEvents>   d_messageEventsQueue;
///     bsl::deque<bmqa::OpenQueueStatus> d_openQueueResultsQueue;
///     ...
///
///   public:
///     // MANIPULATORS
///     virtual void onSessionEvent(const bmqa::SessionEvent& event)
///     {
///         bsl::cout << "Received session event " << event << "\n";
///         // some business logic, typically a switch case on
///         // 'bmqt::SessionEventType'
///         d_sessionEventsQueue.push_back(event);
///     }
///
///     virtual void onMessageEvent(const bmqa::MessageEvent& event)
///     {
///         bsl::cout << "Received message event " << event << "\n";
///         // some business logic, typically a switch case on
///         // 'bmqt::MessageEventType'
///         d_messageEventsQueue.push_back(event);
///     }
///
///     void onOpenQueueStatus(const bmqa::OpenQueueStatus& result)
///     {
///         bsl::cout << "Received open queue result: " << result << "\n";
///         // Some business logic
///         d_openQueueResultsQueue.push_back(result);
///     }
///     ...
///
///     bmqa::SessionEvent popSessionEvent()
///     {
///         BSLS_ASSERT(d_sessionEventsQueue.size() > 0);
///         bmqa::SessionEvent ret(d_receivedSessionEvents.front());
///         d_receivedSessionEvents.pop_front();
///         return ret;
///     }
///
///     bmqa::MessageEvent popMessageEvent()
///     {
///         BSLS_ASSERT(d_messageEventsSize.size() > 0);
///         bmqa::MessageEvent ret(d_receivedMessageEvents.front());
///         d_receivedMessageEvents.erase(d_receivedMessageEvents.begin());
///         return ret;
///     }
///
///     bmqa::OpenQueueStatus popOpenQueueStatus()
///     {
///         BSLS_ASSERT(d_openQueueResultsQueue.size() > 0);
///         bmqa::OpenQueueStatus ret(d_openQueueResultsQueue.front());
///         d_openQueueResultsQueue.erase(d_openQueueResultsQueue.begin());
///         return ret;
///     }
///     ...
/// };
/// ```
///
/// Usage                                             {#bmqa_mocksession_usage}
/// =====
///
/// This section illustrates intended use of this component.
///
/// Example 1                                           {#bmqa_mocksession_ex1}
/// ---------
///
/// The folowing example shows a simple producer in asynchronous mode, which
/// will start the session, open a queue, post a message to the queue, generate
/// an ack for that message and finally stop the session (skipping over close
/// queue because it is analogous to opening a queue).  In theory, you can use
/// `emitting` on the @ref BMQA_EXPECT_CALL macro and `enqueueEvent`
/// interchangeably, but in practice it is important to note that events from
/// the broker are generated asynchronously, which means that they are not
/// emitted as you call the method.  You can control emission of events,
/// however, by delaying the call to `emitEvent`.
///
/// @note As with @bbref{bmqa::Session}, calling `nextEvent` is meaningless in
///       asynchronous mode.
///
/// ```
/// void unitTest()
/// {
///     // Create an event handler
///     EventHandler eventHandler(d_allocator_p);
///
///     // The following static initializer method calls all the appropriate
///     // static initializers of the underlying components needed for the
///     // 'MockSession'.  The constructor of 'MockSession' will call it in
///     // any case but if events need to be built outside the scope of the
///     // creation of 'MockSession' you will need to explicitly invoke this
///     // static initializer method.
///     // bmqa::MockSession::initialize(s_allocator_p);
///
///     bslma::ManagedPtr<bmqa::SessionEventHandler> handlerMp;
///     handlerMp.load(&eventHandler, 0, bslma::ManagedPtrUtil::noOpDeleter);
///
///     bmqa::MockSession mockSession(handlerMp,
///                                   bmqt::SessionOptions(d_allocator_p),
///                                   d_allocator_p);
///
///     bmqa::QueueId       queueId(bmqt::CorrelationId(1), d_allocator_p);
///     bmqt::CorrelationId corrId(1);
///
///     // Expect a call to start and the call emits an 'e_CONNECTED' event.
///     BMQA_EXPECT_CALL(mockSession, startAsync())
///         .returning(0)
///         .emitting(bmqa::MockSessionUtil::createSessionEvent(
///                       bmqt::SessionEventType::e_CONNECTED,
///                       0,   // statusCode
///                       "",  // errorDescription
///                       d_allocator_p));
///
///     // Make a call to startAsync and emit the event that is enqueued from
///     // that call.
///     ASSERT_EQ(mockSession.startAsync(), 0);
///
///     // Emit our enqueued event.  This fully sets up the session which is
///     // now ready to use.  Typically you would have some business logic on
///     // 'e_CONNECTED' that makes your application ready to use.
///     ASSERT_EQ(mockSession.emitEvent(), true);
///
///     // Our event handler internally just stores the event emitted, so pop
///     // it out and examine.
///     bmqa::SessionEvent startEvent(eventHandler.popSessionEvent());
///
///     ASSERT_EQ(startEvent.type(), bmqt::SessionEventType::e_CONNECTED);
///     ASSERT_EQ(startEvent.status Code(), 0);
///
///     // Create the uri to your queue as you would in your application.
///     const bmqt::Uri uri("bmq://my.domain/queue");
///
///     // Initialize the queue flags for a producer with acks enabled
///     bsls::Types::Uint64 flags = 0;
///     bmqt::QueueFlagsUtil::setWriter(&flags);
///     bmqt::QueueFlagsUtil::setAck(&flags);
///
///     // We use the macro to expect a call to 'openQueueAsync', binding the
///     // 'uri' and 'queueId' objects as well as the 'flags' that we created.
///     bmqa::MockSession::OpenQueueCallback openQueueCallback =
///         bdlf::BindUtil::bind(&EventHandler::onOpenQueueStatus,
///                              &eventHandler,
///                              bdlf::PlaceHolders::_1); // result
///
///     BMQA_EXPECT_CALL(mockSession,
///                      openQueueAsync(uri1,
///                                     flags,
///                                     openQueueCallback));
///     BMQA_EXPECT_CALL(mockSession,
///                      openQueueAsync(uri, flags, openQueueCallback));
///
///     // Now that we have set our expectations we can try to open the queue.
///     mockSession.openQueueAsync(uri1, flags, openQueueCallback);
///
///     // Since the application may not have direct access to the queue, we
///     // need to get the 'queueId' from the session.  We can then bind this
///     // retrieved 'queueId' to the 'e_QUEUE_OPEN_RESULT' session event and
///     // enqueue it to the 'MockSession'.
///     // Note: You can only get the 'queueId' after 'openQueue' or
///     //       'openQueueAsync' has been invoked on the session.
///     bmqa::QueueId         queueId1(corrId1);
///     bmqa::OpenQueueStatus openQueueResult =
///         bmqa::MockSessionUtil::createOpenQueueStatus(
///                     queueId1,
///                     bmqt::OpenQueueResult::e_TIMEOUT,  // statusCode
///                     "Local Timeout",                   // errorDescription
///                     d_allocator_p);
///     mockSession.enqueueEvent(openQueueResult);
///
///     // We just enqueued a 'bmqa::OpenQueueStatus' to be emitted.  We can
///     // emit it using 'emitEvent'.
///     ASSERT_EQ(mockSession.emitEvent(), true);
///
///     //  Pop out this event from the handler and examine it.
///     bmqa::OpenQueueStatus result = eventHandler.popOpenQueueStatus();
///     ASSERT_EQ(result, openQueueResult);
///
///     // On emission of 'bmqa::OpenQueueStatus', the queue is fully open and
///     // we can now post to it.
///     bmqa::MessageEventBuilder builder;
///     mockSession.loadMessageEventBuilder(&builder);
///
///     BMQA_EXPECT_CALL(mockSession, post(builder.messageEvent()))
///         .returning(0);
///
///     // Use the builder to build a mesage event and pack it for the queue
///     // that has been opened.  If you try to pack the message for an
///     // invalid or closed queue, packing the message will fail. This has
///     // been elided for brevity.
///
///     // Now that the event has been built we can 'post' it to BMQ.
///     ASSERT_EQ(mockSession.post(builder.messageEvent()), 0);
///
///     // Simply creating a blob buffer factory on the stack to be used by
///     // 'createAckEvent'.  Typically you would have one for the component.
///     bdlbb::PooledBlobBufferFactory bufferFactory(4 * 1024, d_allocator_p);
///
///     // The method 'createAckEvent' takes a vector of 'AckParams' to
///     // specify multiple acks per event, but here we are only acknowledging
///     // 1 message.  Specify a positive ack with 'e_SUCCESS' here but you
///     // can specify any from 'bmqt::AckResult::Enum'.
///     bsl::vector<bmqa::MockSessionUtil::AckParams> acks(d_allocator_p);
///     acks.emplace_back(bmqt::AckResult::e_SUCCESS,
///                       bmqt::CorrelationId(1),
///                       bmqt::MessageGUID(), // Real GUID needed if you want
///                                            // to record ack messages.
///                       bmqa::QueueId(1));
///
///     // Enqueuing ack event to be emitted.  We use the helper function
///     // 'createAckEvent' to generate this event.
///     mockSession.enqueueEvent(bmqa::MockSessionUtil::createAckEvent(
///                                                           acks,
///                                                           &bufferFactory,
///                                                           d_allocator_p));
///
///     // Emit the enqueued ack event.
///     ASSERT_EQ(mockSession.emitEvent(), true);
///
///     // As we did earlier, pop it out and examine.
///     bmqa::MessageEvent ackEvent(eventHandler.popMessageEvent());
///     ASSERT_EQ(ackEvent.type(), bmqt::MessageEventType::e_ACK);
///     bmqa::MessageIterator mIter = ackEvent.messageIterator();
///     mIter.nextMessage();
///     ASSERT_EQ(mIter.message().ackStatus(), bmqt::AckResult::e_SUCCESS);
///
///     // This is a simple test.  After posting our message and receiving the
///     // ack, we are now shutting down our application.  Therefore we expect
///     // a 'stopAsync' call.
///     BMQA_EXPECT_CALL(mockSession, stopAsync());
///
///     // Now make a call to 'stopAsync' to stop our session.
///     mockSession.stopAsync();
///
///     // Here we are enqueuing an 'e_DISCONNECTED' event as you would
///     // receive from the broker on a successful shutdown.
///     mockSession.enqueueEvent(bmqa::MockSessionUtil::createSessionEvent(
///                                    bmqt::SessionEventType::e_DISCONNECTED,
///                                    0,   // statusCode
///                                    "",  // errorDescription
///                                    d_allocator_p));
///     ASSERT_EQ(mockSession.emitEvent(), true);
///
///     // Our event handler internally just stores the event emitted, so pop
///     // it out and examine.
///     bmqa::SessionEvent stopEvent(eventHandler.popSessionEvent());
///     ASSERT_EQ(stopEvent.type(), bmqt::SessionEventType::e_DISCONNECTED);
///     ASSERT_EQ(stopEvent.statusCode(), 0);
///
///     // The corresponding pendant operation of the 'initialize' which would
///     // need to be called only if 'initialize' was explicitly called.
///     // bmqa::MockSession::shutdown();
/// }
/// ```
///
/// Example 2                                           {#bmqa_mocksession_ex2}
/// ---------
///
/// The folowing example shows a consumer in synchronous mode, which will start
/// the session, generate a push message (simulating the broker), confirm the
/// message and then stop the session.  Additionally, this test case also sets
/// all expectations up front before running the code, as this is the alternate
/// way of writing your test driver.
///
/// @note Using `enqueue` or `emitEvent` on @bbref{bmqa::MockSession} or
///       `emitting` on the @ref BMQA_EXPECT_CALL macro in synchronous mode is
///       meaningless.
///
///```
/// void unitTest()
/// {
///     // MockSession created without an eventHandler.
///     bmqa::MockSession mockSession(bmqt::SessionOptions(d_allocator_p),
///                                   d_allocator_p);
///
///     // The following static initializer method calls all the appropriate
///     // static initializers of the underlying components needed for the
///     // 'MockSession'.  The constructor of 'MockSession' will call it in
///     // any case but if events need to be built outside the scope of the
///     // creation of 'MockSession' you will need to explicitly invoke this
///     // static initializer method.
///     // bmqa::MockSession::initialize(s_allocator_p);
///
///     // Create simple queueIds and corrIds
///     bmqa::QueueId       queueId(1);
///     bmqt::CorrelationId corrId(1);
///
///     // Create the uri to your queue as you would in your application.
///     bmqt::Uri uri("bmq://my.domain/queue");
///
///     // Expecting that 'startAsync' will be called on the MockSession.
///     BMQA_EXPECT_CALL(mockSession, startAsync())
///         .returning(0);
///
///     // Simply creating a blob buffer factory on the stack to be used by
///     // 'createAckEvent'.  Typically you would have one for the component.
///     bdlbb::PooledBlobBufferFactory bufferFactory(4 * 1024, d_allocator_p);
///
///     // We then expect that 'nextEvent' will be called to return the
///     // 'e_CONNECTED' event from the broker
///     BMQA_EXPECT_CALL(mockSession, nextEvent(bsls::TimeInterval()))
///         .returning(bmqa::MockSessionUtil::createSessionEvent(
///                        bmqt::SessionEventType::e_CONNECTED,
///                        bmqt::CorrelationId::autoValue(),
///                        0,   // errorCode
///                        "",  // errorDescription
///                        d_allocator_p));
///         // Note that we use an 'autoValue' for correlationId because it's
///         // irrelevant for a 'CONNECTED' event.
///
///      // Initialize the queue flags for a consumer
///      bsls::Types::Uint64 flags = 0;
///      bmqt::QueueFlagsUtil::setReader(&flags);
///
///     // We use the macro to expect a call to 'openQueueSync', binding the
///     // 'uri' and 'queueId' objects as well as the flags that we created.
///     // Note that the 'queueId' object will be modified as 'openQueueSync'
///     // takes it as an output parameter.
///     bmqa::OpenQueueStatus expectedResult =
///         bmqa::MockSessionUtil::createOpenQueueStatus(
///                      queueId,
///                      bmqt::OpenQueueResult::e_SUCCESS,  // statusCode
///                      "",                                // errorDescription
///                      d_allocator_p);
///     BMQA_EXPECT_CALL(mockSession, openQueueSync(&queueId, uri, flags))
///         .returning(expectedResult);
///
///     // Build our incoming message event.
///     bsl::vector<bmqa::MockSessionUtil::PushMessageParams> pushMsgs(
///                                                             d_allocator_p);
///     bdlbb::Blob payload(&bufferFactory, d_allocator_p);
///     bdlbb::BlobUtil::append(&payload, "hello", 6);
///
///     const char        guidHex[] = "00000000000000000000000000000001";
///     bmqt::MessageGUID guid;
///     guid.fromHex(guidHex);
///
///     bmqa::MessageProperties properties;
///     mockSession.loadMessageProperties(&properties);
///
///     // For each message that we are supposed to receive from the broker,
///     // we need to specify the payload, the queueId, a guid (the hex is
///     // random but unique within your test driver) and properties which
///     // could be empty.
///     pushMsgs.emplace_back(payload, queueId, guid, properties);
///     bmqa::Event pushMsgEvent = bmqa::MockSessionUtil::createPushEvent(
///                                                             pushMsgs,
///                                                             &bufferFactory,
///                                                             d_allocator_p);
///     BMQA_EXPECT_CALL(mockSession, nextEvent(bsls::TimeInterval()))
///         .returning(pushMsgEvent);
///
///     // Next we expect a call to 'confirmMessages', to confirm the 1 message
///     // that we received from the broker.
///     bmqa::ConfirmEventBuilder confirmBuilder;
///     mockSession.loadConfirmEventBuilder(&confirmBuilder);
///     BMQA_EXPECT_CALL(mockSession, confirmMessages(&confirmBuilder))
///         .returning(0);
///
///     // Expectations have been set up.  Now we run the code.
///     // 'startAsync' is the first call.  We expect it to return 0 and we
///     // expect 'nextEvent' to return the 'e_CONNECTED' session event.
///     int rc = mockSession.startAsync();
///     ASSERT_EQ(rc, 0);
///     bmqa::SessionEvent startEvent = mockSession.nextEvent(
///                                                       bsls::TimeInterval())
///         .sessionEvent();
///     ASSERT_EQ(startEvent.type(), bmqt::SessionEventType::e_CONNECTED);
///     ASSERT_EQ(startEvent.statusCode(),       0);
///     ASSERT_EQ(startEvent.errorDescription(), "");
///
///     // Next we expect a call to 'openQueue' to open the queue.
///     bmqa::OpenQueueStatus result = mockSession.openQueueSync(&queueId,
///                                                              uri,
///                                                              flags);
///     ASSERT_EQ(result, expectedResult);
///
///     // Now our call to 'nextEvent' will generate a push message from the
///     // broker, which we will then go on to confirm.
///     bmqa::MessageEvent pushMsgEvt(mockSession.nextEvent(
///                                                       bsls::TimeInterval())
///                                                           .messageEvent());
///     ASSERT_EQ(pushMsgEvt.type(), bmqt::MessageEventType::e_PUSH);
///
///     // Now that we have received a push message which has yet to be
///     // confirmed, we can confirm that 1 unconfirmed message exists.
///     ASSERT_EQ(mockSession.unconfirmedMessages(), 1U);
///
///     // Since there is only 1 message in our message event, we dont have to
///     // iterate over the event but in reality you will want to iterate over
///     // each message and add it to the confirm builder.
///     bmqa::MessageIterator mIter = pushMsgEvt.messageIterator();
///     mIter.nextMessage();
///     confirmBuilder.addMessageConfirmation(mIter.message());
///     ASSERT_EQ(confirmBuilder.messageCount(), 1);
///
///     // Confirm the messages using the builder that has been populated.
///     rc = mockSession.confirmMessages(&confirmBuilder);
///     ASSERT_EQ(rc, 0);
///
///     // Voila! We now have no unconfirmed messages.
///     ASSERT_EQ(mockSession.unconfirmedMessages(), 0u);
///     // 'stop' has been elided for brevity and is analogous to 'start'
///
///     // The corresponding pendant operation of the 'initialize' which would
///     // need to be called only if 'initialize' was explicitly called.
///     // bmqa::MockSession::shutdown();
/// }
/// ```
///
/// Thread Safety                                    {#bmqa_mocksession_thread}
/// -------------
///
/// THREAD SAFE.

// BMQ
#include <bmqa_abstractsession.h>
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>  // for 'bmqa::SessionEventHandler'

#include <bmqt_queueoptions.h>
#include <bmqt_sessionoptions.h>

// MWC
#include <mwcst_statcontext.h>

// BDE
#include <ball_log.h>
#include <bdlb_variant.h>
#include <bdlbb_blob.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_cstddef.h>
#include <bsl_deque.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_alignedbuffer.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Event;
}
namespace bmqimp {
class MessageCorrelationIdContainer;
}
namespace bmqimp {
struct Stat;
}

namespace bmqa {

// FORWARD DECLARATION
class ConfirmEventBuilder;

// Record an expected method 'CALL' into the specified mock session 'OBJ'.
#define BMQA_EXPECT_CALL(OBJ, CALL)                                           \
    (((OBJ).expect_##CALL).fromLocation(__FILE__, __LINE__))

// ======================
// struct MockSessionUtil
// ======================

/// Utility methods to create `bmqa` events
struct MockSessionUtil {
  private:
    // PRIVATE TYPES
    typedef bsl::shared_ptr<bmqimp::Event> EventImplSp;
    // Event impl shared pointer to access
    // the pimpl of 'bmqa::Event'.

    typedef bsl::shared_ptr<bmqimp::Queue> QueueImplSp;
    // Queue impl shared pointer to access
    // the pimpl of 'bmqa::QueueId'.

  public:
    // PUBLIC TYPES

    /// Struct representing parameters for an ack message.
    struct AckParams {
        // PUBLIC DATA
        bmqt::AckResult::Enum d_status;  // Status code

        bmqt::CorrelationId d_correlationId;
        // Correlation id

        bmqt::MessageGUID d_guid;  // Message GUID of confirmed message

        QueueId d_queueId;
        // Queue id for message being referred
        // to

        // CREATORS

        /// Create a new AckParams object with the specified `status`,
        /// `correlationId`, `guid` and `queueId`.
        AckParams(const bmqt::AckResult::Enum status,
                  const bmqt::CorrelationId&  correlationId,
                  const bmqt::MessageGUID&    guid,
                  const bmqa::QueueId&        queueId);
    };

    // PUBLIC TYPES

    /// Struct representing parameters for a push message.
    struct PushMessageParams {
        // PUBLIC DATA
        bdlbb::Blob d_payload;  // Payload of message

        QueueId d_queueId;  // Queue Id for this message

        bmqt::MessageGUID d_guid;  // GUID for message

        MessageProperties d_properties;  // Optionally specified properties for
                                         // message

        // CREATORS

        /// Create a new PushMessageParams object with the specified
        /// `payload`, `queueId`, `guid` and `properties`.
        PushMessageParams(const bdlbb::Blob&       payload,
                          const bmqa::QueueId&     queueId,
                          const bmqt::MessageGUID& guid,
                          const MessageProperties& properties);
    };

    // CLASS METHODS

    /// Create and return an `Event` configured as a message event of type
    /// `e_ACK` with the specified `acks` params using the specified
    /// `bufferFactory` and the specified `allocator` to supply memory.
    static Event createAckEvent(const bsl::vector<AckParams>& acks,
                                bdlbb::BlobBufferFactory*     bufferFactory,
                                bslma::Allocator*             allocator);

    /// Create and return an `Event` configured as a message event of type
    /// `e_PUSH` and with the specified `pushEventParams`, using the
    /// specified `bufferFactory` and the specified `allocator` to supply
    /// memory.
    static Event
    createPushEvent(const bsl::vector<PushMessageParams>& pushEventParams,
                    bdlbb::BlobBufferFactory*             bufferFactory,
                    bslma::Allocator*                     allocator);

    /// DEPRECATED: Use the `createOpenQueueStatus(...)`,
    ///            `createConfigureQueueStatus(...)`, or
    ///            `createCloseQueueStatus(...)` methods instead.  This
    ///            method will be marked as `BSLS_ANNOTATION_DEPRECATED` in
    ///            future release of libbmq.
    static Event
    createQueueSessionEvent(bmqt::SessionEventType::Enum sessionEventType,
                            QueueId*                     queueId,
                            const bmqt::CorrelationId&   correlationId,
                            int                          errorCode,
                            const bslstl::StringRef&     errorDescription,
                            bslma::Allocator*            allocator);

    /// Create and return an `Event` configured as a session event with the
    /// specified `sessionEventType`, `errorCode` and `errorDescription` and
    /// using the specified `allocator` to supply memory.  Note that this
    /// method will not create queue related session events.
    static Event
    createSessionEvent(bmqt::SessionEventType::Enum sessionEventType,
                       const bmqt::CorrelationId&   correlationId,
                       const int                    errorCode,
                       const bslstl::StringRef&     errorDescription,
                       bslma::Allocator*            allocator);

    /// Create and return a `bmqa::OpenQueueStatus` object with the
    /// specified `queueId`, `statusCode`, and `errorDescription`, using the
    /// optionally specified `allocator` to supply memory in the result.
    static OpenQueueStatus
    createOpenQueueStatus(const QueueId&              queueId,
                          bmqt::OpenQueueResult::Enum statusCode,
                          const bsl::string&          errorDescription,
                          bslma::Allocator*           allocator = 0);

    /// Create and return a `bmqa::ConfigureQueueStatus` object with the
    /// specified `queueId`, `statusCode`, and `errorDescription`, using the
    /// optionally specified `allocator` to supply memory in the result.
    static ConfigureQueueStatus
    createConfigureQueueStatus(const QueueId&                   queueId,
                               bmqt::ConfigureQueueResult::Enum statusCode,
                               const bsl::string& errorDescription,
                               bslma::Allocator*  allocator = 0);

    /// Create and return a `bmqa::CloseQueueStatus` object with the
    /// specified `queueId`, `statusCode`, and `errorDescription`, using the
    /// optionally specified `allocator` to supply memory in the result.
    static CloseQueueStatus
    createCloseQueueStatus(const QueueId&               queueId,
                           bmqt::CloseQueueResult::Enum statusCode,
                           const bsl::string&           errorDescription,
                           bslma::Allocator*            allocator = 0);
};

// =================
// class MockSession
// =================

/// Mechanism to mock a `bmqa::Session`
class MockSession : public AbstractSession {
  public:
    // CLASS METHODS

    /// Perform a one time initialization needed by components used in
    /// `MockSession` and `MockSessionUtil`.  This method only needs to be
    /// called once before any other method, but can be called multiple
    /// times provided that for each call to `initialize` there is a
    /// corresponding call to `shutdown`.  Use the optionally specified
    /// `allocator` for any memory allocation, or the `global` allocator if
    /// none is provided.  Note that specifying the allocator is provided
    /// for test drivers only, and therefore users should let it default to
    /// the global allocator.
    /// NOTE: This method will need to be invoked only if the application
    ///       needs to use `MockSessionUtil` outside the scope of
    ///       `MockSession`.
    static void initialize(bslma::Allocator* allocator = 0);

    /// Pendant operation of the `initialize` one.  The number of calls to
    /// `shutdown` must equal the number of calls to `initialize`, without
    /// corresponding `shutdown` calls, to fully destroy the objects.  It is
    /// safe to call `initialize` after calling `shutdown`.  The behaviour
    /// is undefined if `shutdown` is called without `initialize` first
    /// being called.
    static void shutdown();

    // PUBLIC TYPES

    /// Callback used to inform the test driver of an assertion failure.
    /// The application test driver using the `MockSession` may provide an
    /// implementation that makes the test driver fail (for instance,
    /// calling BDE's test driver ASSERT macro).
    typedef bsl::function<
        void(const char* description, const char* file, int line)>
        FailureCb;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQA.MOCKSESSION");

  private:
    // CONSTANTS

    // Constant representing the maximum size of a `mwcc::TwoKeyHashMap`
    // object, so that the below AlignedBuffer is big enough.  No `mwc`
    // headers can be included in `bmq` public headers, to prevent build
    // time dependency.
    static const int k_MAX_SIZEOF_MWCC_TWOKEYHASHMAP = 256;

    // PRIVATE TYPES

    /// Aligned buffer holding the two key hash map
    typedef bsls::AlignedBuffer<k_MAX_SIZEOF_MWCC_TWOKEYHASHMAP>
        TwoKeyHashMapBuffer;

    /// Cast for bmqimp::Queue to access internal data
    typedef bsl::shared_ptr<bmqimp::Queue> QueueImplSp;

    /// Cast for bmqimp::Event to access internal data
    typedef bsl::shared_ptr<bmqimp::Event> EventImplSp;

    /// Convenience for bmqimp::Stat
    typedef bsl::shared_ptr<bmqimp::Stat> StatImplSp;

    /// Convenience for bmqimp::MessageCorrelationIdContainer
    typedef bsl::shared_ptr<bmqimp::MessageCorrelationIdContainer>
        CorrelationIdContainerSp;

    typedef bsl::function<void()> CallbackFn;

    /// Struct holding attributes associated with an asynchronous queue
    /// operation executed with a user-specified callback
    struct Job {
        // PUBLIC DATA
        CallbackFn d_callback;
        // Signature of a 'void' callback method

        QueueImplSp d_queue;
        // Queue associated with this job

        bmqt::SessionEventType::Enum d_type;
        // Type of queue event associated with
        // this job (OPEN,CONFIGURE, or CLOSE)

        int d_status;
        // Status of the queue operation
    };

    typedef bdlb::Variant<Event, Job> EventOrJob;

    enum Method {
        // Enumeration for the methods in the 'MockSession' protocol.  Each
        // enum value corresponds to a method.
        e_START,
        e_START_ASYNC,
        e_STOP,
        e_STOP_ASYNC,
        e_FINALIZE_STOP,
        e_OPEN_QUEUE,
        e_OPEN_QUEUE_SYNC,
        e_OPEN_QUEUE_ASYNC,
        e_OPEN_QUEUE_ASYNC_CALLBACK,
        e_CLOSE_QUEUE,
        e_CLOSE_QUEUE_SYNC,
        e_CLOSE_QUEUE_ASYNC,
        e_CLOSE_QUEUE_ASYNC_CALLBACK,
        e_CONFIGURE_QUEUE,
        e_CONFIGURE_QUEUE_SYNC,
        e_CONFIGURE_QUEUE_ASYNC,
        e_CONFIGURE_QUEUE_ASYNC_CALLBACK,
        e_NEXT_EVENT,
        e_POST,
        e_CONFIRM_MESSAGE,
        e_CONFIRM_MESSAGES
    };

    struct Call {
        // PUBLIC TYPES
        typedef bsl::vector<EventOrJob> EventsAndJobs;  // Vector of events

        // PUBLIC DATA
        int d_rc;
        // Value to be returned on call

        Method d_method;
        // The type of method

        int d_line;
        // Line number

        bsl::string d_file;
        // File

        bmqt::Uri d_uri;
        // Uri associated with this call

        bsls::Types::Uint64 d_flags;
        // Flags associated with this call

        bmqt::QueueOptions d_queueOptions;
        // QueueOptions associated with this
        // call

        bsls::TimeInterval d_timeout;
        // Timeout interval associated with
        // this call

        OpenQueueCallback d_openQueueCallback;
        // Callback to be invoked upon emission
        // of an async openQueue (if callback
        // was provided)

        ConfigureQueueCallback d_configureQueueCallback;
        // Callback to be invoked upon emission
        // of an async configureQueue (if
        // callback was provided)

        CloseQueueCallback d_closeQueueCallback;
        // Callback to be invoked upon emission
        // of an async closeQueue (if callback
        // was provided)

        bmqa::OpenQueueStatus d_openQueueResult;
        // The result of an open queue
        // operation

        bmqa::ConfigureQueueStatus d_configureQueueResult;
        // The result of a configure queue
        // operation

        bmqa::CloseQueueStatus d_closeQueueResult;
        // The result of a close queue
        // operation

        EventsAndJobs d_emittedEvents;
        // Events to be emitted on this call

        Event d_returnEvent;
        // Event to be returned on this call

        MessageEvent d_messageEvent;
        // MessageEvent associated with this
        // call

        MessageConfirmationCookie d_cookie;
        // MessageConfirmationCookie associated
        // with this call

        bslma::Allocator* d_allocator_p;
        // Allocator

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(Call, bslma::UsesBslmaAllocator)

        // CREATORS

        /// Create a Call object having the specified `method` and the
        /// specified `allocator` to supply memory.
        Call(Method method, bslma::Allocator* allocator);

        Call(const Call& other, bslma::Allocator* allocator);

        // MANIPULATORS

        /// Record the specified `file` and `line` that this call was
        /// created from.
        ///
        /// NOTE: This function is invoked through the macro only.
        Call& fromLocation(const char* file, int line);

        /// Specify the return value for this function call to be the
        /// specified `rc`.
        Call& returning(int rc);

        /// Specify the return value for this function call to be the
        /// specified `result`.  The behavior is undefined unless this call
        /// corresponds to a synchronous open queue.
        Call& returning(const bmqa::OpenQueueStatus& result);

        /// Specify the return value for this function call to be the
        /// specified `result`.  The behavior is undefined unless this call
        /// corresponds to a synchronous configure queue.
        Call& returning(const bmqa::ConfigureQueueStatus& result);

        /// Specify the return value for this function call to be the
        /// specified `result`.  The behavior is undefined unless this call
        /// corresponds to a synchronous close queue.
        Call& returning(const bmqa::CloseQueueStatus& result);

        /// Specify the return value for this function call to be the
        /// specified `event`.
        Call& returning(const Event& event);

        /// Specify the specified `event` to be emitted on this function
        /// call.
        Call& emitting(const Event& event);

        /// Specify the specified `openQueueResult` to be emitted on this
        /// function call.  The behavior is undefined unless the call is an
        /// `openQueueAsync` provided a callback.
        Call& emitting(const OpenQueueStatus& openQueueResult);

        /// Specify the specified `configureQueueResult` to be emitted on
        /// this function call.  The behavior is undefined unless the call
        /// is a `configureQueueResult` provided a callback.
        Call& emitting(const ConfigureQueueStatus& configureQueueResutl);

        /// Specify the specified `closeQueueResult` to be emitted on this
        /// function call.  The behavior is undefined unless the call is a
        /// `closeQueueResult` provided a callback.
        Call& emitting(const CloseQueueStatus& closeQueueResult);

        // ACCESSORS

        /// Get the method name of this call.
        const char* methodName() const;
    };

    /// Queue of expected calls
    typedef bsl::deque<Call> CallQueue;

    /// Queue of posted events
    typedef bsl::deque<bmqa::MessageEvent> PostedEvents;

    // DATA
    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;
    // Buffer factory

    bslma::ManagedPtr<SessionEventHandler> d_eventHandler_mp;
    // Event handler (set only in
    // asynchronous mode)

    mutable CallQueue d_calls;
    // sequence of method calls that are
    // expected

    mutable bsl::deque<EventOrJob> d_eventsAndJobs;
    // events waiting to be emitted

    bsl::unordered_set<bmqt::MessageGUID> d_unconfirmedGUIDs;
    // GUIDS of unconfirmed messages

    TwoKeyHashMapBuffer d_twoKeyHashMapBuffer;
    // Aligned buffer of two key hash map
    // uri, corrid to queues

    FailureCb d_failureCb;
    // Currently installed failure callback
    // that is invoked if methods are
    // called incorrectly or out of order.

    int d_lastQueueId;
    // QueueId

    CorrelationIdContainerSp d_corrIdContainer_sp;
    // Mock correlationId container

    PostedEvents d_postedEvents;
    // Queue of posted events

    mutable bslmt::Mutex d_mutex;
    // protects all public methods

    mwcst::StatContext d_rootStatContext;
    // Top level stat context for this
    // mocked Session.

    StatImplSp d_queuesStats_sp;
    // Stats for all queues

    bmqt::SessionOptions d_sessionOptions;
    // Session Options for current session

    bslma::Allocator* d_allocator_p;
    // Allocator

  private:
    // PRIVATE CLASS METHODS

    /// Get the string representation of the specified `method`.
    static const char* toAscii(const Method method);

    // PRIVATE MANIPULATORS

    /// Configure this component to keep track of statistics.
    void initializeStats();

    /// Open the specified `queueId` with the specified `uri` and `flags`.
    /// If the specified `async` param is true, set the state of the
    /// specified `queueId` to `e_OPENING`, else set it to 'e_OPENED.  This
    /// is to account for the fact that in `openQueue` blocks until the
    /// queue is opened whereas `openQueueAsync` returns immediately and the
    /// state changes to opened on emission of a successfull open queue
    /// response.
    void openQueueImp(QueueId*                  queueId,
                      const bmqt::QueueOptions& options,
                      const bmqt::Uri&          uri,
                      bsls::Types::Uint64       flags,
                      bool                      async);

    /// Modify the queues specified by the `event` if this event is a
    /// session event of type `e_QUEUE_OPEN_RESULT` or
    /// `e_QUEUE_CLOSE_RESULT`.
    void processIfQueueEvent(Event* event);

    /// Modify the queue(s) associated with the specified `job` if this job
    /// is associated with an operation of type `e_QUEUE_OPEN_RESULT` or
    /// `e_QUEUE_CLOSE_RESULT`.
    void processIfQueueJob(Job* job);

    /// Record the messages in the specified `event` as unconfirmed if it is
    /// a push message event.
    void processIfPushEvent(const Event& event);

    // PRIVATE ACCESSORS

    /// Invoke the failure callback because of a wrong call to the specified
    /// `method`.
    void assertWrongCall(const Method method) const;

    /// Invoke the failure callback because of a wrong call to the specified
    /// `method`, while the specified `expectedCall` was expected instead.
    void assertWrongCall(const Method method, const Call& expectedCall) const;

    /// Verify that the specified `actual` value is equal to the specified
    /// `expected` value for argument `argName` (a string) in the call to
    /// `method` (a Method enum) in `call` (a Call).  Invoke the failure
    /// callback if not.
    template <class T, class U>
    void assertWrongArg(const T&     expected,
                        const U&     actual,
                        const Method method,
                        const char*  arg,
                        const Call&  call) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MockSession, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `MockSession` in *synchronous* mode using the
    /// optionally specified `options`.  In such mode, events have to be
    /// fetched by the application using the `nextEvent()` method.
    /// Optionally specify an `allocator` used to supply memory.  If
    /// `allocator` is 0, the currently installed default allocator is used.
    explicit MockSession(
        const bmqt::SessionOptions& options   = bmqt::SessionOptions(),
        bslma::Allocator*           allocator = 0);

    /// Create a `MockSession` in *asynchronous* mode using the specified
    /// `eventHandler` as callback for event processing and the optionally
    /// specified `options`.  Optionally specify an `allocator` used to
    /// supply memory.  If the optionally specified `allocator` is 0, the
    /// currently installed default allocator is used.
    explicit MockSession(
        bslma::ManagedPtr<SessionEventHandler> eventHandler,
        const bmqt::SessionOptions& options   = bmqt::SessionOptions(),
        bslma::Allocator*           allocator = 0);

    /// Stop the `MockSession` and destroy this object.
    ~MockSession() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (specific to 'bmqa::MockSession')

    /// Enqueue the specified `event` in the queue of events to be emitted.
    /// NOTE: This method is unique to `MockSession` and is valid only in
    ///       unit tests.
    void enqueueEvent(const bmqa::Event& event);

    /// Emit the specified number of `numEvents` from the queue of events to
    /// be emitted.  Return true if at least one event was emitted, and
    /// false otherwise.
    /// NOTE: This method is unique to `MockSession` and is valid only in
    ///       unit tests.
    bool emitEvent(int numEvents = 1);

    Call&
    expect_start(const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_startAsync(
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_stop();
    Call& expect_stopAsync();
    Call& expect_finalizeStop();
    Call&
          expect_openQueue(QueueId*                  queueId,
                           const bmqt::Uri&          uri,
                           bsls::Types::Uint64       flags,
                           const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                           const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_openQueueSync(
        QueueId*                  queueId,
        const bmqt::Uri&          uri,
        bsls::Types::Uint64       flags,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    Call& expect_openQueueAsync(
        QueueId*                  queueId,
        const bmqt::Uri&          uri,
        bsls::Types::Uint64       flags,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_openQueueAsync(
        QueueId*                  queueId,
        const bmqt::Uri&          uri,
        bsls::Types::Uint64       flags,
        const OpenQueueCallback&  callback,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    Call& expect_closeQueue(
        QueueId*                  queueId,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_closeQueueSync(
        QueueId*                  queueId,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    Call& expect_closeQueueAsync(
        QueueId*                  queueId,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_closeQueueAsync(
        QueueId*                  queueId,
        const CloseQueueCallback& callback,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_configureQueue(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_configureQueueSync(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_configureQueueAsync(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    Call& expect_configureQueueAsync(
        QueueId*                      queueId,
        const bmqt::QueueOptions&     options,
        const ConfigureQueueCallback& callback,
        const bsls::TimeInterval&     timeout = bsls::TimeInterval());

    Call&
    expect_nextEvent(const bsls::TimeInterval& timeout = bsls::TimeInterval());
    Call& expect_post(const MessageEvent& messageEvent);
    Call& expect_confirmMessage(const Message& message);
    Call& expect_confirmMessage(const MessageConfirmationCookie& cookie);

    /// Expect a call to the function and return a reference offering
    /// modifiable access to the corresponding `Call` object.
    ///
    /// NOTE: Do not use these method directly, use the macro
    ///       `BMQA_EXPECT_CALL` instead.
    Call& expect_confirmMessages(ConfirmEventBuilder* builder);

    // MANIPULATORS
    //   (virtual bmqa::AbstractSession)
    int start(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Start the `MockSession` with an optionally specified `timeout`.  The
    /// return values are elucidated in `bmqt::GenericResult`.  In general a
    /// call to `start` or `startAsync` emits a `SessionEvent` of type
    /// `e_CONNECTED` or `e_CONNECTION_TIMEOUT`.
    int startAsync(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Stop the `MockSession`.  In general a call to `start` or
    /// `startAsync` emits a `SessionEvent` of type `e_DISCONNECTED` or
    /// `e_CONNECTION_TIMEOUT`.
    void stopAsync() BSLS_KEYWORD_OVERRIDE;

    /// This method is only to be used if the session is in synchronous mode
    /// (i.e., not using the EventHandler): it must be called once all
    /// threads getting events with `nextEvent()` have been joined.
    void finalizeStop() BSLS_KEYWORD_OVERRIDE;

    /// Load the `MessageEventBuilder` into the specified `builder` output
    /// parameter.
    void loadMessageEventBuilder(MessageEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE;

    /// Load the `ConfirmEventBuilder` into the specified `builder` output
    /// parameter.
    void loadConfirmEventBuilder(ConfirmEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE;

    /// Load the `MessageProperties` into the specified `buffer` output
    /// parameter.
    void
    loadMessageProperties(MessageProperties* buffer) BSLS_KEYWORD_OVERRIDE;

    /// Queue management
    ///----------------

    /// Load `QueueId` object associated with the specified `uri` into the
    /// specified `queueId` output parameter.
    int getQueueId(QueueId*         queueId,
                   const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE;

    /// Load `QueueId` object associated with the specified `correlationId`
    /// into the specified `queueId` output parameter.
    int getQueueId(QueueId* queueId, const bmqt::CorrelationId& correlationId)
        BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the `openQueueSync(QueueId *queueId...)` instead.
    ///             This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int openQueue(QueueId*                  queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Open the queue having the specified `uri` with the specified `flags`
    /// (a combination of the values defined in `bmqt::QueueFlags::Enum`),
    /// using the specified `queueId` to correlate events related to that
    /// queue.  The object `queueId` referring to is modified, so the
    /// `queueId` represents the actual queue uri, flags and options.
    /// Return a result providing the status and context of the operation.
    /// Use the optionally specified `options` to configure some advanced
    /// settings.  In general, a call to `openQueueSync` emits nothing.
    OpenQueueStatus
    openQueueSync(QueueId*                  queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the `openQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int
    openQueueAsync(QueueId*                  queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously open the queue having the specified `uri` with the
    /// specified `flags` (a combination of the values defined in
    /// `bmqt::QueueFlags::Enum`).  The object `queueId` referring to is
    /// modified, so the `queueId` represents the actual queue uri, flags
    /// and options.  The result of the operation is communicated to the
    /// specified `callback` via a `bmqa::OpenQueueStatus`, providing an
    /// automatically generated correlation `queueId` and the status and
    /// context of the requested operation.  Use the optionally specified
    /// `options` to configure some advanced settings.  In general, a call
    /// to `openQueueAsync` does not emit a `SessionEvent`, but rather
    /// invokes the `callback` (if provided) instead when the corresponding
    /// `emitEvent` is called.
    ///
    /// NOTE: `openQueueAsync` updates the queue state to `e_OPENING` which
    ///       is further updated upon invocation of the `callback` (if
    ///       provided) with a successful `bmqa::OpenQueueStatus`.
    void
    openQueueAsync(QueueId*                  queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const OpenQueueCallback&  callback,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the 'configureQueueSync(QueueId *queueId...)
    ///             instead.  This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int
    configureQueue(QueueId*                  queueId,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the queue identified by the specified `queueId` using the
    /// specified `options` to configure some advanced settings and return a
    /// result providing the status and context of the operation.  In
    /// general, a call to `configureQueueSync` emits nothing.
    ConfigureQueueStatus configureQueueSync(QueueId*                  queueId,
                                            const bmqt::QueueOptions& options,
                                            const bsls::TimeInterval& timeout)
        BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the `configureQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int configureQueueAsync(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options = bmqt::QueueOptions(),
        const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously configure the queue identified by the specified
    /// `queueId` using the specified `options` to configure some advanced
    /// settings.  The result of the operation is communicated to the
    /// specified `callback` via a `bmqa::ConfigureQueueStatus`, providing
    /// the status and context of the requested operation.  In general, a
    /// call to `configureQueueAsync` does not emit a `SessionEvent`, but
    /// rather invokes the `callback` (if provided) instead when the
    /// corresponding `emitEvent` is called.
    void configureQueueAsync(QueueId*                      queueId,
                             const bmqt::QueueOptions&     options,
                             const ConfigureQueueCallback& callback,
                             const bsls::TimeInterval&     timeout =
                                 bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the `closeQueueSync(QueueId *queueId...)` instead.
    ///             This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int closeQueue(QueueId*                  queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Close the queue identified by the specified `queueId` using the
    /// specified `timeout`.  Populate the optionally specified `result`
    /// with the status and context of the operation and return a value
    /// providing the status of the operation.  In general, a call to
    /// `closeQueueSync` emits nothing.
    CloseQueueStatus
    closeQueueSync(QueueId*                  queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// DEPRECATED: Use the `closeQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as
    ///             `BSLS_ANNOTATION_DEPRECATED` in future release of
    ///             libbmq.
    int closeQueueAsync(QueueId*                  queueId,
                        const bsls::TimeInterval& timeout =
                            bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously close the queue identified by the specified `queueId`
    /// using the specified `timeout`.  The result of the operation is
    /// communicated to the specified `callback` via a
    /// `bmqa::CloseQueueStatus`, providing the status and context of the
    /// requested operation.  In general, a call to `closeQueueAsync` does
    /// not emit a `SessionEvent`, but rather invokes the `callback` (if
    /// provided) instead when the corresponding `emitEvent` is called.
    void closeQueueAsync(QueueId*                  queueId,
                         const CloseQueueCallback& callback,
                         const bsls::TimeInterval& timeout =
                             bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// Returns the nextEvent emitted.
    ///
    /// NOTE: This method should only be used when the `MockSession` has
    ///       been created in synchronous mode.  It is invalid if used in
    ///       asynchronous mode and your test case is likely to be faulty
    ///       if used with such a set up.
    Event nextEvent(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Post the specified `messageEvent`.  Return values are defined as per
    /// `bmqt::PostResult`.  In general a call to `post` emits a
    /// `MessageEvent` of type `e_ACK` or no response if acks are not
    /// requested.
    int post(const MessageEvent& messageEvent) BSLS_KEYWORD_OVERRIDE;

    int confirmMessage(const Message& message) BSLS_KEYWORD_OVERRIDE;
    int confirmMessage(const MessageConfirmationCookie& cookie)
        BSLS_KEYWORD_OVERRIDE;

    /// Confirm messages specified by the `message`, `cookie` or `builder`.
    /// Return values are defined as per `bmqt::GenericResult`.  No event is
    /// emitted for calls to `confirmMessage`.
    int confirmMessages(ConfirmEventBuilder* builder) BSLS_KEYWORD_OVERRIDE;

    int configureMessageDumping(const bslstl::StringRef& command)
        BSLS_KEYWORD_OVERRIDE;
    // NOT IMPLEMENTED

    /// Set the failure callback of to be the specified `failureCb`.  This
    /// callback is invoked whenever an expectation set by the test driver
    /// is not met.
    void setFailureCallback(const FailureCb& failureCb);

    /// Load into the specified `event` the next posted event on this
    /// session, if any, and return true; leave `event` unchanged and return
    /// false otherwise.
    bool popPostedEvent(bmqa::MessageEvent* event);

    // ACCESSORS

    /// Get the number of unconfirmed messages.  This corresponds to the
    /// number of push messages enqueued to the session object but not yet
    /// confirmed by the application.
    size_t unconfirmedMessages() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class MockSession
// -----------------

inline void MockSession::setFailureCallback(const FailureCb& failureCb)
{
    d_failureCb = failureCb;
}

inline size_t MockSession::unconfirmedMessages() const
{
    return d_unconfirmedGUIDs.size();
}

inline bool MockSession::popPostedEvent(bmqa::MessageEvent* event)
{
    // PRECONDITIONS
    BSLS_ASSERT(event);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCKED

    if (d_postedEvents.empty()) {
        return false;  // RETURN
    }

    *event = d_postedEvents.front();
    d_postedEvents.pop_front();

    return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif
