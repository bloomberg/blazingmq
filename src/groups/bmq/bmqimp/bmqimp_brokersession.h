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

// bmqimp_brokersession.h                                             -*-C++-*-
#ifndef INCLUDED_BMQIMP_BROKERSESSION
#define INCLUDED_BMQIMP_BROKERSESSION

//@PURPOSE: Provide the implementation of a session with the bmqbrkr.
//
//@CLASSES:
//  bmqimp::BrokerSession: Implementation of a session with the bmqbrkr.
//
//@DESCRIPTION: 'bmqimp::BrokerSession' implements all the high level business
// logic for communication with the bmqbrkr.  It manages the queues, messages,
// and handles the requests/responses with the bmqbrkr.
//
/// Thread Safety
///-------------
// Thread safe.

// BMQ

#include <bmqimp_eventqueue.h>
#include <bmqimp_eventsstats.h>
#include <bmqimp_messagecorrelationidcontainer.h>
#include <bmqimp_messagedumper.h>
#include <bmqimp_queue.h>
#include <bmqimp_queuemanager.h>
#include <bmqimp_stat.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_queueid.h>
#include <bmqp_requestmanager.h>
#include <bmqpi_dtcontext.h>
#include <bmqpi_dtspan.h>
#include <bmqpi_dttracer.h>
#include <bmqpi_hosthealthmonitor.h>
#include <bmqt_correlationid.h>
#include <bmqt_hosthealthstate.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_queueoptions.h>
#include <bmqt_resultcode.h>
#include <bmqt_sessionoptions.h>
#include <bmqt_uri.h>

// MWC
#include <mwcio_channel.h>
#include <mwcma_countingallocatorstore.h>
#include <mwcst_statcontext.h>
#include <mwcst_statvalue.h>
#include <mwcu_samethreadchecker.h>
#include <mwcu_throttledaction.h>

// BDE
#include <bdlbb_blob.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlcc_singleconsumerqueue.h>
#include <bdlmt_eventscheduler.h>
#include <bsl_deque.h>
#include <bsl_functional.h>
#include <bsl_memory.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bslmt_readerwritermutex.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bslmt_timedsemaphore.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqimp {

// FORWARD DECLARATION
class BrokerSession;
class Queue;

// ============================
// class BrokerSession_Executor
// ============================

/// Provides an executor suitable for submitting functors on session's FSM
/// thread. Note that this class conforms to the Executor concept as defined
/// in the `mwcex` package documentation.
class BrokerSession_Executor {
  private:
    // PRIVATE DATA
    BrokerSession* d_owner_p;

  public:
    // CREATORS

    /// Create a `BrokerSession_Executor` object for executing functors on
    /// FSM thread of the specified `session`.
    explicit BrokerSession_Executor(BrokerSession* session);

  public:
    // ACCESSORS

    /// Return `true` if `*this` refers to the same session as `rhs`, and
    /// `false` otherwise.
    bool operator==(const BrokerSession_Executor& rhs) const;

    /// Submit the specified functor `f` to be executed on the executor's
    /// associated session.
    void post(const bsl::function<void()>& f) const;
};

// ===================
// class BrokerSession
// ===================

/// Implementation of a session with the bmqbrkr.
class BrokerSession BSLS_CPP11_FINAL {
  public:
    // PUBLIC TYPES

    /// Invoked as a result of a response from the broker to a session event
    /// submitted by the user, `EventCallback` is an alias for a callback
    /// function object (functor) that takes as an argument the specified
    /// `event` indicating the result and context of the requested
    /// operation.
    typedef bmqimp::Event::EventCallback EventCallback;

    /// Enum representing the state of the connection with bmqbrkr
    struct State {
        // TYPES
        enum Enum {
            e_STARTING = 0  // The session has been started, but the
                            // channel is not yet connected
            ,
            e_STARTED = 1  // The session is started
            ,
            e_RECONNECTING = 2  // The connection is lost without user
                                // request
            ,
            e_CLOSING_SESSION = 3  // The disconnect request is sent to the
                                   // broker, waiting for the response
            ,
            e_CLOSING_CHANNEL = 4  // The channel is closing, waiting for the
                                   // operation to complete
            ,
            e_STOPPED = 5  // The session is stopped
        };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a
        /// reference to `stream`.  Optionally specify an initial
        /// indentation `level`, whose absolute value is incremented
        /// recursively for nested objects.  If `level` is
        /// specified, optionally specify `spacesPerLevel`, whose
        /// absolute value indicates the number of spaces per
        /// indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation
        /// of the first line.  If `spacesPerLevel` is negative,
        /// format the entire output on one line, suppressing all
        /// but the initial indentation (as governed by `level`).
        static bsl::ostream& print(bsl::ostream& stream,
                                   State::Enum   value,
                                   int           level          = 0,
                                   int           spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.
        static const char* toAscii(State::Enum value);
    };

    /// Enum representing events that accepts the session FSM
    struct FsmEvent {
        // TYPES
        enum Enum {
            e_START = 0  // User start request
            ,
            e_START_FAILURE = 1  // Session start has failed
            ,
            e_START_TIMEOUT = 2  // Session start timeout
            ,
            e_STOP = 3  // User stop request
            ,
            e_CHANNEL_UP = 4  // Network channel is up
            ,
            e_CHANNEL_DOWN = 5  // Network channel is down
            ,
            e_SESSION_DOWN = 6  // BlazingMQ broker is disconnected

            // Host Health events.
            ,
            e_HOST_UNHEALTHY = 7  // Host running application is unhealthy
            ,
            e_HOST_HEALTHY = 8  // Host has become healthy again
            ,
            e_ALL_QUEUES_RESUMED = 9  // All queues have been resumed
        };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a
        /// reference to `stream`.  Optionally specify an initial
        /// indentation `level`, whose absolute value is incremented
        /// recursively for nested objects.  If `level` is
        /// specified, optionally specify `spacesPerLevel`, whose
        /// absolute value indicates the number of spaces per
        /// indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation
        /// of the first line.  If `spacesPerLevel` is negative,
        /// format the entire output on one line, suppressing all
        /// but the initial indentation (as governed by `level`).
        static bsl::ostream& print(bsl::ostream&  stream,
                                   FsmEvent::Enum value,
                                   int            level          = 0,
                                   int            spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.
        static const char* toAscii(FsmEvent::Enum value);
    };

    /// Struct to represent state transition
    struct StateTransition {
        State::Enum d_currentState;
        // current state

        FsmEvent::Enum d_event;
        // incoming event

        State::Enum d_newState;
        // new state
    };

    /// Enum representing events accepted by the queue FSM
    struct QueueFsmEvent {
        // TYPES
        enum Enum {
            e_OPEN_CMD = 1  // Open queue request
            ,
            e_CONFIG_CMD = 2  // Configure queue request
            ,
            e_CLOSE_CMD = 3  // Close queue request
            ,
            e_REQ_NOT_SENT = 4  // Request is not sent to the broker
            ,
            e_RESP_OK = 5  // Successful response
            ,
            e_LATE_RESP = 6  // Late response
            ,
            e_RESP_BAD = 7  // Response with error
            ,
            e_RESP_TIMEOUT = 8  // Response timeout
            ,
            e_RESP_EXPIRED = 9  // Response timeout when channel is down
            ,
            e_CHANNEL_DOWN = 10  // Network channel is down
            ,
            e_CHANNEL_UP = 11  // Network channel is up
            // Host health events.
            ,
            e_SUSPEND = 12  // Queue has been asked to suspend
            ,
            e_RESUME = 13  // Queue has been asked to resume
            // Locally cancelled.
            ,
            e_REQ_CANCELED = 14  // Request is canceled by the SDK
            ,
            e_SESSION_DOWN = 15  // Request is canceled due to the session
                                 // goes down
        };

        // CLASS METHODS

        /// Write the string representation of the specified enumeration
        /// `value` to the specified output `stream`, and return a
        /// reference to `stream`.  Optionally specify an initial
        /// indentation `level`, whose absolute value is incremented
        /// recursively for nested objects.  If `level` is
        /// specified, optionally specify `spacesPerLevel`, whose
        /// absolute value indicates the number of spaces per
        /// indentation level for this and all of its nested
        /// objects.  If `level` is negative, suppress indentation
        /// of the first line.  If `spacesPerLevel` is negative,
        /// format the entire output on one line, suppressing all
        /// but the initial indentation (as governed by `level`).
        static bsl::ostream& print(bsl::ostream&       stream,
                                   QueueFsmEvent::Enum value,
                                   int                 level          = 0,
                                   int                 spacesPerLevel = 4);

