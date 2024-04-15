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

// bmqa_session.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQA_SESSION
#define INCLUDED_BMQA_SESSION

//@PURPOSE: Provide access to the BlazingMQ broker.
//
//@CLASSES:
//  bmqa::SessionEventHandler: interface for receiving events asynchronously.
//  bmqa::Session            : mechanism for access to the BlazingMQ broker.
//
//@DESCRIPTION: This component provides a mechanism, 'bmqa::Session', that
// provides access to a message queue broker and an interface,
// 'bmqa::SessionEventHandler' for asynchronous notification of events.  The
// broker manages named, persistent queues of messages.  This broker allows a
// client to open queues, post messages to them, or retrieve messages from
// them.  All of these operations take place within the context of the session
// opened by the client application.
//
// Messages received from a broker are communicated to the application by the
// session associated with that broker in the form of events (see
// 'bmqa_event').  Events can be of two different types: (1) Messages and
// message status events ('bmqa::MessageEvent'), or (2) Session or queue
// status events ('bmqa::SessionEvent').
//
// A 'Session' can dispatch events to the application in either a synchronous
// or asynchronous mode.  In synchronous mode, the application must call the
// 'nextEvent' method in order to obtain events from the 'Session'.  In
// asynchronous mode, the application must supply a concrete
// 'SessionEventHandler' object at construction time.  The concrete
// 'SessionEventHandler' provided by the application must implement the
// 'onSessionEvent' and 'onMessageEvent' methods, which will be called by the
// 'Session' every time a session event or a message event is received.  Note
// that by default, a session created in asynchronous mode creates only one
// internal thread to dispatch events, but a different value for number of
// threads can be specified in 'bmqt::SessionOptions'.
//
// A 'Session' is created either in synchronous or in asynchronous mode, and it
// will remain in that mode until destruction.  Allowing a mix between
// synchronous or asynchronous would make the SDK complicated.  The only
// exceptions are the "start" and "open" operations that must be available in
// synchronous or asynchronous version for the convenience of the programmer.
//
// By default a 'Session' connects to the local broker, which in turn may
// connect to a remote cluster based on configuration.
//
// After a 'bmqa::Session' is started, the application has to open one or
// several queues in read and/or write mode.
//
/// Disclaimer
///----------
// A 'Session' object is a heavy object representing the negotiated TCP session
// with the broker, and the entire associated state (opened queues, statistics,
// ...).  Therefore, sessions should be always reused if possible, preferably
// with only *one* session per lifetime of a component/library/task.
// Note that at the time of this writing multiplexing of different logical
// sessions over the same physical connection is not supported, so in certain
// circumstances reuse of the same session across the whole of a single
// application will not be possible. For example, if an application uses two
// unrelated libraries both of which use BlazingMQ under the hood, they won't
// be able to share a session as it stands.
// An example of an extreme inefficiency and an abuse of resources is to
// create a session ad-hoc every time a message needs to be posted by the same
// component.
//
/// Thread-safety
///-------------
// This session object is *thread* *enabled*, meaning that two threads can
// safely call any methods on the *same* *instance* without external
// synchronization.
//
/// Connecting to the Broker
///------------------------
// A 'Session' establishes a communication with a broker service using TCP/IP.
// Each 'Session' object must be constructed with a 'bmqa::SessionOptions'
// object, which provides the necessary information to connect to the broker.
// In particular, the 'SessionOptions' object must specify the IP address and
// port needed to connect to the broker.  The 'SessionOptions' object may also
// provide extra parameters for tuning the TCP connection behavior (see
// 'bmqa_sessionoptions' for details).
//
// Note that in most cases the user does not need to explicitly construct a
// 'SessionOptions' object: the default constructor for 'SessionOptions'
// creates an instance that will connect to the broker service on the local
// machine using the standard port.
//
// Some options can also be provided using environment variables.
//: o !BMQ_BROKER_URI!: Corresponds to 'SessionOptions::brokerUri'.
//:   If this environment variable is set, its value will override the one
//:   specified in the 'SessionOptions'.
//
// A 'Session' object is created in an unconnected state.  The 'start' or
// 'startAsync' method must be called to connect to the broker.  Note that
// 'start' method is blocking, and returns either after connection to broker
// has been established (success), or after specified timeout (failure).
// 'startAsync' method, as the name suggests, connects to the broker
// asynchronously (i.e., it returns immediately), and the result of the
// operation is notified via 'bmqt::SessionEventType::CONNECTED' session event.
//
// When the 'Session' is no longer needed, the application should call the
// 'stop' (blocking) or 'stopAsync' (non-blocking) method to shut down the
// 'Session' and disconnect from the broker.  Note that destroying a Session
// automatically stops it.  The session can be restarted with a call to 'start'
// or 'startAsync' once it has been fully stopped.
//
/// Connection loss and reconnection
///--------------------------------
// If the connection between the application and the broker is lost, the
// 'Session' will automatically try to reconnect periodically.  The 'Session'
// will also notify the application of the event of losing the connection via
// 'bmqt::SessionEventType::CONNECTION_LOST' session event.
//
// Once the connection has been re-established with the broker (as a result of
// one of the periodic reconnection attempts), the 'Session' will notify the
// application via 'bmqt::SessionEventType::RECONNECTED' session event.  After
// the connection re-establishment, the 'Session' will attempt to reopen the
// queues that were in 'OPEN' state prior to connection loss.  The 'Session'
// will notify the application of the result of reopen operation via
// 'bmqt::SessionEventType::QUEUE_REOPEN_RESULT' for each queue.  Note that a
// reopen operation on a queue may fail (due to broker issue, machine issue,
// etc), so the application must keep track on these session events, and stop
// posting on a queue that failed to reopen.
//
// After all reopen operations are complete and application has been notified
// with all 'bmqt::SessionEventType::QUEUE_REOPEN_RESULT' events, the 'Session'
// delivers a 'bmqt::SessionEventType::STATE_RESTORED' session event to the
// application.
//
/// Example 1
///- - - - -
// The following example illustrates how to create a 'Session' in synchronous
// mode, start it, and stop it.
//..
//  void runSession()
//  {
//      bmqt::SessionOptions options;
//      options.setBrokerUri("tcp://localhost:30114");
//
//      bmqa::Session session(options);
//      int res = session.start();
//      if (0 != res) {
//          bsl::cout << "Failed to start session (" << res << ")"
//                    << bsl::endl;
//          return;
//      }
//      bsl::cout << "Session started." << bsl::endl;
//
//      // Open queue in READ or WRITE or READ/WRITE mode, and receive or
//      // post messages, etc.
//      // ...
//
//      session.stop();
//  }
//..
// This example can be simplified because the constructor for 'Session' uses a
// default 'SessionOptions' object that will connect to the local broker
// service.  The example may be rewritten as follow:
//..
//  void runSession()
//  {
//      bmqa::Session session;     // using default 'SessionOptions'
//      int res = session.start();
//      if (0 != res) {
//          bsl::cout << "Failed to start session (" << res << ")"
//                    << bsl::endl;
//          return;
//      }
//      bsl::cout << "Session started." << bsl::endl;
//
//      // Open queue in READ or WRITE or READ/WRITE mode, and receive or
//      // post messages, etc.
//      // ...
//
//      session.stop();
//  }
//..
//
/// Processing session events - synchronous mode
///--------------------------------------------
// If the 'Session' is created in synchronous mode, the application needs to
// call the 'nextEvent' method on a regular basis in order to receive events.
// This method takes an optional wait timeout as a parameter, and it will
// return the next available 'Event' from the session's internal event queue or
// it will block the calling thread execution until new 'Event' arrives or
// until the specified timeout expires.  It is safe to call the 'nextEvent'
// method from different threads simultaneously: the 'Session' class provides
// proper synchronization logic to protect the internal event queue from
// corruption in this scenario.
//
/// Example 2
///- - - - -
// The following example demonstrates how to write a function that queries and
// processes events synchronously.  In this example the switch form checks the
// type of the 'Event' and performs the necessary actions.
//
// We first define two functions to process 'SessionEvent' and 'MessageEvent'.
// These functions return 'true' if we should keep processing events and
// 'false' otherwise (i.e., no more events are expected from the 'Session').
//..
//  bool processSessionEvent(const bmqa::SessionEvent& event)
//  {
//      bool result = true;
//      switch (event.type()) {
//
//        case bmqt::SessionEventType::e_CONNECTED:
//          // The connection to the broker is established (as a result
//          // of a call to the 'start' method).
//          openQueues();
//          startPostingToQueues();
//          break;
//
//        case bmqt::SessionEventType::e_DISCONNECTED:
//          // The connection to the broker is terminated (as a result
//          // of a call to the 'stop' method).
//          result = false;
//          break;
//
//        case bmqt::SessionEventType::e_CONNECTION_LOST:
//          // The connection to the broker dropped. Stop posting to the queue.
//          stopPostingToQueues();
//          break;
//
//        case bmqt::SessionEventType::e_STATE_RESTORED:
//          // The connection to the broker has been restored (i.e., all queues
//          // have been re-opened. Resume posting to the queue.
//          resumePostingToQueues();
//          break;
//
//        case bmqt::SessionEventType::e_CONNECTION_TIMEOUT:
//          // The connection to the broker has timed out.
//          result = false;
//          break;
//
//        case bmqt::SessionEventType::e_ERROR:
//          // Internal error
//          bsl::cout << "Unexpected session error: "
//                    << event.errorDescription() << bsl::endl;
//          break;
//
//      } // end switch
//
//      return result;
//  }
//
//  bool processMessageEvent(const bmqa::MessageEvent& event)
//  {
//      bool result = true;
//      switch (event.type()) {
//
//        case bmqt::MessageEventType::e_PUSH: {
//          // Received a 'PUSH' event from the broker.
//          bmqa::MessageIterator msgIter = event.messageIterator();
//          while (msgIter.nextMessage()) {
//              const bmqa::Message& msg = msgIter.message();
//              // Process 'PUSH' msg here (omitted for brevity)
//              // ...
//          }
//      } break;
//
//        case bmqt::MessageEventType::e_ACK: {
//          // Received an 'ACK' event from the broker.
//          bmqa::MessageIterator msgIter = event.messageIterator();
//          while (msgIter.nextMessage()) {
//              const bmqa::Message& msg = msgIter.message();
//              // Process 'ACK' msg here (omitted for brevity)
//              // ...
//          }
//      } break;
//
//      } // end switch
//
//      return result;
//  }
//..
//
// Next, we define a function that handles events synchronously using the
// 'processSessionEvent' and 'processMessageEvent' functions.
//..
//  void handleEventsSynchronously(bmqa::Session *startedSession)
//  {
//      bool more = true;
//      while (more) {
//          bmqa::Event event =
//                  startedSession->nextEvent(bsls::TimeInterval(2.0));
//          if (event.isSessionEvent()) {
//              more = processSessionEvent(event.sessionEvent());
//          }
//          else {
//              more = processMessageEvent(event.messageEvent());
//          }
//      }
//  }
//..
//
/// Processing session events - asynchronous mode
///---------------------------------------------
// If application wishes to use 'Session' in asynchronous mode, it must pass a
// managed pointer to an event handler implementing the 'SessionEventHandler'.
// In this case, when 'Session' is started, a thread pool owned by the
// 'Session' is also started for processing events asynchronously.  The
// 'Session' will call event handler's 'onSessionEvent' or 'onMessageEvent'
// method every time a session event or a message event is available.
//
// Note that after the 'Session' is associated with some event handler, this
// association cannot be changed or canceled.  The event handler will be used
// for processing events until the 'Session' object is destroyed.
//
/// Example 3
///- - - - -
// The following example demonstrates how to implement an event handler and how
// to make the 'Session' use an instance of this event handler for processing
// events.
//
// First, we define a concrete implementation of 'SessionEventHandler'.
//
//..
//  class MyHandler: public bmqa::SessionEventHandler {
//  public:
//      MyHandler() { }
//      virtual ~MyHandler() { }
//      virtual void onSessionEvent(const bmqa::SessionEvent& event);
//      virtual void onMessageEvent(const bmqa::MessageEvent& event);
//  };
//
//  void MyHandler::onSessionEvent(const bmqa::SessionEvent& event)
//  {
//      // The implementation is similar to our 'processSessionEvent' function
//      // defined in the previous example.
//      processSessionEvent(event);
//  }
//
//  void MyHandler::onMessageEvent(const bmqa::MessageEvent& event)
//  {
//      // The implementation is similar to our 'processMessageEvent' function
//      // defined in the previous example.
//      processMessageEvent(event);
//  }
//..
// Next, we define a function that creates a 'Session' using our implementation
// of 'SessionEventHandler'.
//..
//  void runAsyncSession()
//  {
//      bslma::ManagedPtr<SessionEventHandler> handlerMp(new MyHandler());
//
//      bmqa::Session session(handlerMp);   // using default 'SessionOptions'
//      int res = session.start();
//      if (0 != res) {
//          bsl::cout << "Failed to start session (" << res << ")"
//                    << bsl::endl;
//          return;
//      }
//
//      // ...
//
//      session.stop();
//  }
//..
//
/// Opening queues
///--------------
// Once the 'Session' has been created and started, the application can use it
// to open queues for producing and/or consuming messages.  A queue is
// associated with a domain.  Domain metadata must be deployed in the BlazingMQ
// infrastructure prior to opening queues under that domain, because opening a
// queue actually loads the metadata deployed for the associated domain.
//
// The metadata associated with a domain defines various parameters like
// maximum queue size and capacity, persistent policy, routing policy, etc.
//
// Queue are identified by URIs (Unified Resource Identifiers) that must
// follow the BlazingMQsyntax, manipulated as 'bmqt::Uri' objects.  A queue URI
// is typically formatted as follows:
//..
//  bmq://my.domain/my.queue
//..
// Note that domain names are unique in BlazingMQ infrastructure, which makes a
// fully qualified queue URI unique too.
//
// Queues in BlazingMQ infrastructure are created by applications on demand.
// Broke creates a queue when it receives an open-queue request from an
// application for a queue that does not exist currently.
//
// Application can open a queue by calling 'openQueue' or 'openQueueAsync'
// method on a started session.  Application must pass appropriate flags to
// indicate if it wants to post messages to queue, consume messages from the
// queue, or both.
//
// Note that 'openQueue' is a blocking method, and returns after specified
// queue has been successfully opened (success) or after specified timeout has
// expired (failure).  'openQueueAsync' method, as the name suggests, is non
// blocking, and the result of the operation is notified via
// 'bmqt::SessionEventType::QUEUE_OPEN_RESULT' session event.
//
/// Example 4
///- - - - -
// The following example demonstrates how to open a queue for posting messages.
// The code first opens the queue with appropriate flags, and then uses
// 'bmqa::MessageEventBuilder' to build a message event and post to the queue.
//..
//  // Session creation and startup logic elided for brevity
//  const char *queueUri = "bmq://my.domain/my.queue";
//  bmqa::QueueId myQueueId(1);       // ID for the queue
//  int rc = session.openQueue(
//                      &myQueueId,
//                      queueUri,
//                      bmqt::QueueFlags::e_WRITE | bmqt::QueueFlags::e_ACK,
//                      bsls::TimeInterval(30, 0));
//
//  if (rc != 0) {
//      bsl::cerr << "Failed to open queue, rc: "
//                << bmqt::OpenQueueResult::Enum(rc)
//                << bsl::endl;
//      return;
//  }
//..
// Note that apart from 'WRITE' flag, 'ACK' flag has been passed to
// 'openQueue' method above.  This indicates that application is interested in
// receiving 'ACK' notification for each message it posts to the queue,
// irrespective of whether or not the message was successfully received by the
// broker and posted to the queue.
//
// Once the queue has been successfully opened for writing, messages can be
// posted to the queue for consumption by interested applications.  We will use
// 'bmqa::MessageEventBuilder' to build a message event.
//..
//  // Create a message event builder
//  bmqa::MessageEventBuilder builder;
//  session.loadMessageEventBuilder(&builder);
//
//  // Create and post a message event containing 1 message
//  bmqa::Message& msg = builder.startMessage();
//
//  msg.setCorrelationId(myCorrelationId);
//  msg.setDataRef(&myPayload);  // where 'myPayload' is of type 'bdlbb::Blob'
//  rc = builder.packMessage(myQueueId);
//  if (rc != 0) {
//      bsl::cerr << "Failed to pack message, rc: "
//                << bmqt::EventBuilderResult::Enum(rc)
//                << bsl::endl;
//      return;
//  }
//
//  // Post message event
//  rc = session.post(builder.messageEvent());
//  if (rc != 0) {
//      bsl::cerr << "Failed to post message event to the queue, rc: "
//                << bmqt::PostResult::Enum(rc)
//                << bsl::endl;
//      return;
//  }
//
//  // ... post more messages
//..
//
/// Closing queues
///--------------
// After an application no longer needs to produce or consume messages from a
// queue, it can be closed by 'closeQueue' or 'closeQueueAsync' method.  Note
// that closing a queue closes an application's "view" on the queue, and may
// not lead to queue deletion in the broker.  A 'Session' does not expose any
// method to explicitly delete a queue.
//
// Note that 'closeQueue' is a blocking method and returns after the specified
// queue has been successfully closed (success) or after specified timeout has
// expired (failure).  'closeQueueAsync', as the name suggests, is a
// non-blocking method, and result of the operation is notified via
// 'bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT' session event.
//
// There are 3 flavors which behave differently with regard to thread blocking
// and callback execution:
///----------------------------------------------------------------------------
//           |  openQueue        |  openQueueSync       |  openQueueAsync
//           |  configureQueue   |  configureQueueSync  |  configureQueueAsync
//           |  closeQueue       |  closeQueueSync      |  closeQueueAsync
//           | (deprecated Sync) | (Synchronous)        | (Asynchronous)
//-----------|-------------------|----------------------|----------------------
//   event   | unblocks in       | unblocks in event    | executes callback in
//  handler  | internal thread   | handler thread  (*)  | event handler thread
//           |                   |                      |
// nextEvent | unblocks in       | unblocks in          | executes callback
//           | internal thread   | internal thread      | in nextEvent thread
//-----------------------------------------------------------------------------
//
// (*) - guarantees unblocking after all previously enqueued events have been
// emitted to the eventHandler, allowing the user to have proper serialization
// of events for the given queue (for example no more PUSH messages will be
// delivered through the eventHandler for the queue after
// configureQueueSync(maxUnconfirmed = 0) returns).

