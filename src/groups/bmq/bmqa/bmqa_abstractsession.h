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

// bmqa_abstractsession.h                                             -*-C++-*-
#ifndef INCLUDED_BMQA_ABSTRACTSESSION
#define INCLUDED_BMQA_ABSTRACTSESSION

/// @file bmqa_abstractsession.h
///
/// @brief Provide a pure protocol for a BlazingMQ session.
///
/// @bbref{bmqa::AbstractSession} is a pure protocol for a BlazingMQ session.

#include <bmqa_closequeuestatus.h>
#include <bmqa_configurequeuestatus.h>
#include <bmqa_confirmeventbuilder.h>
#include <bmqa_event.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageproperties.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqt_queueoptions.h>
#include <bmqt_uri.h>

// BDE
#include <bsl_functional.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqa {

// =====================
// class AbstractSession
// =====================

/// A pure protocol for a session.
class AbstractSession {
  public:
    // TYPES

    /// Invoked as a response to an asynchronous open queue operation,
    /// `OpenQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef bsl::function<void(const bmqa::OpenQueueStatus& result)>
        OpenQueueCallback;

    /// Invoked as a response to an asynchronous configure queue operation,
    /// `ConfigureQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef bsl::function<void(const bmqa::ConfigureQueueStatus& result)>
        ConfigureQueueCallback;

    /// Invoked as a response to an asynchronous close queue operation,
    /// `CloseQueueCallback` is an alias for a callback function object
    /// (functor) that takes as an argument the specified `result`,
    /// providing the result and context of the requested operation.
    typedef bsl::function<void(const bmqa::CloseQueueStatus& result)>
        CloseQueueCallback;

  public:
    // CREATORS

    /// Destructor
    virtual ~AbstractSession();