        /// Return the non-modifiable string representation corresponding to
        /// the specified enumeration `value`, if it exists, and a unique
        /// (error) string otherwise.
        static const char* toAscii(QueueFsmEvent::Enum value);
    };

    /// Struct to represent state transition
    struct QueueStateTransition {
        /// current state
        QueueState::Enum d_currentState;

        /// incoming event
        QueueFsmEvent::Enum d_event;

        /// new state
        QueueState::Enum d_newState;
    };

    /// Signature of a state callback functor
    typedef bsl::function<
        bmqt::GenericResult::Enum(State::Enum, State::Enum, FsmEvent::Enum)>
        StateFunctor;

  private:
    // TYPES
    typedef bmqp::RequestManager<bmqp_ctrlmsg::ControlMessage,
                                 bmqp_ctrlmsg::ControlMessage>
        RequestManagerType;

    /// This is top-most internal callback to be set as `AsyncNotifierCb` in
    /// all requests.  BS guarantees that 1) the callback will always be
    /// called; 2) from the FSM thread.
    /// It can be one of two:
    ///  1) for async operations - `asyncRequestNotifier`
    ///  2) for sync operations  - `syncRequestNotifier`
    typedef bsl::function<void(const RequestManagerType::RequestSp&)>
        FsmCallback;

    /// Signature of the callback to provide to the `configure` method.
    typedef bsl::function<void(const RequestManagerType::RequestSp& context,
                               const bsl::shared_ptr<Queue>&        queue)>
        ConfiguredCallback;

    typedef bdlcc::SharedObjectPool<Event,
                                    bdlcc::ObjectPoolFunctors::DefaultCreator,
                                    bdlcc::ObjectPoolFunctors::Clear<Event> >
        EventPool;

    typedef bdlcc::SingleConsumerQueue<bsl::shared_ptr<Event> > FsmEventQueue;

    typedef bsl::unordered_map<int, int> QueueRetransmissionTimeoutMap;

    class SessionFsm {
      private:
        BrokerSession& d_session;
        // Reference to the parent object

        bsls::AtomicInt d_state;
        // Current state

        bool d_onceConnected;
        // Indicate that the session has once
        // reached STARTED state

        bsl::vector<StateTransition> d_transitionTable;
        // State transition table

        /// HiRes timer value of the begin start/stop operation
        bsls::Types::Int64 d_beginTimestamp;

      private:
        // PRIVATE MANIPULATORS

        /// Set the specified state `value` to be the current state of this
        /// FSM and call parent's `stateCb` callback.  Verify that it is a
        /// valid transition from the current state to the `value` state on
        /// the specified `event`
        bmqt::GenericResult::Enum setState(BrokerSession::State::Enum value,
                                           FsmEvent::Enum             event);

        /// Unconditionally set STARTED state as a reaction to the specified
        /// `event` and store the specified `channel` pointer.
        void setStarted(FsmEvent::Enum                         event,
                        const bsl::shared_ptr<mwcio::Channel>& channel);

        /// Unconditionally set STOPPED state as a reaction to the specified
        /// `event`.  If the specified `isStartTimeout` flag is `true` then
        /// the CONNECTION_TIMEOUT event is emitted
        void setStopped(FsmEvent::Enum event, bool isStartTimeout = false);

        bmqt::GenericResult::Enum setStarting(FsmEvent::Enum event);

        bmqt::GenericResult::Enum setClosingSession(FsmEvent::Enum event);

        void setReconnecting(FsmEvent::Enum event);

        /// Unconditionally set corresponding FSM state as a reaction to the
        /// specified `event` and execute state entry logic.  Return value
        /// if not void indicates state entry logic execution result.
        void setClosingChannel(FsmEvent::Enum event);

        /// Log start/stop operation time for the specified `operation`,
        /// using the stored operation begin timestamp.
        /// Reset the begin timestamp to 0.
        void logOperationTime(const char* operation);

      public:
        // CREATORS
        SessionFsm(BrokerSession& session);

        // ACCESSORS

        /// return the current FSM state
        BrokerSession::State::Enum state() const;

        /// return a reference to the non-modifiable transition table
        const bsl::vector<BrokerSession::StateTransition>& table() const;

        // MANIPULATORS

        /// Handle user start request event
        bmqt::GenericResult::Enum handleStartRequest();

        /// Handle start request timeout event
        void handleStartTimeout();

        /// Handle synchronous start request failure.
        void handleStartSynchronousFailure();

        /// Handle user stop request event
        void handleStopRequest();

        /// Handle IO channel up event that provides the specified `channel`
        void handleChannelUp(const bsl::shared_ptr<mwcio::Channel>& channel);

        /// Handle IO channel down event
        void handleChannelDown();

        /// Handle disconnect broker response
        void handleSessionClosed();

        /// Handle application host transitioning to an unhealthy state
        void handleHostUnhealthy();

        /// Handle restoration of health of the application host
        void handleHostHealthy();

        /// Handle all queues resumed, following restoration of host health
        void handleAllQueuesResumed();
    };

    class QueueFsm {
      private:
        // PRIVATE TYPES

        typedef bsl::unordered_map<bsl::string, bsls::Types::Int64>
            TimestampMap;

        // PRIVATE DATA

        BrokerSession& d_session;
        // Reference to the parent object

        bsl::vector<QueueStateTransition> d_transitionTable;
        // State transition table

        TimestampMap d_timestampMap;
        // Map of HiRes timestamp of the operation beginning per each queue

      private:
        // PRIVATE MANIPULATORS

        /// Set the state of the specified `queue` to the specified `value`.
        /// The state transition is checked against the transition table,
        /// i.e. if the combination of the current queue state, the
        /// specified queue FSM `event` and the new queue state is present
        /// in the table.  Assert is fired if the state transition is not in
        /// the table.
        void setQueueState(const bsl::shared_ptr<Queue>& queue,
                           QueueState::Enum              value,
                           QueueFsmEvent::Enum           event);

        /// Generate and set a valid queueId to the specified `queue`.
        void setQueueId(const bsl::shared_ptr<Queue>&        queue,
                        const RequestManagerType::RequestSp& context);

        /// Start opening the specified `queue` with the specified
        /// `timeout` for the operation to complete.  Use the specified
        /// request `context` to signal about operation result.  Return
        /// `success` on successful send of the open request, or a reason of
        /// the failure if it couldn't be sent for any reason.
        bmqt::OpenQueueResult::Enum
        actionOpenQueue(const bsl::shared_ptr<Queue>&        queue,
                        const RequestManagerType::RequestSp& context,
                        const bsls::TimeInterval&            timeout);

        /// Start reopening the specified `queue` with the specified
        /// `timeout` for the operation to complete.  Return `success` on
        /// successful send of the open request, or a reason of the failure
        /// if it couldn't be sent for any reason.
        bmqt::OpenQueueResult::Enum
        actionReopenQueue(const bsl::shared_ptr<Queue>&        queue,
                          const RequestManagerType::RequestSp& context,
                          const bsls::TimeInterval&            timeout);

        /// Start the first phase of the closing queue procedure for the
        /// specified `queue` sending a configure request with null
        /// settings with the specified `timeout` for the operation to
        /// complete.  Set the specified `closeContext` to signal the
        /// operationon result.  Return `success` on/ successful send of the
        /// configure request, or a reason of the failure if it couldn't be
        /// sent for any reason.
        bmqt::ConfigureQueueResult::Enum actionDeconfigureQueue(
            const bsl::shared_ptr<Queue>&        queue,
            const RequestManagerType::RequestSp& closeContext,
            const bsls::TimeInterval&            timeout);

        /// Start the first phase of the closing queue procedure for the
        /// specified expired `queue`.  Return `success` on successful send
        /// of the configure request, or a reason of the failure if it
        /// couldn't be sent for any reason.
        bmqt::ConfigureQueueResult::Enum
        actionDeconfigureExpiredQueue(const bsl::shared_ptr<Queue>& queue);

        /// Remove the specified `queue` from the active queues container.
        void actionRemoveQueue(const bsl::shared_ptr<Queue>& queue);