// BMQ

#include <bmqa_abstractsession.h>
#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_confirmeventbuilder.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_openqueuestatus.h>
#include <bmqt_queueoptions.h>
#include <bmqt_sessionoptions.h>
#include <bmqt_uri.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqimp {
class Application;
}
namespace bmqimp {
class Event;
}
namespace bmqp {
class MessageGUIDGenerator;
}
namespace bslmt {
class Semaphore;
}

namespace bmqa {

// FORWARD DECLARATION
class Event;
class Message;
class MessageEvent;
class MessageProperties;
class QueueId;
class SessionEvent;

// =========================
// class SessionEventHandler
// =========================

/// Pure protocol for an asynchronous event handler.  The implementation
/// must be thread safe if the `Session` is configured to use multiple
/// threads.
class SessionEventHandler {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~SessionEventHandler();

    // MANIPULATORS

    /// Process the specified session `event` (connected, disconnected,
    /// queue opened, queue closed, etc.).
    virtual void onSessionEvent(const SessionEvent& event) = 0;

    /// Process the specified message `event` containing one or more
    /// messages.
    virtual void onMessageEvent(const MessageEvent& event) = 0;
};

// ==================
// struct SessionImpl
// ==================

/// Impl structure for the session data members, so that special task such
/// as `bmqadm` can access them by reinterpret casting a `Session` object.
/// Care should be taken though since `Session` is a polymorphic class.
struct SessionImpl {
    // PUBLIC DATA
    bslma::Allocator* d_allocator_p;
    // The allocator to use