    // MANIPULATORS

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
    virtual int
    start(const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// Connect to the BlazingMQ broker and start the message processing for
    /// this `Session`.  This method returns without blocking.  The result
    /// of the operation is communicated with a session event.  If the
    /// optionally specified `timeout` is not populated, use the one defined
    /// in the session options.  Return 0 on success (this doesn't imply the
    /// session is connected !), or a non-zero value corresponding to the
    /// `bmqt::GenericResult::Enum` enum values otherwise.  The behavior is
    /// undefined if this method is called on an already started `Session`.
    virtual int
    startAsync(const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// Gracefully disconnect from the BlazingMQ broker and stop the
    /// operation of this `Session`.  This method blocks waiting for all
    /// already invoked event handlers to exit and all session-related
    /// operations to be finished.  No other method but `start()` may be
    /// used after this method returns.  This method must *NOT* be called if
    /// the session is in synchronous mode (i.e., not using the
    /// EventHandler), `stopAsync()` should be called in this case.
    virtual void stop();

    /// Disconnect from the BlazingMQ broker and stop the operation of this
    /// `Session`.  This method returns without blocking and neither enforce
    /// nor waits for any already started session-related operation to be
    /// finished.  No method may be used after this method returns.
    virtual void stopAsync();

    /// **DEPRECATED**
    ///
    /// This method is only to be used if the session is in synchronous mode
    /// (i.e., not using the EventHandler): it must be called once all
    /// threads getting events with `nextEvent()` have been joined.
    virtual void finalizeStop();

    /// Load into the specified `builder` an instance of
    /// `bmqa::MessageEventBuilder` that can be used to build message event
    /// for posting on this session.  The behavior is undefined unless the
    /// session has been successfully started and `builder` is non-null.
    /// Note that lifetime of the loaded object is bound by the lifetime of
    /// this session instance (i.e., loaded instance cannot outlive this
    /// session instance).  Also note that the `MessageEventBuilder` objects
    /// are pooled, so this operation is cheap, and `MessageEventBuilder`
    /// can be obtained on demand and kept on the stack.
    virtual void loadMessageEventBuilder(MessageEventBuilder* builder);

    /// Load into the specified `builder` an instance of
    /// `bmqa::ConfirmEventBuilder` that can be used to build a batch of
    /// CONFIRM messages for sending to the broker.  The behavior is
    /// undefined unless the session has been successfully started and
    /// `builder` is non-null.  Note that the lifetime of the loaded object
    /// is bound by the lifetime of this session instance (i.e., loaded
    /// instance cannot outlive this session instance).
    virtual void loadConfirmEventBuilder(ConfirmEventBuilder* builder);

    /// Load into the specified `buffer` an instance of `MessageProperties`
    /// that can be used to specify and associate properties while building
    /// a `bmqa::Message`.  The behavior is undefined unless the session has
    /// been successfully started and `buffer` is non-null.  Note that
    /// lifetime of the loaded object is bound by the lifetime of this
    /// session instance (i.e., loaded instance cannot outlive this session
    /// instance).
    virtual void loadMessageProperties(MessageProperties* buffer);

    /// Queue management
    ///----------------

    /// Load in the specified `queueId` the queue corresponding to the
    /// specified `uri` and return 0 if such a queue was found, or leave
    /// `queueId` untouched and return a non-zero value if no queue
    /// corresponding to `uri` is currently open.
    virtual int getQueueId(QueueId* queueId, const bmqt::Uri& uri);

    /// Load in the specified `queueId` the queue corresponding to the
    /// specified `correlationId` and return 0 if such a queue was found, or
    /// leave `queueId` untouched and return a non-zero value if no queue
    /// corresponding to `correlationId` is currently open.
    virtual int getQueueId(QueueId*                   queueId,
                           const bmqt::CorrelationId& correlationId);

    /// DEPRECATED: Use the `openQueueSync(QueueId *queueId...)` instead.
    ///             This method will be marked as `BSLA_DEPRECATED` in future
    ///             release of libbmq.
    virtual int
    openQueue(QueueId*                  queueId,
              const bmqt::Uri&          uri,
              bsls::Types::Uint64       flags,
              const bmqt::QueueOptions& options = bmqt::QueueOptions(),
              const bsls::TimeInterval& timeout = bsls::TimeInterval());

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
    virtual OpenQueueStatus
    openQueueSync(QueueId*                  queueId,
                  const bmqt::Uri&          uri,
                  bsls::Types::Uint64       flags,
                  const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                  const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// DEPRECATED: Use the `openQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as `BSLA_DEPRECATED`
    ///             in future release of libbmq.
    virtual int
    openQueueAsync(QueueId*                  queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval());

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
    virtual void
    openQueueAsync(bmqa::QueueId*            queueId,
                   const bmqt::Uri&          uri,
                   bsls::Types::Uint64       flags,
                   const OpenQueueCallback&  callback,
                   const bmqt::QueueOptions& options = bmqt::QueueOptions(),
                   const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// DEPRECATED: Use the `configureQueueSync(QueueId *queueId...)`
    ///             instead.  This method will be marked as `BSLA_DEPRECATED`
    ///             in future release of libbmq.
    virtual int
    configureQueue(QueueId*                  queueId,
                   const bmqt::QueueOptions& options,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// Configure the queue identified by the specified `queueId` using the
    /// specified `options` and return a result providing the status and
    /// context of the operation.  Fields from `options` that have not been
    /// explicitly set will not be modified.  If the optionally specified
    /// `timeout` is not populated, use the one defined in the session
    /// options.  This operation returns error if there is a pending
    /// configure for the same queue.  This operation will block until
    /// either success, failure, or timing out happens.
    ///
    /// THREAD: Note that calling this method from the event processing
    ///         thread(s) (i.e., from the EventHandler callback, if
    ///         provided) *WILL* lead to a *DEADLOCK*.
    virtual ConfigureQueueStatus configureQueueSync(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// DEPRECATED: Use the `configureQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as `BSLA_DEPRECATED`
    ///             in future release of libbmq.
    virtual int configureQueueAsync(
        QueueId*                  queueId,
        const bmqt::QueueOptions& options,
        const bsls::TimeInterval& timeout = bsls::TimeInterval());

    virtual void configureQueueAsync(
        QueueId*                      queueId,
        const bmqt::QueueOptions&     options,
        const ConfigureQueueCallback& callback,
        const bsls::TimeInterval&     timeout = bsls::TimeInterval());

    /// Asynchronously configure the queue identified by the specified
    /// 'queueId' using the specified 'options' to configure some advanced
    /// settings.  The result of the operation is communicated to the
    /// specified 'callback' via a 'bmqa::ConfigureQueueStatus', providing
    /// the status and context of the requested operation.  If the
    /// optionally specified 'timeout' is not populated, use the one defined
    /// in the session options.
    ///
    /// THREAD: The 'callback' will *ALWAYS* be invoked from the
    ///         EventHandler thread(s) (or if a SessionEventHandler was not
    ///         specified, from the thread invoking 'nextEvent').
    ///
    /// DEPRECATED: Use the `closeQueueSync(QueueId *queueId...)` instead.
    ///             This method will be marked as `BSLA_DEPRECATED` in future
    ///             release of libbmq.
    virtual int
    closeQueue(QueueId*                  queueId,
               const bsls::TimeInterval& timeout = bsls::TimeInterval());

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
    virtual CloseQueueStatus
    closeQueueSync(QueueId*                  queueId,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// DEPRECATED: Use the `closeQueueAsync(...)` with callback flavor
    ///             instead.  This method will be marked as `BSLA_DEPRECATED`
    ///             in future release of libbmq.
    virtual int
    closeQueueAsync(QueueId*                  queueId,
                    const bsls::TimeInterval& timeout = bsls::TimeInterval());

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
    virtual void
    closeQueueAsync(QueueId*                  queueId,
                    const CloseQueueCallback& callback,
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
    virtual Event
    nextEvent(const bsls::TimeInterval& timeout = bsls::TimeInterval());

    /// Asynchronously post the specified `event` that must contain one or
    /// more `Messages`.  The return value is one of the values defined in
    /// the `bmqt::PostResult::Enum` enum.  Return zero on success and a
    /// non-zero value otherwise.  Note that success implies that SDK has
    /// accepted the `event` and will eventually deliver it to the broker.
    /// The behavior is undefined unless the session was started.
    virtual int post(const MessageEvent& event);

    /// Asynchronously confirm the receipt of the specified `message`.  This
    /// indicates that the application is done processing the message and
    /// that the broker can safely discard it from the queue according to
    /// the retention policy set up for that queue.  Return 0 on success,
    /// and a non-zero value otherwise.  Note that success implies that SDK
    /// has accepted the `message` and will eventually send confirmation
    /// notification to the broker.
    virtual int confirmMessage(const Message& message);

    /// Asynchronously confirm the receipt of the message with which the
    /// specified `cookie` is associated.  This indicates that the
    /// application is done processing the message and that the broker can
    /// safely discard it from the queue according to the retention policy
    /// set up for that queue.  Return 0 on success, and a non-zero value
    /// otherwise.  Note that success implies that SDK has accepted the
    /// `message` and will eventually send confirmation notification to the
    /// broker.
    virtual int confirmMessage(const MessageConfirmationCookie& cookie);

    /// Asynchronously confirm the receipt of the batch of CONFIRM messages
    /// contained in the specified `builder`.  This indicates that the
    /// application is done processing all of the messages and that the
    /// broker can safely discard them from the queue according to the
    /// retention policy set up for that queue.  Return 0 on success, and a
    /// non-zero value otherwise.  Note that in case of success, the
    /// instance pointed by the `builder` will be reset.  Also note that
    /// success implies that SDK has accepted all of the messages in
    /// `builder` and will eventually send confirmation notification to the
    /// broker.  Behavior is undefined unless `builder` is non-null.
    virtual int confirmMessages(ConfirmEventBuilder* builder);

    /// Debugging related
    ///-----------------

    /// Configure this session instance to dump messages to the installed
    /// logger at `ball::Severity::e_INFO` level according to the specified
    /// `command` that should adhere to the following pattern:
    /// ```
    ///  IN|OUT ON|OFF|100|10s
    /// ```
    /// where each token has a specific meaning:
    /// * **IN**  : incoming (`PUSH` and `ACK`) events
    /// * **OUT** : outgoing (`PUT` and `CONFIRM`) events
    /// * **ON**  : turn on message dumping until explicitly turned off
    /// * **OFF** : turn off message dumping
    /// * **100** : turn on message dumping for the next 100 messages
    /// * **10s** : turn on message dumping for the next 10 seconds
    /// Note that above, `100` and `10` numerical values are for just for
    /// illustration purposes, and application can choose an appropriate
    /// value for them.  Also note that pattern is case-insensitive.  Return
    /// zero if `command` is valid and message dumping has been configured,
    /// non-zero value otherwise.  The behavior is undefined unless the
    /// session has been started.
    virtual int configureMessageDumping(const bslstl::StringRef& command);
};

}  // close package namespace
}  // close enterprise namespace

#endif