        /// Start the second phase of the open queue sequence with the
        /// specified `timeout` to complete.  Send configure queue request
        /// for the specified `queue` and bind the specified initial open
        /// `context` to the configure callback.  The specified
        /// `isReopenRequest` flag indicates whether the open sequence is
        /// performed to reopen the `queue`.  Return `success` on successful
        /// send of the configure request, or a reason of the failure if it
        /// couldn't be sent for any reason.
        bmqt::ConfigureQueueResult::Enum actionOpenConfigureQueue(
            const RequestManagerType::RequestSp& openQueueContext,
            const RequestManagerType::RequestSp& configQueueContext,
            const bsl::shared_ptr<Queue>&        queue,
            const bsls::TimeInterval&            timeout,
            bool                                 isReopenRequest);

        /// Send a close queue request to close the specified `queue`.
        void actionCloseQueue(const bsl::shared_ptr<Queue>& queue);

        /// Send a close queue request to close the specified `queue`.  Bind
        /// the specified `context` to the request callback.  Use the
        /// specified `absTimeout` value as a request timeout.
        bmqt::GenericResult::Enum
        actionCloseQueue(const RequestManagerType::RequestSp& context,
                         const bsl::shared_ptr<Queue>&        queue,
                         const bsls::TimeInterval&            absTimeout);

        /// Called when the specified `queue` is successfully opened to set
        /// queue configuration parameters provided by the broker and stored
        /// in the specified `openQueueContext`.  If the specified
        /// `isReopenRequest` flag is false enable queue statistics.
        void
        actionInitQueue(const bsl::shared_ptr<Queue>&        queue,
                        const RequestManagerType::RequestSp& openQueueContext,
                        bool                                 isReopenRequest);

        /// Make configure queue request for the specified `queue` with the
        /// specified `options` with the specified `timeout` for the
        /// operation to complete.  Use the specified `context` for the
        /// request and result notifier.  Return `success` on successful
        /// send of the configure request, or a reason of the failure if it
        /// couldn't be sent for any reason.
        bmqt::ConfigureQueueResult::Enum
        actionConfigureQueue(const bsl::shared_ptr<Queue>&  queue,
                             const bmqt::QueueOptions&      options,
                             const bsls::TimeInterval&      timeout,
                             RequestManagerType::RequestSp& context);

        /// Send configure queue request for the specified `queue` taking in
        /// account the specified `response` from the previous configure
        /// request.  Return `success` on successful send of the configure
        /// request, or a reason of the failure if it couldn't be sent for
        /// any reason.
        bmqt::ConfigureQueueResult::Enum
        actionReconfigureQueue(const bsl::shared_ptr<Queue>&       queue,
                               const bmqp_ctrlmsg::ControlMessage& response);

        /// Initiate the suspension of a queue.
        void actionInitiateQueueSuspend(const bsl::shared_ptr<Queue>& queue);

        /// Initiate the resumption of a queue.
        void actionInitiateQueueResume(const bsl::shared_ptr<Queue>& queue);

        /// Log start/stop/configure operation time for the specified
        /// `queueUri` and `operation`, using the stored operation begin
        /// timestamp. After logging, begin timestamp is removed from
        /// timestamps map.
        void logOperationTime(const bsl::string& queueUri,
                              const char*        operation);

      public:
        // CREATORS
        QueueFsm(BrokerSession& session);

        // ACCESSORS

        /// return a reference to the non-modifiable transition table
        const bsl::vector<BrokerSession::QueueStateTransition>& table() const;

        // MANIPULATORS

        /// Handle user open queue request.  Any error indicated by the
        /// return value means that the queue FSM has not accepted the
        /// request.
        bmqt::OpenQueueResult::Enum
        handleOpenRequest(const bsl::shared_ptr<Queue>&        queue,
                          const bsls::TimeInterval&            timeout,
                          const RequestManagerType::RequestSp& context);

        /// Handle queue reopen request initiated by the session.
        void handleReopenRequest(const bsl::shared_ptr<Queue>&        queue,
                                 const bsls::TimeInterval&            timeout,
                                 const RequestManagerType::RequestSp& context);

        /// Handle queue configure request initiated by the user.
        bmqt::ConfigureQueueResult::Enum
        handleConfigureRequest(const bsl::shared_ptr<Queue>&  queue,
                               const bmqt::QueueOptions&      options,
                               const bsls::TimeInterval&      timeout,
                               RequestManagerType::RequestSp& context);

        /// Handle queue close request initiated by the user.
        bmqt::CloseQueueResult::Enum
        handleCloseRequest(const bsl::shared_ptr<Queue>&        queue,
                           const bsls::TimeInterval&            timeout,
                           const RequestManagerType::RequestSp& context);

        /// Handle a case when a request to the broker has not been sent
        /// due to some error described with the specified `status`.
        void handleRequestNotSent(const bsl::shared_ptr<Queue>&        queue,
                                  const RequestManagerType::RequestSp& context,
                                  bmqp_ctrlmsg::StatusCategory::Value  status);

        /// Handle response error.
        void handleResponseError(const bsl::shared_ptr<Queue>&        queue,
                                 const RequestManagerType::RequestSp& context,
                                 const bsls::TimeInterval& absTimeout);

        void handleSessionDown(const bsl::shared_ptr<Queue>&        queue,
                               const RequestManagerType::RequestSp& context);

        /// Handle the request is canceled locally.
        void
        handleRequestCanceled(const bsl::shared_ptr<Queue>&        queue,
                              const RequestManagerType::RequestSp& context);

        /// Handle broker response timeout.
        void
        handleResponseTimeout(const bsl::shared_ptr<Queue>&        queue,
                              const RequestManagerType::RequestSp& context);

        /// Handle broker response timeout while there is no connection.
        void
        handleResponseExpired(const bsl::shared_ptr<Queue>&        queue,
                              const RequestManagerType::RequestSp& context);

        /// Handle successful response from the broker.
        void handleResponseOk(const bsl::shared_ptr<Queue>&        queue,
                              const RequestManagerType::RequestSp& context,
                              const bsls::TimeInterval&            timeout);

        /// Process a response received from the broker for a request that
        /// was already timed out internally in the client.
        void handleLateResponse(const bsl::shared_ptr<Queue>&        queue,
                                const RequestManagerType::RequestSp& context);

        /// Handle network channel down event.
        void handleChannelDown(const bsl::shared_ptr<Queue>& queue);

        /// Handle a request to suspend a queue.
        void handleQueueSuspend(const bsl::shared_ptr<Queue>& queue);

        /// Handle a request to resume a queue.
        void handleQueueResume(const bsl::shared_ptr<Queue>& queue);

        /// Create a status response for the specified `context` using the
        /// specified `status` code and error `reason` description.
        void injectErrorResponse(const RequestManagerType::RequestSp& context,
                                 bmqp_ctrlmsg::StatusCategory::Value  status,
                                 const bslstl::StringRef& reason = "");
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQIMP.BROKERSESSION");

  private:
    // DATA
    mwcma::CountingAllocatorStore d_allocators;
    // Allocator store to spawn new
    // allocators for sub-components

    EventPool d_eventPool;
    // ObjectPool of Event

    const bmqt::SessionOptions& d_sessionOptions;
    // SessionOptions

    bdlmt::EventScheduler* d_scheduler_p;
    // Pointer to the event scheduler to
    // use (held, not owned)

    bdlbb::BlobBufferFactory* d_bufferFactory_p;
    // Raw pointer (held, not owned) to
    // the blob buffer factory to use.

    bsl::shared_ptr<mwcio::Channel> d_channel_sp;
    // Channel to use for communication,
    // held not owned

    bsl::deque<bdlbb::Blob> d_extensionBlobBuffer;
    // Buffer to store event blobs when
    // they cannot be sent due to the
    // channel HWM condition.

    bsls::AtomicBool d_acceptRequests;
    // False if a session is not
    // started yet or a disconnect
    // request has been sent; this is
    // used to reject operations once
    // 'stop()' has been called to make
    // sure that the contract
    // ('disconnectRequest' is the *last*
    // message a client will send) is
    // honored and no request/operations
    // are being attempted after 'stop()'
    // and before the channel goes down.

    bsls::AtomicBool d_extensionBufferEmpty;
    // If false user events (PUTs or
    // CONFIRMs) cannot be accepted.  In
    // this case back pressure mechanism
    // is enabled to block user's thread
    // until the messages can be sent.