    bmqt::SessionOptions d_sessionOptions;
    // Session options as provided by
    // the application.

    bslma::ManagedPtr<SessionEventHandler> d_eventHandler_mp;
    // Event handler, if any, to use
    // for notifying application of
    // events.

    bsl::shared_ptr<bmqp::MessageGUIDGenerator> d_guidGenerator_sp;
    // GUID generator object.

    bslma::ManagedPtr<bmqimp::Application> d_application_mp;
    // The application object.

  private:
    // NOT IMPLEMENTED
    SessionImpl(const SessionImpl&) BSLS_KEYWORD_DELETED;
    SessionImpl& operator=(const SessionImpl&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SessionImpl, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object having the specified `options` and
    /// `eventHandler` and using the optionally specified `allocator`.
    SessionImpl(const bmqt::SessionOptions&            options,
                bslma::ManagedPtr<SessionEventHandler> eventHandler,
                bslma::Allocator*                      allocator = 0);
};

// =============
// class Session
// =============

/// A session with a BlazingMQ broker.
class Session : public AbstractSession {
  public:
    // TYPES

    /// Invoked as a response to an asynchronous open queue operation,
    /// `OpenQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef AbstractSession::OpenQueueCallback OpenQueueCallback;

    /// Invoked as a response to an asynchronous configure queue operation,
    /// `ConfigureQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef AbstractSession::ConfigureQueueCallback ConfigureQueueCallback;