    bslmt::Mutex d_extensionBufferLock;
    // Lock for usage of the
    // 'd_extensionBlobBuffer'

    bslmt::Condition d_extensionBufferCondition;
    // Condition variable to notify
    // 'post' caller that the extension
    // buffer is empty.

    mutable Stat d_queuesStats;
    // Stats for all queues

    mutable EventsStats d_eventsStats;
    // Stats for all events

    const StateFunctor d_stateCb;
    // Callback to invoke when the
    // session makes state transition.

    bool d_usingSessionEventHandler;
    // True if the session is configured
    // with a session event handler
    // callback, for event processing;
    // false if the user didn't specify
    // one (and will use nextEvent).

    MessageCorrelationIdContainer d_messageCorrelationIdContainer;
    // Message correlationId container

    bslmt::ThreadUtil::Handle d_fsmThread;
    // FSM thread handle

    mwcu::SameThreadChecker d_fsmThreadChecker;
    // Mechanism to check if a method is
    // called in the FSM thread

    FsmEventQueue d_fsmEventQueue;
    // Queue of internal events

    EventQueue d_eventQueue;
    // Queue of events to deliver to the
    // application

    RequestManagerType d_requestManager;
    // Request manager

    QueueManager d_queueManager;
    // Queues manager

    bsls::AtomicInt d_numPendingReopenQueues;
    // Number of requests that have been
    // sent to the broker to reopen the
    // queues after connection was
    // re-established with the broker.

    bsls::AtomicInt d_numPendingHostHealthRequests;
    // Number of requests that have been
    // sent to the broker to suspend or
    // resume queues that have not yet
    // received responses.

    mwcu::ThrottledActionParams d_throttledFailedPostMessage;
    // State for mwcu::ThrottledAction,
    // across all queues for when posting
    // fails due to BW_LIMIT.

    mwcu::ThrottledActionParams d_throttledFailedAckMessages;
    // State for mwcu::ThrottledAction,
    // across all queues.

    bmqimp::MessageDumper d_messageDumper;
    // Message dumper

    bslma::Allocator* d_allocator_p;
    // Allocator to use

    SessionFsm d_sessionFsm;
    // FSM of this component

    QueueFsm d_queueFsm;
    // FSM of the Queue objects

    bmqt::HostHealthState::Enum d_hostHealthState;
    // Last queried host health state.

    bdlmt::SignalerConnectionGuard d_hostHealthSignalerConnectionGuard;
    // Handle representing connection
    // between a HostHealthMonitor and an
    // internal callback to be invoked
    // when the host health changes.

    bslmt::TimedSemaphore d_startSemaphore;
    // Semaphore used for making 'sync'
    // start

    bslmt::Semaphore d_stopSemaphore;
    // Semaphore used for making 'sync'
    // stop

    bslmt::Mutex d_startStopMutex;
    // Mutex to synchronize the stop and
    // start operations

    bsls::AtomicInt d_inProgressEventHandlerCount;
    // Counter of how many threads are
    // currently processing a user event

    bsls::AtomicBool d_isStopping;
    // true means the session is stopping
    // and the DISCONNECTED event has
    // been dispatched to the user event
    // handler

    bdlmt::EventScheduler::EventHandle d_messageExpirationTimeoutHandle;
    // Timer Event handle for pending PUT
    // messages' expiration timeout

    int d_nextRequestGroupId;
    // Id of the next request group to
    // use

    QueueRetransmissionTimeoutMap d_queueRetransmissionTimeoutMap;
    // Map queueId and the queue
    // retransmission timeout provided by
    // the broker

    unsigned int d_nextInternalSubscriptionId;
    // Assists generating unique ids for Configure requests.

    int d_doConfigureStream;
    // Temporary safety switch to control configure request.

  private:
    // NOT IMPLEMENTED
    BrokerSession(const BrokerSession&);
    BrokerSession& operator=(const BrokerSession&);

  private:
    // PRIVATE MANIPULATORS

    /// Set the state to the specified `state`, call stateCB callback and
    /// return a result code of this callback.
    int setState(State::Enum state);

    /// Format the specified `context` as a bmqp message and send it to the
    /// BlazingMQ broker, with the specified `timeout` and `context`.
    /// Return `success` on successful send of the request, or a reason of
    /// the failure if it couldn't be sent for any reason (not connected,
    /// unable to serialize the request, ...).
    bmqt::GenericResult::Enum
    sendRequest(const RequestManagerType::RequestSp& context,
                const bmqp::QueueId&                 queueId,
                bsls::TimeInterval                   timeout);

    /// Send the specified `blob`, representing a `Confirm` packet over the
    /// channel and containing the specified `msgCount` message confirmation
    /// in it (count used for statistics).
    void sendConfirm(const bdlbb::Blob& blob, const int msgCount);

    /// Dequeue and handle events from the FSM queue.  This method runs in
    /// FSM thread until the thread is stopped.
    void fsmThreadLoop();

    /// Decorate the specified `event`, before invoking the specified
    /// `eventHandlerCb` with it.  Used to wrap the user provided
    /// SessionEventHandler so that `event` can be decorated before being
    /// delivered.
    ///
    /// THREAD: This method is called from one of the EVENT HANDLER
    ///         dispatcher threads.
    void eventHandlerCbWrapper(
        const bsl::shared_ptr<Event>&           event,
        const EventQueue::EventHandlerCallback& eventHandlerCb);

    /// Callback used as a wrapper for async operations and new-style
    /// operations in event handler mode.  Read result from the specified
    /// `context`.  Create and enqueue user event with the specified
    /// `eventType`, `queue`, `correlationId`, and `eventCallback`.
    /// Called from either FSM or the caller thread.
    void asyncRequestNotifier(const RequestManagerType::RequestSp& context,
                              bmqt::SessionEventType::Enum         eventType,
                              const bmqt::CorrelationId&    correlationId,
                              const bsl::shared_ptr<Queue>& queue,
                              const EventCallback&          eventCallback);

    /// Callback used as a wrapper for deprecated sync operations.  Set the
    /// specified result `status`, and finally post on the specified
    /// `semaphore`.  Called from either FSM or the caller thread.
    void syncRequestNotifier(bslmt::Semaphore*                    semaphore,
                             int*                                 status,
                             const RequestManagerType::RequestSp& context);

    /// Callback used as a wrapper for sync operations (from the `xxxSync`
    /// APIs flavors) in the mode where event handler is not used (i.e.
    /// users explicitly controlling calls to `nextEvent`).
    /// Execute the specified `eventCallback` for the specified 'eventType,
    /// `queue`, and `correlationId` in either FSM or the caller thread.
    void
    manualSyncRequestNotifier(const RequestManagerType::RequestSp& context,
                              bmqt::SessionEventType::Enum         eventType,
                              const bmqt::CorrelationId&    correlationId,
                              const bsl::shared_ptr<Queue>& queue,
                              const EventCallback&          eventCallback);

    /// Process the raw BlazingMQ event represented by the specified
    /// `event`.  This method gets called each time a new event (received
    /// from the broker) is taken from the FSM event queue.
    void processRawEvent(const bmqp::Event& event);

    /// Process the control event represented by the specified `event`.
    /// This method gets called each time a new control event (received from
    /// the broker) is available on the channel.
    void processControlEvent(const bmqp::Event& event);

    /// This method gets called each time a new heart beat event (sent by
    /// the broker) is available on the channel.
    void onHeartbeat();

    void enableMessageRetransmission(const bmqp::PutMessageIterator& putIter,
                                     const bsls::TimeInterval&       sentTime);

    /// Process the put event represented by the specified `event`.  This
    /// method gets called each time a new put event is poseted by the user.
    void processPutEvent(const bmqp::Event& event);

    /// Process the confirm event represented by the specified `event`.
    /// This method gets called each time a new confirm event is poseted by
    /// the user.
    void processConfirmEvent(const bmqp::Event& event);

    /// Process the push event represented by the specified `event`.  This
    /// method gets called each time a new push event (received from the
    /// broker) is available on the channel.
    void processPushEvent(const bmqp::Event& event);

    /// Process the ack event represented by the specified `event`.  This
    /// method gets called each time a new ack event (received from the
    /// broker) is available on the channel.
    void processAckEvent(const bmqp::Event& event);

    /// Callback invoked in reply to a `disconnect` with the specified
    /// `context`.
    void onDisconnectResponse(const RequestManagerType::RequestSp& context);

    /// Invoke queue FSM event handler depending on the specified `contex`
    /// state and the specified `isLocalTimeout` and `isLateResponse` flags.
    /// Submit the specified `queue` and `absTimeout` to the FSM handler.
    void handleQueueFsmEvent(const RequestManagerType::RequestSp& context,
                             const bsl::shared_ptr<Queue>&        queue,
                             bool                     isLocalTimeout,
                             bool                     isLateResponse,
                             const bsls::TimeInterval absTimeout);

    /// Callback invoked in reply to an `openQueue` (first part in the
    /// process of opening a queue) for the specified `queue`, with the
    /// specified request `context`.  Wait until the specified `absTimeout`
    /// for the request to complete.
    void onOpenQueueResponse(const RequestManagerType::RequestSp& context,
                             const bsl::shared_ptr<Queue>&        queue,
                             const bsls::TimeInterval             absTimeout);

    /// Callback invoked in reply to a `closeQueue` (second part in the
    /// process of closing a queue) for the specified `queue`, with the
    /// specified request `context`.
    void onCloseQueueResponse(const RequestManagerType::RequestSp& context,
                              const bsl::shared_ptr<Queue>&        queue);

    /// Send configure queue request of the open queue sequence with the
    /// specified `timeout` to complete for the specified `queue` and bind
    /// the specified initial open `request` to the configure callback.  The
    /// specified `isReopenRequest` flag indicates whether the open sequence
    /// is performed to reopen the `queue`.  Return `success` on successful
    /// send of the configure request, or a reason of the failure if it
    /// couldn't be sent for any reason.
    bmqt::ConfigureQueueResult::Enum sendOpenConfigureQueue(
        const RequestManagerType::RequestSp& openQueueContext,
        const RequestManagerType::RequestSp& configQueueContext,
        const bsl::shared_ptr<Queue>&        queue,
        const bsls::TimeInterval             absTimeout,
        bool                                 isReopenRequest);

    /// Send a close queue request to close the specified `queue`.
    void sendCloseQueue(const bsl::shared_ptr<Queue>& queue);

    /// Send a close queue request to close the specified `queue`. Bind the
    /// specified `closeQueueRequest` context to the request callback.
    /// Use the specified `absTimeout` value as a request timeout, and the
    /// specified `isFinal` flag for the corresponding close request field.
    bmqt::GenericResult::Enum
    sendCloseQueue(const RequestManagerType::RequestSp& closeQueueRequest,
                   const bsl::shared_ptr<Queue>&        queue,
                   const bsls::TimeInterval             absTimeout,
                   bool                                 isFinal);

    /// Callback invoked after reply to a `configureQueue` that was sent as
    /// the second part in the process of opening the specified `queue`,
    /// with the specified contexts `openQueueContext` and
    /// `configureQueueContext`, and where the `isReopenRequest` flag
    /// indicates if the request was a reopen after connection was
    /// re-established.
    void onOpenQueueConfigured(
        const RequestManagerType::RequestSp& configureQueueContext,
        const bsl::shared_ptr<Queue>&        queue,
        const RequestManagerType::RequestSp& openQueueContext,
        bool                                 isReopenRequest);

    /// Callback invoked in reply to a `configureQueue` that was configured
    /// with the options `previousOptions` for the specified `queue`, having
    /// the specified `previousOptions`, with the specified request
    /// `context`, and where the specified `configuredCb` is invoked when
    /// finished.
    void onConfigureQueueResponse(const RequestManagerType::RequestSp& context,
                                  const bsl::shared_ptr<Queue>&        queue,
                                  const bmqt::QueueOptions& previousOptions,
                                  const ConfiguredCallback& configuredCb);

    /// Callback invoked after a standalone configureQueue for the specified
    /// `queue` with the specified request `context`.
    void
    onConfigureQueueConfigured(const RequestManagerType::RequestSp& context,
                               const bsl::shared_ptr<Queue>&        queue);

    /// Callback invoked on completion of a request to suspend a queue.
    /// Enqueues a pair of session-events that will notify client of
    /// suspension and begin rejecting subsequent writes to the queue.
    void onSuspendQueueConfigured(const RequestManagerType::RequestSp& context,
                                  const bsl::shared_ptr<Queue>&        queueSp,
                                  const bool deferred);

    /// Callback invoked on completion of a request to resume a queue.
    /// Enqueues a pair of session-events that will notify client of
    /// suspension and resume accepting subsequent writes to the queue. If
    /// there are no other outstanding host-health requests, also enqueues
    /// an `ALL_QUEUES_RESUMED` event to notify the client that ordinary
    /// operation of the application has resumed.
    void onResumeQueueConfigured(const RequestManagerType::RequestSp& context,
                                 const bsl::shared_ptr<Queue>&        queueSp,
                                 const bool deferred);

    /// Callback invoked after reply to a `configureQueue` that was sent as
    /// the first part in the process of closing the specified `queue`, with
    /// the specified `configureQueueContext`, and where the specified
    /// `closeQueueRequest` is to be prepared and sent along with specified
    /// `isFinal` flag.  Wait until the specified `absTimeout` for the
    /// request to complete.
    void onCloseQueueConfigured(
        const RequestManagerType::RequestSp& configureQueueContext,
        const bsl::shared_ptr<Queue>&        queue,
        const RequestManagerType::RequestSp& closeQueueRequest,
        const bsls::TimeInterval             absTimeout,
        bool                                 isFinal);

    /// Invoked when a valid channel that was once connected is being set.
    /// Reopen all queues which were opened before connection dropped
    ///
    /// THREAD: This method is called from the FSM thread.
    void reopenQueues();

    /// Invoked when a network channel goes down.  Invoke queue FSM handler
    /// for channel down event.
    ///
    /// THREAD: This method is called from the FSM thread.
    void notifyQueuesChannelDown();

    /// Open the specified `queue` using the specified `request`.  Wait for
    /// the specified `timeout` for the request to complete.  Set the
    /// optionally specified `eventCallback` on the event being enqueued to
    /// the event queue.  Return `success` on successful send of the
    /// request, or a reason of the failure if it couldn't be sent for any
    /// reason.
    bmqt::OpenQueueResult::Enum
    openQueueImp(const bsl::shared_ptr<Queue>&  queue,
                 bsls::TimeInterval             timeout,
                 RequestManagerType::RequestSp& context);

    /// Helper method to open the specified `queue` using the specified
    /// `context`.  Wait for the specified `timeout` for the request to
    /// complete. Return `success` on successful send of the request, or a
    /// reason of the failure if it couldn't be sent for any reason.
    bmqt::OpenQueueResult::Enum
    sendOpenQueueRequest(const RequestManagerType::RequestSp& context,
                         const bsl::shared_ptr<Queue>&        queue,
                         bsls::TimeInterval                   timeout);

    /// Configure the specified `queue` using the specified `options`, with
    /// the specified `context`.  If the specified `checkConcurrent` is
    /// true, then check that the queue's `pendingConfigureId` is empty,
    /// return error if it's not empty. If `updateOptions` is true, then the
    /// queue's options are updated to match, and these are understood to be
    /// the options currently preferred by the client.  Wait for the
    /// specified `timeout` for the request to complete, and invoke the
    /// specified `configuredCb` when finished.  Return `success` on
    /// successful send of the configureQueue request, or a reason of the
    /// failure if it couldn't be sent for any reason (not connected, unable
    /// to serialize the request, ...).
    bmqt::ConfigureQueueResult::Enum
    configureQueueImp(const RequestManagerType::RequestSp& context,
                      const bsl::shared_ptr<Queue>&        queue,
                      const bmqt::QueueOptions&            newClientOptions,
                      const bsls::TimeInterval             timeout,
                      const ConfiguredCallback&            configuredCb,
                      const bool checkConcurrent = true);

    /// Send configure queue request for the specified `queue` with the
    /// specified `options` with the specified `timeout` for the operation
    /// to complete.  Set the specified `eventCallback` on the event being
    /// enqueued to the event queue.  Return `success` on successful send of
    /// the configure request, or a reason of the failure if it couldn't be
    /// sent for any reason.
    bmqt::ConfigureQueueResult::Enum
    sendConfigureRequest(const bsl::shared_ptr<Queue>&  queue,
                         const bmqt::QueueOptions&      options,
                         const bsls::TimeInterval       timeout,
                         RequestManagerType::RequestSp& context);