    /// Invoked as a response to an asynchronous close queue operation,
    /// `CloseQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef AbstractSession::CloseQueueCallback CloseQueueCallback;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQA.SESSION");

  private:
    // DATA
    SessionImpl d_impl;  // Sole data member of this object.

  private:
    // NOT IMPLEMENTED
    Session(const Session&) BSLS_KEYWORD_DELETED;

    /// Copy constructor and assignment operator are not implemented
    Session& operator=(const Session&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Session, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `Session` in *synchronous* mode using the optionally
    /// specified `options`.  In such mode, events have to be fetched by the
    /// application using the `nextEvent()` method.  Optionally specify an
    /// `allocator` used to supply memory.  If `allocator` is 0, the
    /// currently installed default allocator is used.
    explicit Session(
        const bmqt::SessionOptions& options   = bmqt::SessionOptions(),
        bslma::Allocator*           allocator = 0);

    /// Create a `Session` in *asynchronous* mode using the specified
    /// `eventHandler` as callback for event processing and the optionally
    /// specified `options`.  Optionally specify an `allocator` used to
    /// supply memory.  If the optionally specified `allocator` is 0, the
    /// currently installed default allocator is used.
    explicit Session(
        bslma::ManagedPtr<SessionEventHandler> eventHandler,
        const bmqt::SessionOptions& options   = bmqt::SessionOptions(),
        bslma::Allocator*           allocator = 0);

    /// Stop the `Session` and destroy this object.
    ~Session() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //   (virtual bmqa::AbstractSession)

    /// Session management
    ///------------------

    /// Connect to the BlazingMQ broker and start the message processing for
    /// this `Session`.  This method blocks until either the `Session` is
    /// connected to the broker, fails to connect, or the operation times
    /// out.  If the optionally specified `timeout` is not populated, use
    /// the one defined in the session options.  Return 0 on success, or a
    /// non-zero value corresponding to the `bmqt::GenericResult::Enum` enum
    /// values otherwise.  The behavior is undefined if this method is
    /// called on an already started `Session`.
    int start(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Connect to the BlazingMQ broker and start the message processing for
    /// this `Session`.  This method returns without blocking.  The result
    /// of the operation is communicated with a session event.  If the
    /// optionally specified `timeout` is not populated, use the one defined
    /// in the session options.  Return 0 on success (this doesn't imply the
    /// session is connected !), or a non-zero value corresponding to the
    /// `bmqt::GenericResult::Enum` enum values otherwise.  The behavior is
    /// undefined if this method is called on an already started `Session`.
    int startAsync(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Gracefully disconnect from the BlazingMQ broker and stop the
    /// operation of this `Session`.  This method blocks waiting for all
    /// already invoked event handlers to exit and all session-related
    /// operations to be finished.  No other method but `start()` may be
    /// used after this method returns.  This method must *NOT* be called if
    /// the session is in synchronous mode (i.e., not using the
    /// EventHandler), `stopAsync()` should be called in this case.
    void stop() BSLS_KEYWORD_OVERRIDE;

    /// Disconnect from the BlazingMQ broker and stop the operation of this
    /// `Session`.  This method returns without blocking and neither enforce
    /// nor waits for any already started session-related operation to be
    /// finished.  No method may be used after this method returns.
    void stopAsync() BSLS_KEYWORD_OVERRIDE;

    /// @deprecated This method is only to be used if the session is in
    /// synchronous mode (i.e., not using the EventHandler): it must be called
    /// once all threads getting events with `nextEvent()` have been joined.
    BSLA_DEPRECATED void finalizeStop() BSLS_KEYWORD_OVERRIDE;

    /// Return a MessageEventBuilder that can be used to build message event
    /// for posting on this session.  The behavior is undefined unless the
    /// session has been successfully started.  Note that lifetime of the
    /// returned object is bound by the lifetime of this session instance
    /// (i.e., returned instance cannot outlive this session instance).
    /// Also note that the `MessageEventBuilder` objects are pooled, so this
    /// operation is cheap, and `MessageEventBuilder` can be obtained on
    /// demand and kept on the stack.
    ///
    /// @deprecated Use the `loadMessageEventBuilder` method instead.
    BSLA_DEPRECATED virtual MessageEventBuilder createMessageEventBuilder();

    /// Load into the specified `builder` an instance of
    /// `bmqa::MessageEventBuilder` that can be used to build message event
    /// for posting on this session.  The behavior is undefined unless the
    /// session has been successfully started and `builder` is non-null.
    /// Note that lifetime of the loaded object is bound by the lifetime of
    /// this session instance (i.e., loaded instance cannot outlive this
    /// session instance).  Also note that the `MessageEventBuilder` objects
    /// are pooled, so this operation is cheap, and `MessageEventBuilder`
    /// can be obtained on demand and kept on the stack.
    void loadMessageEventBuilder(MessageEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `builder` an instance of
    /// `bmqa::ConfirmEventBuilder` that can be used to build a batch of
    /// CONFIRM messages for sending to the broker.  The behavior is
    /// undefined unless the session has been successfully started and
    /// `builder` is non-null.  Note that the lifetime of the loaded object
    /// is bound by the lifetime of this session instance (i.e., loaded
    /// instance cannot outlive this session instance).
    void loadConfirmEventBuilder(ConfirmEventBuilder* builder)
        BSLS_KEYWORD_OVERRIDE;

    /// Load into the specified `buffer` an instance of `MessageProperties`
    /// that can be used to specify and associate properties while building
    /// a `bmqa::Message`.  The behavior is undefined unless the session has
    /// been successfully started and `buffer` is non-null.  Note that
    /// lifetime of the loaded object is bound by the lifetime of this
    /// session instance (i.e., loaded instance cannot outlive this session
    /// instance).
    void
    loadMessageProperties(MessageProperties* buffer) BSLS_KEYWORD_OVERRIDE;

    /// Queue management
    ///----------------

    /// Load in the specified `queueId` the queue corresponding to the
    /// specified `uri` and return 0 if such a queue was found, or leave
    /// `queueId` untouched and return a non-zero value if no queue
    /// corresponding to `uri` is currently open.
    int getQueueId(QueueId*         queueId,
                   const bmqt::Uri& uri) BSLS_KEYWORD_OVERRIDE;

    /// Load in the specified `queueId` the queue corresponding to the
    /// specified `correlationId` and return 0 if such a queue was found, or
    /// leave `queueId` untouched and return a non-zero value if no queue
    /// corresponding to `correlationId` is currently open.
    int getQueueId(QueueId* queueId, const bmqt::CorrelationId& correlationId)
        BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `openQueueSync(QueueId *queueId...)` method
    /// instead.
    BSLA_DEPRECATED int
    openQueue(QueueId*                  queueId,
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
    /// settings.  Note that this operation fails if `queueId` is
    /// non-unique.  If the optionally specified `timeout` is not populated,
    /// use the one defined in the session options.  This operation will
    /// block until either success, failure, or timing out happens.
    ///
    /// THREAD: Note that calling this method from the event processing
    ///         thread(s) (i.e., from the EventHandler callback, if
    ///         provided) *WILL* lead to a *DEADLOCK*.
    OpenQueueStatus
    openQueueSync(QueueId*                  queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `openQueue(QueueId *queueId...)` method instead.
    BSLA_DEPRECATED virtual int
    openQueue(const QueueId&            queueId,
              const bmqt::Uri&          uri,
              bsls::Types::Uint64       flags,
              const bmqt::QueueOptions& options = bmqt::QueueOptions(),
              const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// @deprecated Use the `openQueueAsync(...)` method with callback flavor
    /// instead.
    BSLA_DEPRECATED int
    openQueueAsync(QueueId*                  queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously open the queue having the specified `uri` with the
    /// specified `flags` (a combination of the values defined in
    /// `bmqt::QueueFlags::Enum`), using the specified `queueId` to
    /// correlate events related to that queue and the optionally specified
    /// `options` to configure some advanced settings.  The object `queueId`
    /// referring to is modified, so the `queueId` represents the actual
    /// queue uri, flags and options.  The result of the operation is
    /// communicated to the specified `callback` via a
    /// `bmqa::OpenQueueStatus`, providing the status and context of the
    /// requested operation.  Note that this operation fails if `queueId` is
    /// non-unique.  If the optionally specified `timeout` is not populated,
    /// use the one defined in the session options.
    ///
    /// THREAD: The `callback` will *ALWAYS* be invoked from the
    ///         EventHandler thread(s) (or if a SessionEventHandler was not
    ///         specified, from the thread invoking `nextEvent`).
    void
    openQueueAsync(QueueId*                  queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const OpenQueueCallback&  callback,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `configureQueueSync(QueueId *queueId...)` method
    /// instead.
    BSLA_DEPRECATED int
    configureQueue(QueueId*                  queueId,
                   const bmqt::QueueOptions& options,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Configure the queue identified by the specified `queueId` using the
    /// specified `options` and return a result providing the status and
    /// context of the operation.  If the optionally specified `timeout` is
    /// not populated, use the one defined in the session options.  This
    /// operation returns error if there is a pending configure for the same
    /// queue.  This operation will block until either success, failure, or
    /// timing out happens.
    ///
    /// Note that the following `bmqt::QueueOptions` fields cannot be
    /// reconfigured after the queue has been opened:
    ///   - suspendsOnBadHostHealth
    /// Attempts to reconfigure these fields will yield an `e_NOT_SUPPORTED`
    /// error code.
    ///
    /// THREAD: Note that calling this method from the event processing
    ///         thread(s) (i.e., from the EventHandler callback, if
    ///         provided) *WILL* lead to a *DEADLOCK*.
    ConfigureQueueStatus
    configureQueueSync(QueueId*                  queueId,
                       const bmqt::QueueOptions& options,
                       const bsls::TimeInterval& timeout =
                           bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `configureQueueAsync(...)` method with callback
    /// flavor instead.
    BSLA_DEPRECATED int
    configureQueueAsync(QueueId*                  queueId,
                        const bmqt::QueueOptions& options,
                        const bsls::TimeInterval& timeout =
                            bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously configure the queue identified by the specified
    /// `queueId` using the specified `options` to configure some advanced
    /// settings.  The result of the operation is communicated to the
    /// specified `callback` via a `bmqa::ConfigureQueueStatus`, providing
    /// the status and context of the requested operation.  If the
    /// optionally specified `timeout` is not populated, use the one defined
    /// in the session options.
    ///
    /// Note that the following `bmqt::QueueOptions` fields cannot be
    /// reconfigured after the queue has been opened:
    ///   - suspendsOnBadHostHealth
    /// Attempts to reconfigure these fields will yield an `e_NOT_SUPPORTED`
    /// error code.
    ///
    /// THREAD: The `callback` will *ALWAYS* be invoked from the
    ///         EventHandler thread(s) (or if a SessionEventHandler was not
    ///         specified, from the thread invoking `nextEvent`).
    void configureQueueAsync(QueueId*                      queueId,
                             const bmqt::QueueOptions&     options,
                             const ConfigureQueueCallback& callback,
                             const bsls::TimeInterval&     timeout =
                                 bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `closeQueueSync(QueueId *queueId...)` method
    /// instead.
    BSLA_DEPRECATED int
    closeQueue(QueueId*                  queueId,
               const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Close the queue identified by the specified `queueId` and return a
    /// result providing the status and context of the operation.  If the
    /// optionally specified `timeout` is not populated, use the one defined
    /// in the session options.  Any outstanding configureQueue request for
    /// this `queueId` will be canceled.  This operation will block until
    /// either success, failure, or timing out happens.  Once this method
    /// returns, there is guarantee that no more messages and events for
    /// this `queueId` will be received.  Note that successful processing of
    /// this request in the broker closes this session's view of the queue;
    /// the underlying queue may not be deleted in the broker.  When this
    /// method returns, the correlationId associated to the queue is
    /// cleared.
    ///
    /// THREAD: Note that calling this method from the event processing
    ///         thread(s) (i.e., from the EventHandler callback, if
    ///         provided) *WILL* lead to a *DEADLOCK*.
    CloseQueueStatus
    closeQueueSync(QueueId*                  queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `closeQueue(QueueId *queueId...)` method instead.
    BSLA_DEPRECATED virtual int
    closeQueue(const QueueId&            queueId,
               const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// @deprecated Use the `closeQueueAsync(...)` method with callback flavor
    /// instead.
    BSLA_DEPRECATED int
    closeQueueAsync(QueueId*                  queueId,
                    const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously close the queue identified by the specified
    /// `queueId`.  Any outstanding configureQueue requests will be
    /// canceled.  The result of the operation is communicated to the
    /// specified `callback` via a `bmqa::CloseQueueStatus`, providing the
    /// status and context of the operation.  If the optionally specified
    /// `timeout` is not populated, use the one defined in the session
    /// options.  Note that successful processing of this request in the
    /// broker closes this session's view of the queue; the underlying queue
    /// may not be deleted in the broker.  The correlationId associated to
    /// the queue remains valid until the `bmqa::CloseQueueStatus` result
    /// has been received and processed by the `callback`, after which it
    /// will be cleared and no more messages and events for this `queueId`
    /// will be received.
    ///
    /// THREAD: The `callback` will *ALWAYS* be invoked from the
    ///         EventHandler thread(s) (or if a SessionEventHandler was not
    ///         specified, from the thread invoking `nextEvent`).
    void closeQueueAsync(QueueId*                  queueId,
                         const CloseQueueCallback& callback,
                         const bsls::TimeInterval& timeout =
                             bsls::TimeInterval()) BSLS_KEYWORD_OVERRIDE;

    /// @deprecated Use the `closeQueueAsync(...)` method with callback flavor
    /// instead.
    BSLA_DEPRECATED virtual int
    closeQueueAsync(const QueueId&            queueId,
                    const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// Queue manipulation
    ///------------------

    /// Return the next available event received for this session.  If there
    /// is no event available, this method blocks for up to the optionally
    /// specified `timeout` time interval for an event to arrive.  An empty
    /// time interval for `timeout` (the default) indicates that the method
    /// should not timeout (the method will not return until the next event
    /// is available).  Return a `bmqa::SessionEvent` of type
    /// `bmqt::SessionEventType::e_TIMEOUT` if a timeout was specified and
    /// that timeout expired before any event was received.  Note that this
    /// method can only be used if the session is in synchronous mode (ie
    /// not using the EventHandler).  The behavior is undefined unless the
    /// session was started.
    Event nextEvent(const bsls::TimeInterval& timeout = bsls::TimeInterval())
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously post the specified `event` that must contain one or
    /// more `Messages`.  The return value is one of the values defined in
    /// the `bmqt::PostResult::Enum` enum.  Return zero on success and a
    /// non-zero value otherwise.  Note that success implies that SDK has
    /// accepted the `event` and will eventually deliver it to the broker.
    /// The behavior is undefined unless the session was started.
    int post(const MessageEvent& event) BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously confirm the receipt of the specified `message`.  This
    /// indicates that the application is done processing the message and
    /// that the broker can safely discard it from the queue according to
    /// the retention policy set up for that queue.  Return 0 on success,
    /// and a non-zero value otherwise.  Note that success implies that SDK
    /// has accepted the `message` and will eventually send confirmation
    /// notification to the broker.
    int confirmMessage(const Message& message) BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously confirm the receipt of the message with which the
    /// specified `cookie` is associated.  This indicates that the
    /// application is done processing the message and that the broker can
    /// safely discard it from the queue according to the retention policy
    /// set up for that queue.  Return 0 on success, and a non-zero value
    /// otherwise.  Note that success implies that SDK has accepted the
    /// `message` and will eventually send confirmation notification to the
    /// broker.
    int confirmMessage(const MessageConfirmationCookie& cookie)
        BSLS_KEYWORD_OVERRIDE;

    /// Asynchronously confirm the receipt of the batch of CONFIRM messages
    /// contained in the specified `builder`.  This indicates that the
    /// application is done processing all of the messages and that the
    /// broker can safely discard them from the queue according to the
    /// retention policy set up for that queue.  The return value is one of
    /// the values defined in the `bmqt::GenericResult::Enum` enum.  Note
    /// that in case of success, the instance pointed by the `builder` will
    /// be reset.  Also note that success implies that SDK has accepted all
    /// of the messages in `builder` and will eventually send confirmation
    /// notification to the broker, whereas failure implies that none of the
    /// messages in `builder` were accepted.  Behavior is undefined unless
    /// `builder` is non-null.
    int confirmMessages(ConfirmEventBuilder* builder) BSLS_KEYWORD_OVERRIDE;

    /// Debugging related
    ///-----------------

    /// Configure this session instance to dump messages to the installed
    /// logger at `ball::Severity::INFO` level according to the specified
    /// `command` that should adhere to the following pattern:
    /// ```
    ///  IN|OUT|PUSH|ACK|PUT|CONFIRM ON|OFF|100|10s
    /// ```
    /// where each token has a specific meaning:
    /// * **IN**      : incoming (`PUSH` and `ACK`) events
    /// * **OUT**     : outgoing (`PUT` and `CONFIRM`) events
    /// * **PUSH**    : incoming `PUSH` events
    /// * **ACK**     : incoming `ACK` events
    /// * **PUT**     : outgoing `PUT` events
    /// * **CONFIRM** : outgoing `CONFIRM` events
    /// * **ON**      : turn on message dumping until explicitly turned off
    /// * **OFF**     : turn off message dumping
    /// * **100**     : turn on message dumping for the next 100 messages
    /// * **10s**     : turn on message dumping for the next 10 seconds
    /// Above, the numerical values `100` and `10` are just for illustration
    /// purposes, and application can choose an appropriate positive numeric
    /// value for them.  Also, pattern is case-insensitive.  Return zero if
    /// `command` is valid and message dumping has been configured, non-zero
    /// value otherwise.  The behavior is undefined unless the session has
    /// been started.
    int configureMessageDumping(const bslstl::StringRef& command)
        BSLS_KEYWORD_OVERRIDE;

    /// Internal
    ///--------

    /// Do *NOT* use.  Internal function, reserved for BlazingMQ internal
    /// usage.
    void* impl();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class Session
// -------------

inline int Session::confirmMessage(const Message& message)
{
    return confirmMessage(message.confirmationCookie());
}

inline void* Session::impl()
{
    return &d_impl;
}

}  // close package namespace
}  // close enterprise namespace

#endif