    /// Send configure queue request for the specified `queue`.  The result
    /// of this request is not provided to the user.
    bmqt::ConfigureQueueResult::Enum
    sendReconfigureRequest(const bsl::shared_ptr<Queue>& queue);

    /// For the specified `queue` send a configure request with null
    /// settings with the specified `timeout` for the operation to complete.
    /// Use the specified `closeContext` to signal about operation result.
    /// Return `success` on successful send of the configure request, or a
    /// reason of the failure if it couldn't be sent for any reason.
    bmqt::ConfigureQueueResult::Enum
    sendDeconfigureRequest(const bsl::shared_ptr<Queue>&        queue,
                           const RequestManagerType::RequestSp& closeContext,
                           bsls::TimeInterval                   timeout);

    /// For the specified `queue` send a configure request with null
    /// settings.  Return `success` on successful send of the configure
    /// request, or a reason of the failure if it couldn't be sent for any
    /// reason.
    bmqt::ConfigureQueueResult::Enum
    sendDeconfigureRequest(const bsl::shared_ptr<Queue>& queue);

    /// For the specified `queue` send a suspend request.
    bmqt::ConfigureQueueResult::Enum
    sendSuspendRequest(const bsl::shared_ptr<Queue>&  queueSp,
                       const bmqt::QueueOptions&      options,
                       const bsls::TimeInterval       timeout,
                       RequestManagerType::RequestSp& context);

    /// For the specified `queue` send a resume request.
    bmqt::ConfigureQueueResult::Enum
    sendResumeRequest(const bsl::shared_ptr<Queue>&  queueSp,
                      const bmqt::QueueOptions&      options,
                      const bsls::TimeInterval       timeout,
                      RequestManagerType::RequestSp& context);

    /// Enqueue a `STATE_RESTORED` event if responses for all the
    /// reopen-queue requests have been received from the broker, after the
    /// connection to broker is re-established.  Note that `STATE_RESTORED`
    /// event is enqueued irrespective of the results of the open-queue
    /// requests.
    void enqueueStateRestoredIfNeeded();

    /// Cancel all pending requests (buffered and non-buffered) for the
    /// specified `queue` with the specified `status` adding the optionally
    /// specified cancellation `reason` to the cancel event.
    void cancel(const bsl::shared_ptr<Queue>&       queue,
                bmqp_ctrlmsg::StatusCategory::Value status,
                const bslstl::StringRef&            reason = "");

    /// Cancel all pending requests (buffered and non-buffered) for all
    /// queues with the specified `status` adding the optionally specified
    /// cancellation `reason` to the cancel event.
    void cancel(bmqp_ctrlmsg::StatusCategory::Value status,
                const bslstl::StringRef&            reason = "");

    /// Cancel pending requests with the specified `groupId` with the
    /// specified `status` adding the optionally specified cancellation
    /// `reason` to the cancel event.  If the `groupId` value is set to -1
    /// cancel all requests.
    void cancel(int                                 groupId,
                bmqp_ctrlmsg::StatusCategory::Value status,
                const bslstl::StringRef&            reason = "");

    /// Generate an acknowledgement event using the specified `ackBuilder`,
    /// for the message associated with the specified `queueSp`, `guid`
    /// and `qac`, and populate the specified `ackEvents` vector with the
    /// event blobs.  Return true if the iteration over the message items
    /// should be stopped.
    bool cancelPendingMessageImp(
        bmqp::AckEventBuilder*                                      ackBuilder,
        bsl::shared_ptr<Event>*                                     ackEvent,
        bool*                                                       deleteItem,
        const bsl::shared_ptr<Queue>&                               queueSp,
        const bmqt::MessageGUID&                                    guid,
        const MessageCorrelationIdContainer::QueueAndCorrelationId& qac);

    /// Nack all pending messages for the specified `queueSp`.  If `queueSp`
    /// is null, nack all messages across all queues.
    void cancelPendingMessages(const bsl::shared_ptr<Queue>& queueSp);

    /// Put the specified `event` into the FSM event queue.  Return SUCCESS
    /// if the event is enqueued, error code otherwise.
    bmqt::GenericResult::Enum enqueueFsmEvent(bsl::shared_ptr<Event>& event);

    /// Send the message blob from the specified `qac` item to the channel.
    /// Set the specified `deleteItem` flag to true if the related request
    /// is already completed (the blob is not sent in this case).  Return
    /// true if the iteration should be interrupted Set the specified
    /// `interrupt` flag to true if the write operation has failed.
    bool retransmitControlMessage(
        bool*                                                       deleteItem,
        const MessageCorrelationIdContainer::QueueAndCorrelationId& qac);

    /// Append the message blob from the specified `qac` item to the
    /// specified `builder`.  Return true if appended successfully.  If the
    /// appending fails due to PAYLOAD_TOO_BIG error send the current event
    /// to the channel, reset the `builder` and return false.  If the
    /// channel write fails set the specified `interrupt` flag to true.
    bool appendOrSend(
        bool*                                                       interrupt,
        bmqp::PutEventBuilder&                                      builder,
        const MessageCorrelationIdContainer::QueueAndCorrelationId& qac);

    /// Handle pending PUT or CONTROL message associated with the specified
    /// `guid` and `qac`:
    /// o For PUT message append it to the PUT event using the specified
    ///   `putBuilder`.
    /// o For CONTROL message send the message blob.
    /// Return true if the item iteration should be interrupted.
    bool handlePendingMessage(
        bmqp::PutEventBuilder*                                      putBuilder,
        bool*                                                       deleteItem,
        const bmqt::MessageGUID&                                    guid,
        const MessageCorrelationIdContainer::QueueAndCorrelationId& qac);

    /// Send all unACKed PUT messages and pending CONTROL requests.
    void retransmitPendingMessages();

    void transferAckEvent(bmqp::AckEventBuilder*  ackBuilder,
                          bsl::shared_ptr<Event>* ackEvent);

    /// Invoked from the FSM thread as a handler to the user start request
    /// event, specified as `eventSp`.  This method starts the user event
    /// queue, sets the specified `status` and releases the specified
    /// `semaphore`. If not empty, `span` is the Distributed Trace span
    /// representing the operation of starting the session.
    void doStart(bslmt::Semaphore*                     semaphore,
                 int*                                  status,
                 const bsl::shared_ptr<Event>&         eventSp,
                 const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Invoked from the FSM thread as a handler to the user stop request
    /// event, specified as `eventSp`.  This method initiates session stop
    /// procedure. If not empty, `span` is the Distributed Trace span
    /// representing the operation of stopping the session.
    void doStop(const bsl::shared_ptr<Event>&         eventSp,
                const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Invoked from the FSM thread as a handler to the user open queue
    /// request event, specified as `eventSp`.  This method initiates queue
    /// opening procedure.  If not empty, `span` is the Distributed Trace
    /// span representing the operation of opening the queue.
    void doOpenQueue(const bsl::shared_ptr<Queue>&         queue,
                     const bsls::TimeInterval              timeout,
                     const FsmCallback&                    fsmCallback,
                     const bsl::shared_ptr<Event>&         eventSp,
                     const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Invoked from the FSM thread as a handler to the user configure queue
    /// request event, specified as `eventSp`.  This method initiates queue
    /// configure procedure. If not empty, `span` is the Distributed Trace
    /// span representing the operation of configuring the queue.
    void doConfigureQueue(const bsl::shared_ptr<Queue>&         queue,
                          const bmqt::QueueOptions&             options,
                          const bsls::TimeInterval              timeout,
                          const FsmCallback&                    fsmCallback,
                          const bsl::shared_ptr<Event>&         eventSp,
                          const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Invoked from the FSM thread as a handler to the user close queue
    /// request event, specified as `eventSp`.  This method initiates queue
    /// closing procedure. If not empty, `span` is the Distributed Trace
    /// span representing the operation of closing the queue.
    void doCloseQueue(const bsl::shared_ptr<Queue>&         queue,
                      const bsls::TimeInterval              timeout,
                      const FsmCallback&                    fsmCallback,
                      const bsl::shared_ptr<Event>&         eventSp,
                      const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// Invoked from the FSM thread as a handler to the channel status event
    /// specified as `eventSp` and sent by the IO thread.  This method
    /// updates the channel status and sends related user events.
    void doSetChannel(const bsl::shared_ptr<mwcio::Channel> channel,
                      const bsl::shared_ptr<Event>&         eventSp);

    /// Invoked from the FSM thread as a handler to the session start
    /// timeout event specified as `eventSp` and sent by scheduler thread.
    void doHandleStartTimeout(const bsl::shared_ptr<Event>& eventSp);

    /// Invoked from the FSM thread as a handler to the pending PUT
    /// expiration timeout event specified as `eventSp` and sent by
    /// the scheduler thread.
    void
    doHandlePendingPutExpirationTimeout(const bsl::shared_ptr<Event>& eventSp);

    /// Invoked from the FSM thread as a handler to the channel watermark
    /// event specified as `eventSp` with the specified watermark `type`
    /// sent by the IO thread.
    void doHandleChannelWatermark(mwcio::ChannelWatermarkType::Enum type,
                                  const bsl::shared_ptr<Event>&     eventSp);

    /// Invoked from the FSM thread as a handler to the heartbeat event
    /// specified as `eventSp` sent by the IO thread.
    void doHandleHeartbeat(const bsl::shared_ptr<Event>& eventSp);

    /// Invoked from the FSM thread to start channel closing.
    void disconnectChannel();

    /// Reset the state of the brokerSession, so that start will work again
    void resetState();

    /// Send disconnect request to the broker.
    bmqt::GenericResult::Enum disconnectBroker();

    /// Return an open queue request context for the specified `queue` and
    /// with the specified `fsmCallback` to be called as the request's
    /// `AsyncNotifierCb`.  The specified `isReopen` flag is used to set
    /// user event type (QUEUE_OPEN_RESULT or QUEUE_REOPEN_RESULT). The
    /// specified `isBuffered` flag is used to set request group id.
    RequestManagerType::RequestSp
    createOpenQueueContext(const bsl::shared_ptr<Queue>& queue,
                           const FsmCallback&            fsmCallback,
                           bool                          isBuffered);

    /// Return a configure queue request context for the specified `queue`
    /// and with the specified `options`.  The specified `isDeconfigure`
    /// flag is used to set specific consumer priority settings.  The
    /// specified `isBuffered` flag is used to set request group id.
    RequestManagerType::RequestSp
    createConfigureQueueContext(const bsl::shared_ptr<Queue>& queue,
                                const bmqt::QueueOptions&     options,
                                bool                          isDeconfigure,
                                bool                          isBuffered);

    /// Return a configure queue request context for the specified `queue`
    /// and with the specified `options` and `fsmCallback` to be called as
    /// the request's `AsyncNotifierCb`.
    RequestManagerType::RequestSp
    createStandaloneConfigureQueueContext(const bsl::shared_ptr<Queue>& queue,
                                          const bmqt::QueueOptions& options,
                                          const FsmCallback& fsmCallback);

    /// Return a close queue request context with the specified
    /// `fsmCallback` to be called as the request's `AsyncNotifierCb`.
    RequestManagerType::RequestSp
    createCloseQueueContext(const FsmCallback& fsmCallback);

    /// Returns whether or not the machine the application is running on is
    /// considered healthy.
    bool isHostHealthy() const;

    /// Invoked by `d_sessionOptions.d_hostHealthMonitor` upon detection of
    /// a change in host health.
    void onHostHealthStateChange(bmqt::HostHealthState::Enum state);

    /// Processes change of host health state on FSM thread. Suspends or
    /// resumes all queues that are configured accordingly.
    void doHandleHostHealthStateChange(bmqt::HostHealthState::Enum   state,
                                       const bsl::shared_ptr<Event>& eventSp);

    /// Initiates the suspension of all queues that are sensitive to the
    /// health of the application host.
    void actionSuspendHealthSensitiveQueues();

    /// Initiates the resumption of all queues that are sensitive to the
    /// health of the application host.
    void actionResumeHealthSensitiveQueues();

    /// Create and submit event to the FSM queue with the specified
    /// `fsmEntrance` method.  Return `0` upon success in which case it is
    /// guaranteed that the specified `fsmCallback` will be called upon
    /// completion of the operation regardless whether it is synchronous or
    /// asynchronous.  If the specified `everCallBack` is `true`, also
    /// execute `fsmCallback` upon failure cases.  If `everCallBack` is
    /// `false`, the caller relies on return code; do not execute
    /// `fsmCallback`, just return code.
    int toFsm(const bmqimp::BrokerSession::FsmCallback& fsmCallback,
              const EventCallback&                      fsmEntrance,
              bool                                      everCallBack = true);

    /// Write the specified `blob` into the channel providing the specified
    /// channel `highWaterMark` value.  If a special extention buffer is not
    /// empty append the `blob` into the buffer and return success.  If the
    /// write operation fails with e_LIMIT error which indicates HWM
    /// condition put the specified `blob` into the buffer and return
    /// success.  In all other cases return the result of the write
    /// operation.
    bmqt::GenericResult::Enum
    requestWriterCb(const RequestManagerType::RequestSp& context,
                    const bmqp::QueueId&                 queueId,
                    const bsl::shared_ptr<bdlbb::Blob>&  blob,
                    bsls::Types::Int64                   highWatermark);

    /// Write the specified `blob` into the channel providing the specified
    /// channel `highWaterMark` value.  If the write operation fails with
    /// the e_LIMIT error put the `blob` into the extention buffer.  Return
    /// success status or error code in case of write failure due to any
    /// error other than e_LIMIT.
    bmqt::GenericResult::Enum writeOrBuffer(const bdlbb::Blob& eventBlob,
                                            bsls::Types::Int64 highWaterMark);

    bool acceptUserEvent(const bdlbb::Blob&        eventBlob,
                         const bsls::TimeInterval& timeout);

    void setupPutExpirationTimer(const bsls::TimeInterval& timeout);

    void
    removePendingControlMessage(const RequestManagerType::RequestSp& context);

    /// If Distributed Trace is configured by the session options, this will
    /// return an object which sets `span` to the active span (as defined by
    /// the options' "trace context" object) on its construction, and
    /// reverts the active span to its prior state on destruction.
    bslma::ManagedPtr<void>
    activateDTSpan(const bsl::shared_ptr<bmqpi::DTSpan>& span);

    /// If Distributed Trace is configured by the session options, create a
    /// new span, as a child of the one currently active.  Otherwise, return
    /// an empty `shared_ptr`.  The specified `operation` and the specified
    /// `baggage` are used for creation.
    bsl::shared_ptr<bmqpi::DTSpan>
    createDTSpan(bsl::string_view              operation,
                 const bmqpi::DTSpan::Baggage& baggage =
                     bmqpi::DTSpan::Baggage()) const;

    /// True if the session is started.
    bool isStarted() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BrokerSession, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `scheduler` and
    /// `bufferFactory`.  Use the configuration from the specified
    /// `sessionOptions`.  If the specified `eventHandlerCb` is defined,
    /// invoke it for any events.  Invoke the specified `stateCb` when
    /// the session makes state transition.  All memory allocations will be
    /// done using the specified `allocator`.
    BrokerSession(bdlmt::EventScheduler*                  scheduler,
                  bdlbb::BlobBufferFactory*               bufferFactory,
                  const bmqt::SessionOptions&             sessionOptions,
                  const EventQueue::EventHandlerCallback& eventHandlerCb,
                  const StateFunctor&                     stateCb,
                  bslma::Allocator*                       allocator);

    /// Destructor
    ~BrokerSession();

    // MANIPULATORS

    /// Configure this component to keep track of statistics: create a
    /// sub-context from the specified `rootStatContext`, using the
    /// specified `start` and `end` snapshot location.
    void initializeStats(mwcst::StatContext* rootStatContext,
                         const mwcst::StatValue::SnapshotLocation& start,
                         const mwcst::StatValue::SnapshotLocation& end);

    /// Process the specified `packet`.  This method gets called each time a
    /// new `packet` (sent by the broker) is available on the channel.  It
    /// then acts as a dispatcher to decipher and process the `packet`.
    /// Return error status in case the packet cannot be handled by any
    /// reason.
    bmqt::GenericResult::Enum processPacket(const bdlbb::Blob& packet);

    /// Set the specified `channel` to use for communication with the
    /// bmqbrkr.  If `channel` is non null, this is a newly-established
    /// `channel`.  Otherwise, if `channel` is null, this means the
    /// connection with the broker was lost.
    void setChannel(const bsl::shared_ptr<mwcio::Channel>& channel);

    /// Start the broker session and block until start result or the
    /// specified `timeout` is expired.  Return 0 on success or non-zero on
    /// any case of failure.
    int start(const bsls::TimeInterval& timeout);

    /// Start the broker session in a non-blocking mode.  Returns 0 on
    /// success or non-zero on any case of failure.
    int startAsync();

    /// Initiate a stop of the broker session.  Return after the session is
    /// fully stopped.
    void stop();

    /// Initiate a stop of the broker session.  Return immediately after
    /// starting the stop process.
    void stopAsync();

    /// Return the next event.  If the event queue is empty, block until an
    /// event is available or until the specified `timeout` (relative)
    /// expires. If the `timeout` expires, return a session event of type
    /// `TIMEOUT`.
    ///
    /// THREAD: This method is called from one of the APPLICATION threads.
    bsl::shared_ptr<bmqimp::Event>
    nextEvent(const bsls::TimeInterval& timeout);

    int openQueue(const bsl::shared_ptr<Queue>& queue,
                  bsls::TimeInterval            timeout);

    void openQueueSync(const bsl::shared_ptr<Queue>& queue,
                       bsls::TimeInterval            timeout,
                       const EventCallback&          eventCallback);

    int openQueueAsync(const bsl::shared_ptr<Queue>& queue,
                       bsls::TimeInterval            timeout,
                       const EventCallback& eventCallback = EventCallback());

    int configureQueue(const bsl::shared_ptr<Queue>& queue,
                       const bmqt::QueueOptions&     options,
                       bsls::TimeInterval            timeout);

    void configureQueueSync(const bsl::shared_ptr<Queue>& queue,
                            const bmqt::QueueOptions&     options,
                            bsls::TimeInterval            timeout,
                            const EventCallback&          eventCallback);

    /// Configure the specified `queue` using the specified `options` and
    /// wait for the specified `timeout` duration for the request to
    /// complete.  Set the optionally specified `eventCallback` on the event
    /// in order to have it executed at the conclusion of this operation.
    /// This operation returns error if there is a pending configure for the
    /// same queue.  The return value is one of the values defined in the
    /// `bmqt::ConfigureQueueResult::Enum` enum.
    int
    configureQueueAsync(const bsl::shared_ptr<Queue>& queue,
                        const bmqt::QueueOptions&     options,
                        bsls::TimeInterval            timeout,
                        const EventCallback& eventCallback = EventCallback());

    int closeQueue(const bsl::shared_ptr<Queue>& queue,
                   bsls::TimeInterval            timeout);

    void closeQueueSync(const bsl::shared_ptr<Queue>& queue,
                        bsls::TimeInterval            timeout,
                        const EventCallback&          eventCallback);

    int closeQueueAsync(const bsl::shared_ptr<Queue>& queue,
                        bsls::TimeInterval            timeout,
                        const EventCallback& eventCallback = EventCallback());

    int post(const bdlbb::Blob& eventBlob, const bsls::TimeInterval& timeout);

    int confirmMessage(const bsl::shared_ptr<bmqimp::Queue>& queue,
                       const bmqt::MessageGUID&              messageId,
                       const bsls::TimeInterval&             timeout);

    int confirmMessages(const bdlbb::Blob&        blob,
                        const bsls::TimeInterval& timeout);

    void postToFsm(const bsl::function<void()>& f);

    void enqueueSessionEvent(
        bmqt::SessionEventType::Enum  type,
        int                           statusCode       = 0,
        const bslstl::StringRef&      errorDescription = "",
        const bmqt::CorrelationId&    correlationId    = bmqt::CorrelationId(),
        const bsl::shared_ptr<Queue>& queue         = bsl::shared_ptr<Queue>(),
        const EventCallback&          eventCallback = EventCallback());

    /// Efficiently return a shared pointer to `bmqimp::Event`.
    bsl::shared_ptr<Event> createEvent();

    /// Invoked when the asynchronous start operation timedout.
    void onStartTimeout();

    /// Invoked when pending PUT expiration timeout fires.
    void onPendingPutExpirationTimeout();

    /// Process the specified dump `command`.
    void processDumpCommand(const bmqp_ctrlmsg::DumpMessages& command);

    /// Put empty event into the FSM queue.  Return true if the event is
    /// accepted and handled in the FSM thread, false otherwise.  Used for
    /// testing purposes only.
    bool _synchronize();

    // ACCESSORS

    /// Return true if the session is configured with a session event
    /// handler callback, for event processing; false if the user didn't
    /// specify one (and will use nextEvent).
    bool isUsingSessionEventHandler() const;

    /// Return the state of the broker session.
    State::Enum state() const;

    /// Lookup the queue with the specified `queueId`, or `uri`, or
    /// `correlationId`, and return a shared pointer to the Queue object (if
    /// found), or an empty shared pointer (if not found).
    bsl::shared_ptr<Queue> lookupQueue(const bmqt::Uri& uri) const;
    bsl::shared_ptr<Queue>
    lookupQueue(const bmqt::CorrelationId& correlationId) const;
    bsl::shared_ptr<Queue> lookupQueue(const bmqp::QueueId& queueId) const;

    /// Return transition table of the Session FSM.
    bsl::vector<StateTransition> getSessionFsmTransitionTable() const;

    void handleChannelWatermark(mwcio::ChannelWatermarkType::Enum type);
    // Called when the channel watermark event happens.  The watermark type
    // is indicated by the specified 'type'.

    /// Print the statistics of this `BrokerSession` to the specified
    /// `stream`.  If the specified `includeDelta` is true, the printed
    /// report will include delta statistics (if any) representing
    /// variations since the last print.  The behavior is undefined unless
    /// the statistics were initialized by a call to `initializeStats`.
    void printStats(bsl::ostream& stream, bool includeDelta) const;
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&              stream,
                         BrokerSession::State::Enum value);

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                      stream,
                         BrokerSession::QueueFsmEvent::Enum value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&                      stream,
                                bmqimp::BrokerSession::State::Enum value)
{
    return bmqimp::BrokerSession::State::print(stream, value, 0, -1);
}

inline bsl::ostream& operator<<(bsl::ostream&                         stream,
                                bmqimp::BrokerSession::FsmEvent::Enum value)
{
    return bmqimp::BrokerSession::FsmEvent::print(stream, value, 0, -1);
}

inline bsl::ostream&
operator<<(bsl::ostream&                              stream,
           bmqimp::BrokerSession::QueueFsmEvent::Enum value)
{
    return bmqimp::BrokerSession::QueueFsmEvent::print(stream, value, 0, -1);
}
// ----------------
// class SessionFsm
// ----------------

// ACCESSORS
inline BrokerSession::State::Enum BrokerSession::SessionFsm::state() const
{
    return static_cast<BrokerSession::State::Enum>(d_state.load());
}

inline const bsl::vector<BrokerSession::StateTransition>&
BrokerSession::SessionFsm::table() const
{
    return d_transitionTable;
}

// --------------
// class QueueFsm
// --------------

// ACCESSORS
inline const bsl::vector<BrokerSession::QueueStateTransition>&
BrokerSession::QueueFsm::table() const
{
    return d_transitionTable;
}

// -------------------
// class BrokerSession
// -------------------

inline bsl::shared_ptr<Event> BrokerSession::createEvent()
{
    bsl::shared_ptr<Event> event = d_eventPool.getObject();
    event->setMessageCorrelationIdContainer(&d_messageCorrelationIdContainer);
    return event;
}

inline bool BrokerSession::isUsingSessionEventHandler() const
{
    return d_usingSessionEventHandler;
}

inline BrokerSession::State::Enum BrokerSession::state() const
{
    return d_sessionFsm.state();
}

inline bsl::vector<BrokerSession::StateTransition>
BrokerSession::getSessionFsmTransitionTable() const
{
    return d_sessionFsm.table();
}

inline bool BrokerSession::isStarted() const
{
    return d_sessionFsm.state() == State::e_STARTED;
}

}  // close package namespace
}  // close enterprise namespace

#endif
