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

// bmqimp_brokersession.cpp                                           -*-C++-*-
#include <bmqimp_brokersession.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_negotiatedchannelfactory.h>
#include <bmqimp_queue.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_ackmessageiterator.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_controlmessageutil.h>
#include <bmqp_event.h>
#include <bmqp_eventutil.h>
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_routingconfigurationutils.h>
#include <bmqt_messageguid.h>
#include <bmqt_uri.h>

#include <bmqio_channel.h>
#include <bmqio_status.h>
#include <bmqst_statcontext.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdld_datum.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>  // for bsl::memcpy
#include <bsl_iostream.h>
#include <bsl_list.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>
#include <bslmt_readlockguard.h>
#include <bslmt_semaphore.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_systemclocktype.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace bmqimp {

namespace {
// CONSTANTS

/// To make sure control messages will go through, we give them a higher
/// watermark than data messages, so that writing on the channel will
/// succeed.  The following defines how many bytes we provision for it.
const int k_CONTROL_DATA_WATERMARK_EXTRA = 4 * 1024 * 1024;  // 4 MB

/// Upper-bound for the initial capacity of the event queue
const int k_EVENTQUEUE_INITIAL_CAPACITY = 10 * 1000;

/// Initial capacity of the FSM event queue
const int k_FSMQUEUE_INITIAL_CAPACITY = 1000;

/// RequestManager group id for non buffered requests.
/// The request is buffered if it is kept after CHANNEL_DOWN event, and is
/// retransmitted once the channel restores.  The non buffered requests are
/// cancelled on CHANNEL_DOWN.
/// The following table lists buffered and non buffered requests:
///
/// +-----+-----------------------+--------------+--------------+
/// | No. | Request type          | Buffered     | Non buffered |
/// +-----+-----------------------+--------------+--------------+
/// | 1   | Open                  |      x       |              |
/// +-----+-----------------------+--------------+--------------+
/// | 2   | Open-Configure        |      x       |              |
/// +-----+-----------------------+--------------+--------------+
/// | 3   | Configure             |      x       |              |
/// +-----+-----------------------+--------------+--------------+
/// | 4   | Reconfigure (update)  |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 5   | Reopen                |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 6   | Reopen-Configure      |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 7   | Deconfigure (suspend) |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 8   | Configure (resume)    |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 9   | Close-Deconfigure     |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 10  | Close                 |              |      x       |
/// +-----+-----------------------+--------------+--------------+
/// | 11  | Disconnect            |              |      x       |
/// +-----+-----------------------+--------------+--------------+
///
const int k_NON_BUFFERED_REQUEST_GROUP_ID = 0;

const char k_ENQUEUE_ERROR_TEXT[] = "Failed to process the operation, the"
                                    " session is already being destroyed";

/// Create an `Event` object at the specified `address` using the supplied
/// `allocator`.  This is used by the ObjectPool.
void poolCreateEvent(void*                     address,
                     bdlbb::BlobBufferFactory* bufferFactory,
                     bslma::Allocator*         allocator)
{
    new (address) Event(bufferFactory, allocator);
}

void callbackAdapter(
    const bsl::function<void()>& f,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // See the comment in 'postToFsm'.
    f();
}

void eventCallbackAdapter(bslmt::Semaphore*                   semaphore,
                          const bmqimp::Event::EventCallback& eventCallback,
                          const bsl::shared_ptr<Event>&       eventSp)
{
    BSLS_ASSERT_SAFE(semaphore);

    eventCallback(eventSp);
    semaphore->post();
}

/// Sets `queueSp` to suspended state, blocking all subsequent PUTs and
/// configures issued by the client.
void applyQueueSuspension(const bsl::shared_ptr<Queue>& queueSp, bool value)
{
    // executed by one of the *APPLICATION* threads

    if (queueSp) {
        queueSp->setIsSuspended(value);
    }
}

void releaseSemaphore(
    bslmt::Semaphore*            semaphore,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(semaphore);

    semaphore->post();
}

/// Fills `baggage` with metadata from `queue` that should be added to every
/// Distributed Trace span which represents an operation on a queue.
void fillDTSpanQueueBaggage(bmqpi::DTSpan::Baggage* baggage,
                            const bmqimp::Queue&    queue)
{
    baggage->put("bmq.queue.uri", queue.uri().asString());
}

BSLA_MAYBE_UNUSED
bool isConfigure(const bmqp_ctrlmsg::ControlMessage& request,
                 const bmqp_ctrlmsg::ControlMessage& response)
{
    return request.choice().isConfigureStreamValue()
               ? response.choice().isConfigureStreamResponseValue()
           : request.choice().isConfigureQueueStreamValue()
               ? response.choice().isConfigureQueueStreamResponseValue()
               : false;
}

BSLA_MAYBE_UNUSED
bool isConfigure(const bmqp_ctrlmsg::ControlMessage& request)
{
    return request.choice().isConfigureStreamValue() ||
           request.choice().isConfigureQueueStreamValue();
}

BSLA_MAYBE_UNUSED
bool isConfigureResponse(const bmqp_ctrlmsg::ControlMessage& response)
{
    return response.choice().isConfigureStreamResponseValue() ||
           response.choice().isConfigureQueueStreamResponseValue();
}

void makeDeconfigure(bmqp_ctrlmsg::ControlMessage* request)
{
    if (request->choice().isConfigureStreamValue()) {
        request->choice()
            .configureStream()
            .streamParameters()
            .subscriptions()
            .clear();
    }
    else {
        bmqp_ctrlmsg::QueueStreamParameters& streamParams =
            request->choice().configureQueueStream().streamParameters();

        streamParams.maxUnconfirmedMessages() = 0;
        streamParams.maxUnconfirmedBytes()    = 0;
        streamParams.consumerPriorityCount()  = 0;
        streamParams.consumerPriority() =
            bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;
    }
}

}  // close unnamed namespace

// ------------
// struct State
// ------------

bsl::ostream& BrokerSession::State::print(bsl::ostream&              stream,
                                          BrokerSession::State::Enum value,
                                          int                        level,
                                          int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << State::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* BrokerSession::State::toAscii(BrokerSession::State::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(STARTING)
        CASE(STARTED)
        CASE(RECONNECTING)
        CASE(CLOSING_SESSION)
        CASE(CLOSING_CHANNEL)
        CASE(STOPPED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ---------------
// struct FsmEvent
// ---------------

bsl::ostream&
BrokerSession::FsmEvent::print(bsl::ostream&                 stream,
                               BrokerSession::FsmEvent::Enum value,
                               int                           level,
                               int                           spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << FsmEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
BrokerSession::FsmEvent::toAscii(BrokerSession::FsmEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(START)
        CASE(START_FAILURE)
        CASE(START_TIMEOUT)
        CASE(STOP)
        CASE(CHANNEL_UP)
        CASE(CHANNEL_DOWN)
        CASE(SESSION_DOWN)
        CASE(HOST_UNHEALTHY)
        CASE(HOST_HEALTHY)
        CASE(ALL_QUEUES_RESUMED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// --------------------
// struct QueueFsmEvent
// --------------------

bsl::ostream&
BrokerSession::QueueFsmEvent::print(bsl::ostream&                      stream,
                                    BrokerSession::QueueFsmEvent::Enum value,
                                    int                                level,
                                    int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << QueueFsmEvent::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
BrokerSession::QueueFsmEvent::toAscii(BrokerSession::QueueFsmEvent::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(OPEN_CMD)
        CASE(CONFIG_CMD)
        CASE(CLOSE_CMD)
        CASE(REQ_NOT_SENT)
        CASE(RESP_OK)
        CASE(LATE_RESP)
        CASE(RESP_BAD)
        CASE(RESP_TIMEOUT)
        CASE(RESP_EXPIRED)
        CASE(CHANNEL_DOWN)
        CASE(CHANNEL_UP)
        CASE(SUSPEND)
        CASE(RESUME)
        CASE(REQ_CANCELED)
        CASE(SESSION_DOWN)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ----------------
// class SessionFsm
// ----------------

bmqt::GenericResult::Enum
BrokerSession::SessionFsm::setState(State::Enum value, FsmEvent::Enum event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bool        isValidTransition = false;
    State::Enum oldState          = state();

    BALL_LOG_INFO << "::: STATE TRANSITION: " << oldState << " -> [" << event
                  << "] -> " << value << " :::";

    for (size_t i = 0; i < d_transitionTable.size(); ++i) {
        if (d_transitionTable[i].d_currentState == oldState &&
            d_transitionTable[i].d_event == event &&
            d_transitionTable[i].d_newState == value) {
            isValidTransition = true;
            break;  // BREAK
        }
    }
    BSLS_ASSERT_OPT(isValidTransition && "Invalid transition");

    d_state = value;

    return d_session.d_stateCb(oldState, state(), event);
}

void BrokerSession::SessionFsm::setStarted(
    FsmEvent::Enum                         event,
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(channel && "NULL channel");

    d_session.d_channel_sp = channel;

    setState(State::e_STARTED, event);
    d_onceConnected            = true;
    d_session.d_acceptRequests = true;

    // Post on the semaphore (to wake-up a sync 'start', if any)
    d_session.d_startSemaphore.post();

    // Temporary safety switch to control configure request.
    d_session.d_channel_sp->properties().load(
        &d_session.d_doConfigureStream,
        NegotiatedChannelFactory::k_CHANNEL_PROPERTY_CONFIGURE_STREAM);
}

bmqt::GenericResult::Enum
BrokerSession::SessionFsm::setStarting(FsmEvent::Enum event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bmqt::GenericResult::Enum res = setState(State::e_STARTING, event);
    // STARTING enter logic may go here
    return res;
}

void BrokerSession::SessionFsm::setReconnecting(FsmEvent::Enum event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(state() == State::e_STARTED);

    setState(State::e_RECONNECTING, event);

    // Connection went down, so reset 'd_numPendingReopenQueues'.
    d_session.d_numPendingReopenQueues = 0;

    // Do not cancel pending PUT messages, they will be retransmitted once the
    // channel is up.
    // Call expiration timer handler immediately.  It will setup the next timer
    // event to track expired pending PUT messages while the session has no
    // connection.
    d_session.doHandlePendingPutExpirationTimeout(bsl::shared_ptr<Event>());

    // Enqueue a CONNECTION_LOST event
    d_session.enqueueSessionEvent(bmqt::SessionEventType::e_CONNECTION_LOST);
}

bmqt::GenericResult::Enum
BrokerSession::SessionFsm::setClosingSession(FsmEvent::Enum event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(state() == State::e_STARTED);

    setState(State::e_CLOSING_SESSION, event);

    // Cancel all pending PUT messages, before cancelling any
    // outstanding requests
    bsl::shared_ptr<Queue> noQueue;
    d_session.cancelPendingMessages(noQueue);

    // Cancel all requests (buffered and non-buffered)
    d_session.cancel(bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED,
                     "The request was canceled [reason: disconnected]");

    // All in-progress reopen requests should be cancelled
    BSLS_ASSERT_SAFE(d_session.d_numPendingReopenQueues == 0);

    return d_session.disconnectBroker();
}

void BrokerSession::SessionFsm::setClosingChannel(FsmEvent::Enum event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    setState(State::e_CLOSING_CHANNEL, event);
    d_session.disconnectChannel();
}

void BrokerSession::SessionFsm::setStopped(FsmEvent::Enum event,
                                           bool           isStartTimeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    setState(State::e_STOPPED, event);

    if (isStartTimeout) {
        // The session never reached STARTED
        BSLS_ASSERT_SAFE(!d_onceConnected);

        // Enqueue a connection timeout event
        d_session.enqueueSessionEvent(
            bmqt::SessionEventType::e_CONNECTION_TIMEOUT,
            -1,
            "The connection to bmqbrkr timedout");
    }
    else if (d_onceConnected) {
        d_session.enqueueSessionEvent(bmqt::SessionEventType::e_DISCONNECTED);
        d_onceConnected = false;
    }

    // Cancel all pending PUT messages, before cancelling any outstanding
    // requests.  Although there is similar cancellation in the
    // 'setClosingSession' it also should be done here to cover 'RECONNECTING
    // -> STOPPED' transition, where user stops the reconnecting session that
    // may have pending messages and requests.
    bsl::shared_ptr<Queue> noQueue;
    d_session.cancelPendingMessages(noQueue);

    // Cancel all requests (buffered and non-buffered)
    d_session.cancel(bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED,
                     "The request was canceled [reason: disconnected]");

    // Cancel pending PUTs expiration timer
    d_session.d_scheduler_p->cancelEvent(
        &d_session.d_messageExpirationTimeoutHandle);

    // The session is fully stopped, we can now reset its state to release any
    // references to objects (queues, ...) it may still hold.
    d_session.resetState();

    // Enqueue a special internal DISCONNECTED event with the status code set
    // to -1.  This will trigger the EventHandler thread to release the stop
    // semaphore.  If the 'sessionEventHandler' is not used ('nextEvent' mode)
    // then release the sempahore right here.
    // NOTE: Once we post on the d_stopSemaphore 'this' might be destroyed
    //       (e.g. stop, called from the dtor is waiting on that semaphore).
    //       Therefore, we *MUST* ensure no data member are used or alive
    //       beyond that 'post'.
    if (d_session.d_usingSessionEventHandler) {
        d_session.enqueueSessionEvent(bmqt::SessionEventType::e_DISCONNECTED,
                                      -1);
    }
    else {
        d_session.d_stopSemaphore.post();
    }
}

BrokerSession::SessionFsm::SessionFsm(BrokerSession& session)
: d_session(session)
, d_state(State::e_STOPPED)
, d_onceConnected(false)
, d_beginTimestamp(0)
{
    StateTransition table[] = {
#define S State
#define E FsmEvent

        // current state       event                    new state
        {S::e_STOPPED, E::e_START, S::e_STARTING},
        {S::e_STOPPED, E::e_STOP, S::e_STOPPED},
        //
        {S::e_STARTING, E::e_START_TIMEOUT, S::e_STOPPED},
        {S::e_STARTING, E::e_START_FAILURE, S::e_STOPPED},
        {S::e_STARTING, E::e_STOP, S::e_STOPPED},
        {S::e_STARTING, E::e_CHANNEL_UP, S::e_STARTED},
        {S::e_STARTING, E::e_CHANNEL_DOWN, S::e_STOPPED},
        {S::e_STARTING, E::e_HOST_UNHEALTHY, S::e_STARTING},
        {S::e_STARTING, E::e_HOST_HEALTHY, S::e_STARTING},
        {S::e_STARTING, E::e_ALL_QUEUES_RESUMED, S::e_STARTING},
        //
        {S::e_STARTED, E::e_STOP, S::e_CLOSING_SESSION},
        {S::e_STARTED, E::e_CHANNEL_DOWN, S::e_RECONNECTING},
        {S::e_STARTED, E::e_HOST_UNHEALTHY, S::e_STARTED},
        {S::e_STARTED, E::e_HOST_HEALTHY, S::e_STARTED},
        {S::e_STARTED, E::e_ALL_QUEUES_RESUMED, S::e_STARTED},
        //
        {S::e_RECONNECTING, E::e_STOP, S::e_STOPPED},
        {S::e_RECONNECTING, E::e_CHANNEL_UP, S::e_STARTED},
        {S::e_RECONNECTING, E::e_HOST_UNHEALTHY, S::e_RECONNECTING},
        {S::e_RECONNECTING, E::e_HOST_HEALTHY, S::e_RECONNECTING},
        {S::e_RECONNECTING, E::e_ALL_QUEUES_RESUMED, S::e_RECONNECTING},
        //
        {S::e_CLOSING_SESSION, E::e_SESSION_DOWN, S::e_CLOSING_CHANNEL},
        {S::e_CLOSING_SESSION, E::e_CHANNEL_DOWN, S::e_STOPPED},
        //
        {S::e_CLOSING_CHANNEL, E::e_CHANNEL_DOWN, S::e_STOPPED}

#undef S
#undef E
    };

    const int size    = sizeof(table) / sizeof(table[0]);
    d_transitionTable = bsl::vector<StateTransition>(table,
                                                     table + size,
                                                     d_session.d_allocator_p);
}

bmqt::GenericResult::Enum BrokerSession::SessionFsm::handleStartRequest()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum      event = FsmEvent::e_START;
    bmqt::GenericResult::Enum res   = bmqt::GenericResult::e_UNKNOWN;
    switch (state()) {
    case State::e_STARTED: {
        BALL_LOG_WARN << "Session is already started";

        // Post on the semaphore (to wake-up a sync 'start', if any)
        d_session.d_startSemaphore.post();
        res = bmqt::GenericResult::e_SUCCESS;
    } break;
    case State::e_STARTING: {
        BALL_LOG_ERROR << "Session is already being started";
        res = bmqt::GenericResult::e_NOT_SUPPORTED;
    } break;
    case State::e_RECONNECTING: {
        BALL_LOG_ERROR << "Broker is not connected";
        res = bmqt::GenericResult::e_NOT_CONNECTED;
    } break;
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL: {
        BALL_LOG_ERROR << "Session is being stopped";
        res = bmqt::GenericResult::e_NOT_SUPPORTED;
    } break;
    case State::e_STOPPED: {
        d_beginTimestamp = bmqsys::Time::highResolutionTimer();
        res              = setStarting(event);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
    return res;
}

void BrokerSession::SessionFsm::handleStartTimeout()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_START_TIMEOUT;

    switch (state()) {
    case State::e_STARTING: {
        BALL_LOG_ERROR << "Start (ASYNC) has timed out";
        setStopped(event, true);
        logOperationTime("Start");
    } break;
    case State::e_STARTED: {
        // We got connected just on time :) .. do nothing
    } break;
        // The following fallthrough are intentional
    case State::e_RECONNECTING:
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << "::: UNEXPECTED START_TIMEOUT IN STATE " << state()
                       << " :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleStartSynchronousFailure()
{
    // executed by the FSM thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_START_FAILURE;

    switch (state()) {
    case State::e_STARTING: {
        BALL_LOG_ERROR << "Start (ASYNC) has failed";

        BSLS_ASSERT_SAFE(!d_onceConnected);
        setStopped(event, false);
        // Should not enqueue user events since this failure is returned by
        // 'start'.

        logOperationTime("Start");
    } break;
    case State::e_STARTED:
    case State::e_RECONNECTING:
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << "::: UNEXPECTED START_FAILURE IN STATE " << state()
                       << " :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleStopRequest()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_STOP;

    switch (state()) {
    case State::e_STARTED: {
        d_beginTimestamp = bmqsys::Time::highResolutionTimer();

        bmqt::GenericResult::Enum res = setClosingSession(event);
        if (res != bmqt::GenericResult::e_SUCCESS) {
            BALL_LOG_ERROR << "::: FAILED TO DISCONNECT BROKER GRACEFULLY :::";
            setClosingChannel(FsmEvent::e_SESSION_DOWN);
        }
    } break;
    case State::e_STARTING: {
        // `d_channel_sp` is _not_ set in the STARTING state (only in STARTED).
        // Therefore, simply transition to STOPPED.
        setStopped(event);

        logOperationTime("Start");
    } break;
    case State::e_RECONNECTING: {
        setStopped(event);
    } break;
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL: {
        BALL_LOG_INFO << "::: STOP IN PROGRESS :::";
    } break;
    case State::e_STOPPED: {
        BALL_LOG_INFO << "::: ALREADY STOPPED :::";
        // trigger stateCb
        setStopped(event);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleChannelUp(
    const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_CHANNEL_UP;

    switch (state()) {
    case State::e_STARTING: {
        setStarted(event, channel);
        d_session.enqueueSessionEvent(bmqt::SessionEventType::e_CONNECTED);
        logOperationTime("Start");
    } break;
    case State::e_RECONNECTING: {
        setStarted(event, channel);
        d_session.enqueueSessionEvent(bmqt::SessionEventType::e_RECONNECTED);
        // Re-open all queues which were opened before connection dropped
        d_session.reopenQueues();
    } break;
    case State::e_STARTED:
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << "::: UNEXPECTED CHANNEL_UP IN STATE " << state()
                       << " :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleChannelDown()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_CHANNEL_DOWN;

    d_session.d_channel_sp.reset();

    // Remove all pending blobs from the blob queue
    d_session.d_extensionBlobBuffer.clear();

    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_session.d_extensionBufferLock);
        // LOCK
        d_session.d_extensionBufferEmpty = true;

        // Release any writing user threads
        d_session.d_extensionBufferCondition.broadcast();
    }

    switch (state()) {
    case State::e_STARTED: {
        setReconnecting(event);
    } break;
    case State::e_STARTING: {
        setStopped(event);
    } break;
    case State::e_CLOSING_SESSION: {
        BALL_LOG_WARN << "CHANNEL_DOWN while closing session";
        setStopped(event);
        logOperationTime("Stop");
    } break;
    case State::e_CLOSING_CHANNEL: {
        setStopped(event);
        logOperationTime("Stop");
    } break;
    case State::e_RECONNECTING:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << "::: UNEXPECTED CHANNEL_DOWN IN STATE " << state()
                       << " :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleSessionClosed()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_SESSION_DOWN;

    switch (state()) {
    case State::e_CLOSING_SESSION: {
        setClosingChannel(event);
    } break;
    case State::e_STARTED:
    case State::e_STARTING:
    case State::e_CLOSING_CHANNEL:
    case State::e_RECONNECTING:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << "::: UNEXPECTED SESSION_DOWN WHILE " << state()
                       << " :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleHostUnhealthy()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_HOST_UNHEALTHY;

    switch (state()) {
    case State::e_STARTED:
    case State::e_STARTING:
    case State::e_RECONNECTING: {
        setState(state(), event);
        d_session.actionSuspendHealthSensitiveQueues();
    } break;
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << " ::: UNEXPECTED HOST-HEALTH NOTIFICATION AFTER "
                       << "INITIATING SESSION CLOSURE :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleHostHealthy()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_HOST_HEALTHY;

    switch (state()) {
    case State::e_STARTED:
    case State::e_STARTING:
    case State::e_RECONNECTING: {
        setState(state(), event);
        d_session.actionResumeHealthSensitiveQueues();
    } break;
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_ERROR << " ::: UNEXPECTED HOST-HEALTH NOTIFICATION AFTER "
                       << "STOPPING SESSION :::";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::handleAllQueuesResumed()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const FsmEvent::Enum event = FsmEvent::e_ALL_QUEUES_RESUMED;

    switch (state()) {
    case State::e_STARTED:
    case State::e_STARTING:
    case State::e_RECONNECTING: {
        setState(state(), event);
        d_session.enqueueSessionEvent(
            bmqt::SessionEventType::e_HOST_HEALTH_RESTORED);
    } break;
    case State::e_CLOSING_SESSION:
    case State::e_CLOSING_CHANNEL:
    case State::e_STOPPED: {
        BALL_LOG_INFO << "Elided e_QUEUES_RESTORED event for closing session";
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Session state");
    } break;
    }
}

void BrokerSession::SessionFsm::logOperationTime(const char* operation)
{
    if (d_beginTimestamp) {
        const bsls::Types::Int64 elapsed =
            bmqsys::Time::highResolutionTimer() - d_beginTimestamp;
        BALL_LOG_INFO << operation << " took: "
                      << bmqu::PrintUtil::prettyTimeInterval(elapsed) << " ("
                      << elapsed << " nanoseconds)";
        d_beginTimestamp = 0;
    }
    else {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_WARN << "d_beginTimestamp was not initialized with timestamp";
    }
}
// --------------
// class QueueFsm
// --------------

void BrokerSession::QueueFsm::setQueueState(
    const bsl::shared_ptr<Queue>& queue,
    QueueState::Enum              value,
    QueueFsmEvent::Enum           event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bool             isValidTransition = false;
    QueueState::Enum oldState          = queue->state();

    BALL_LOG_INFO << "::: QUEUE STATE TRANSITION [qId=" << queue->id()
                  << "]: " << oldState << " -> [" << event << "] -> " << value
                  << " :::";

    for (size_t i = 0; i < d_transitionTable.size(); ++i) {
        if (d_transitionTable[i].d_currentState == oldState &&
            d_transitionTable[i].d_event == event &&
            d_transitionTable[i].d_newState == value) {
            isValidTransition = true;
            break;  // BREAK
        }
    }
    BSLS_ASSERT_OPT(isValidTransition && "Invalid transition");

    queue->setState(value);
}

void BrokerSession::QueueFsm::setQueueId(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

    // Set 'queueId' and 'subQueueId'
    bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID,
                          bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID);
    d_session.d_queueManager.generateQueueAndSubQueueId(&queueId,
                                                        queue->uri(),
                                                        queue->flags());
    queue->setId(queueId.id());
    if (queueId.subId() != bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID) {
        // We only set the subId on the 'Queue' object when we have a
        // non-default subId.  At the time of this writing, a non-default subId
        // corresponds to a fan-out consumer, and a default subId corresponds
        // to either a writer or a consumer not having an 'appId'
        // (i.e. non-fan-out consumer)
        queue->setSubQueueId(queueId.subId());
    }

    // Update context
    context->request().choice().openQueue().handleParameters() =
        queue->handleParameters();
}

void BrokerSession::QueueFsm::injectErrorResponse(
    const RequestManagerType::RequestSp& context,
    bmqp_ctrlmsg::StatusCategory::Value  status,
    const bslstl::StringRef&             reason)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Inject an error response which will be handled outside the Queue FSM
    bmqp::ControlMessageUtil::makeStatus(&context->response(),
                                         status,
                                         status,
                                         reason);
}

bmqt::OpenQueueResult::Enum BrokerSession::QueueFsm::actionOpenQueue(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context,
    const bsls::TimeInterval&            timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bmqt::OpenQueueResult::Enum res = d_session.sendOpenQueueRequest(context,
                                                                     queue,
                                                                     timeout);
    if (res == bmqt::OpenQueueResult::e_SUCCESS) {
        // Insert the queue into the list
        d_session.d_queueManager.insertQueue(queue);

        // Increment substream count right after sending open queue request.
        d_session.d_queueManager.incrementSubStreamCount(
            queue->uri().canonical());
    }
    return res;
}

bmqt::OpenQueueResult::Enum BrokerSession::QueueFsm::actionReopenQueue(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context,
    const bsls::TimeInterval&            timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bmqt::OpenQueueResult::Enum res = d_session.sendOpenQueueRequest(context,
                                                                     queue,
                                                                     timeout);

    if (res == bmqt::OpenQueueResult::e_SUCCESS) {
        // Increment substream count right after sending open queue request.
        d_session.d_queueManager.incrementSubStreamCount(
            queue->uri().canonical());
    }
    return res;
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::QueueFsm::actionDeconfigureQueue(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& closeContext,
    const bsls::TimeInterval&            timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Unconditionally reset the pending configure request id
    queue->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);

    bmqt::ConfigureQueueResult::Enum rc =
        d_session.sendDeconfigureRequest(queue, closeContext, timeout);
    if (rc == bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Decrement queue substream count immediately after sending
        // deconfigure request.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());
    }
    return rc;
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::QueueFsm::actionDeconfigureExpiredQueue(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Send deconfigure request
    bmqt::ConfigureQueueResult::Enum rc = d_session.sendDeconfigureRequest(
        queue);

    if (rc == bmqt::ConfigureQueueResult::e_SUCCESS) {
        // Decrement queue substream count immediately after sending
        // deconfigure request.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());
    }
    return rc;
}

void BrokerSession::QueueFsm::actionRemoveQueue(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Cancel all pending requests for this queue (buffered and non-buffered).
    // Use E_NOT_SUPPORTED status to indicate this is FSM internal
    // cancellation.
    d_session.cancel(queue,
                     bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED,
                     "The request was canceled [reason: queue being closed]");

    // Cancel pending PUT messages.  All pending CONTROL messages for this
    // queue should be already cancelled.
    d_session.cancelPendingMessages(queue);

    bsl::shared_ptr<Queue> queueSp = d_session.d_queueManager.removeQueue(
        queue.get());
    BSLS_ASSERT_OPT(queueSp);

    if (bmqt::QueueFlagsUtil::isWriter(queueSp->flags())) {
        // Remove retransmission timeout value from the map.  It is ok that
        // there is no such entry in the map (e.g. the queue was never opened).
        d_session.d_queueRetransmissionTimeoutMap.erase(queueSp->id());
    }

    // Remove queue entry from the map if it is still there
    d_timestampMap.erase(queue->uri().asString());

    // Clear queue statistics.  Do not reset CorrelationId since it may be used
    // from the user thread.
    queueSp->clearStatContext();
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::QueueFsm::actionOpenConfigureQueue(
    const RequestManagerType::RequestSp& openContext,
    const RequestManagerType::RequestSp& configContext,
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval&            timeout,
    bool                                 isReopenRequest)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    return d_session.sendOpenConfigureQueue(openContext,
                                            configContext,
                                            queue,
                                            timeout,
                                            isReopenRequest);
}

void BrokerSession::QueueFsm::actionCloseQueue(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Prepare "empty" closeQueue request
    RequestManagerType::RequestSp context =
        d_session.d_requestManager.createRequest();
    context->request().choice().makeCloseQueue();
    context->setGroupId(k_NON_BUFFERED_REQUEST_GROUP_ID);

    const bsls::TimeInterval absTimeout =
        bmqsys::Time::nowMonotonicClock() +
        d_session.d_sessionOptions.closeQueueTimeout();

    actionCloseQueue(context, queue, absTimeout);
}

bmqt::GenericResult::Enum BrokerSession::QueueFsm::actionCloseQueue(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval&            absTimeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // SubStreamCount is decremented when deconfigure request is sent.  Here if
    // the count is zero then isFinal flag should be set in the close request.
    const bool isFinal = d_session.d_queueManager.subStreamCount(
                             queue->uri().canonical()) == 0;

    return d_session.sendCloseQueue(context, queue, absTimeout, isFinal);
}

void BrokerSession::QueueFsm::actionInitQueue(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& openQueueContext,
    bool                                 isReopenRequest)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const bmqp_ctrlmsg::OpenQueueResponse& resp =
        openQueueContext->response().choice().openQueueResponse();
    const bmqp_ctrlmsg::RoutingConfiguration& config =
        resp.routingConfiguration();
    queue->setAtMostOnce(
        bmqp::RoutingConfigurationUtils::isAtMostOnce(config));

    queue->setHasMultipleSubStreams(
        bmqp::RoutingConfigurationUtils::hasMultipleSubStreams(config));

    // For writer queue keep retransmission timeout value provided by the
    // broker
    if (bmqt::QueueFlagsUtil::isWriter(queue->flags())) {
        d_session.d_queueRetransmissionTimeoutMap[queue->id()] =
            resp.deduplicationTimeMs();
    }

    // Do not register queue stats if stats are disabled or queue is reopening
    if (!isReopenRequest && d_session.d_queuesStats.d_statContext_mp) {
        queue->registerStatContext(
            d_session.d_queuesStats.d_statContext_mp.get());
    }
}

bmqt::ConfigureQueueResult::Enum BrokerSession::QueueFsm::actionConfigureQueue(
    const bsl::shared_ptr<Queue>&  queue,
    const bmqt::QueueOptions&      options,
    const bsls::TimeInterval&      timeout,
    RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    if (queue->isSuspendedWithBroker()) {
        if (!options.suspendsOnBadHostHealth()) {
            // Suspended queue is no longer health sensitive: RESUME.
            return d_session.sendResumeRequest(queue,
                                               options,
                                               timeout,
                                               context);
        }
    }
    else if (options.suspendsOnBadHostHealth() && !d_session.isHostHealthy()) {
        // Active queue is now health sensitive on unhealthy host: SUSPEND.

        // Need to override what we're sending the broker first.
        makeDeconfigure(&context->request());

        return d_session.sendSuspendRequest(queue, options, timeout, context);
    }

    // Ordinary configure.
    return d_session.sendConfigureRequest(queue, options, timeout, context);
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::QueueFsm::actionReconfigureQueue(
    const bsl::shared_ptr<Queue>&       queue,
    const bmqp_ctrlmsg::ControlMessage& response)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    bmqp_ctrlmsg::ConsumerInfo ci;

    if (response.choice().isConfigureStreamResponseValue()) {
        const bmqp_ctrlmsg::StreamParameters& sp =
            response.choice()
                .configureStreamResponse()
                .request()
                .streamParameters();
        if (sp.subscriptions().size() == 1) {
            BSLS_ASSERT_SAFE(sp.subscriptions()[0].consumers().size() == 1);
            ci = sp.subscriptions()[0].consumers()[0];
        }
        else {
            BSLS_ASSERT_SAFE(sp.subscriptions().size() == 0);
        }
    }
    else {
        bmqp::ProtocolUtil::convert(&ci,
                                    response.choice()
                                        .configureQueueStreamResponse()
                                        .request()
                                        .streamParameters());
    }

    bmqt::QueueOptions options(d_session.d_allocator_p);
    options.setMaxUnconfirmedMessages(ci.maxUnconfirmedMessages())
        .setMaxUnconfirmedBytes(ci.maxUnconfirmedBytes())
        .setConsumerPriority(ci.consumerPriority());

    if (queue->options().hasSuspendsOnBadHostHealth()) {
        options.setSuspendsOnBadHostHealth(
            queue->options().suspendsOnBadHostHealth());
    }

    if (queue->options() == options) {
        // No need to reconfigure
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    bmqt::ConfigureQueueResult::Enum rc = d_session.sendReconfigureRequest(
        queue);
    if (rc == bmqt::ConfigureQueueResult::e_SUCCESS) {
        // We successfully sent the configureQueue upstream, but we did so only
        // to synchronize the state (not as a result of an explicit request
        // from the client).  So to not cause any future client-initiated
        // configureQueue to immediately return error, we explicitly set the
        // pending configure queue request id to the null value.
        queue->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);
    }
    return rc;
}

void BrokerSession::QueueFsm::actionInitiateQueueSuspend(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Initialize the options to push to the broker while queue is suspended.
    bmqt::QueueOptions suspendOptions(d_session.d_allocator_p);
    suspendOptions.setMaxUnconfirmedMessages(0).setMaxUnconfirmedBytes(0);

    // Create request context.
    RequestManagerType::RequestSp context =
        d_session.createConfigureQueueContext(queue,
                                              suspendOptions,
                                              true,    // isDeconfigure
                                              false);  // isBuffered

    // Deconfigure queue to stop messages from being sent by the broker.
    d_session.sendSuspendRequest(
        queue,
        queue->options(),
        d_session.d_sessionOptions.configureQueueTimeout(),
        context);
}

void BrokerSession::QueueFsm::actionInitiateQueueResume(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    // Create request context.
    RequestManagerType::RequestSp context =
        d_session.createConfigureQueueContext(queue,
                                              queue->options(),
                                              false,   // isDeconfigure
                                              false);  // isBuffered

    // Reconfigure queue to resume receiving messages from the broker.
    d_session.sendResumeRequest(
        queue,
        queue->options(),
        d_session.d_sessionOptions.configureQueueTimeout(),
        context);
}

void BrokerSession::QueueFsm::logOperationTime(const bsl::string& queueUri,
                                               const char*        operation)
{
    TimestampMap::iterator it = d_timestampMap.find(queueUri);
    if (it != d_timestampMap.end()) {
        const bsls::Types::Int64 elapsed =
            bmqsys::Time::highResolutionTimer() - it->second;
        BALL_LOG_INFO << operation << " [uri=" << queueUri << "] took: "
                      << bmqu::PrintUtil::prettyTimeInterval(elapsed) << " ("
                      << elapsed << " nanoseconds)";
        // Handling of error cases causes of several operations.
        // Log only first one (original) and skip others.
        d_timestampMap.erase(it);
    }
}

BrokerSession::QueueFsm::QueueFsm(BrokerSession& session)
: d_session(session)
, d_timestampMap(session.d_allocator_p)
{
    typedef QueueState    S;
    typedef QueueFsmEvent E;
    QueueStateTransition  table[] = {
        // current state           event              new state
        {S::e_CLOSED, E::e_OPEN_CMD, S::e_OPENING_OPN},
        //
        {S::e_OPENING_OPN, E::e_RESP_OK, S::e_OPENING_CFG},
        {S::e_OPENING_OPN, E::e_REQ_NOT_SENT, S::e_CLOSED},
        {S::e_OPENING_OPN, E::e_RESP_BAD, S::e_CLOSED},
        {S::e_OPENING_OPN, E::e_RESP_TIMEOUT, S::e_OPENING_OPN_EXPIRED},
        {S::e_OPENING_OPN, E::e_RESP_EXPIRED, S::e_CLOSED},
        {S::e_OPENING_OPN, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_REOPENING_OPN, E::e_RESP_OK, S::e_REOPENING_CFG},
        {S::e_REOPENING_OPN, E::e_REQ_NOT_SENT, S::e_CLOSED},
        {S::e_REOPENING_OPN, E::e_RESP_BAD, S::e_CLOSED},
        {S::e_REOPENING_OPN, E::e_RESP_TIMEOUT, S::e_OPENING_OPN_EXPIRED},
        {S::e_REOPENING_OPN, E::e_REQ_CANCELED, S::e_REOPENING_OPN},
        {S::e_REOPENING_OPN, E::e_CHANNEL_DOWN, S::e_PENDING},
        {S::e_REOPENING_OPN, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_OPENING_CFG, E::e_RESP_OK, S::e_OPENED},
        {S::e_OPENING_CFG, E::e_REQ_NOT_SENT, S::e_CLOSING_CLS},
        {S::e_OPENING_CFG, E::e_RESP_BAD, S::e_CLOSING_CLS},
        {S::e_OPENING_CFG, E::e_RESP_TIMEOUT, S::e_OPENING_CFG_EXPIRED},
        {S::e_OPENING_CFG, E::e_RESP_EXPIRED, S::e_CLOSED},
        {S::e_OPENING_CFG, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_REOPENING_CFG, E::e_RESP_OK, S::e_OPENED},
        {S::e_REOPENING_CFG, E::e_REQ_NOT_SENT, S::e_CLOSING_CLS},
        {S::e_REOPENING_CFG, E::e_RESP_BAD, S::e_CLOSING_CLS},
        {S::e_REOPENING_CFG, E::e_RESP_TIMEOUT, S::e_OPENING_CFG_EXPIRED},
        {S::e_REOPENING_CFG, E::e_REQ_CANCELED, S::e_REOPENING_CFG},
        {S::e_REOPENING_CFG, E::e_CHANNEL_DOWN, S::e_PENDING},
        {S::e_REOPENING_CFG, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_OPENED, E::e_CLOSE_CMD, S::e_CLOSING_CFG},
        {S::e_OPENED, E::e_CONFIG_CMD, S::e_OPENED},
        {S::e_OPENED, E::e_REQ_NOT_SENT, S::e_OPENED},
        {S::e_OPENED, E::e_REQ_CANCELED, S::e_OPENED},
        {S::e_OPENED, E::e_RESP_OK, S::e_OPENED},
        {S::e_OPENED, E::e_RESP_BAD, S::e_OPENED},
        {S::e_OPENED, E::e_RESP_TIMEOUT, S::e_OPENED},
        {S::e_OPENED, E::e_LATE_RESP, S::e_OPENED},
        {S::e_OPENED, E::e_CHANNEL_DOWN, S::e_PENDING},
        {S::e_OPENED, E::e_SUSPEND, S::e_OPENED},
        {S::e_OPENED, E::e_RESUME, S::e_OPENED},
        {S::e_OPENED, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_CLOSING_CFG, E::e_RESP_OK, S::e_CLOSING_CLS},
        {S::e_CLOSING_CFG, E::e_REQ_NOT_SENT, S::e_CLOSING_CFG_EXPIRED},
        {S::e_CLOSING_CFG, E::e_RESP_BAD, S::e_CLOSING_CLS},
        {S::e_CLOSING_CFG, E::e_RESP_TIMEOUT, S::e_CLOSING_CFG_EXPIRED},
        {S::e_CLOSING_CFG, E::e_REQ_CANCELED, S::e_CLOSED},
        {S::e_CLOSING_CFG, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_CLOSING_CLS, E::e_RESP_OK, S::e_CLOSED},
        {S::e_CLOSING_CLS, E::e_REQ_NOT_SENT, S::e_CLOSING_CLS_EXPIRED},
        {S::e_CLOSING_CLS, E::e_RESP_BAD, S::e_CLOSED},
        {S::e_CLOSING_CLS, E::e_RESP_TIMEOUT, S::e_CLOSING_CLS_EXPIRED},
        {S::e_CLOSING_CLS, E::e_REQ_CANCELED, S::e_CLOSED},
        {S::e_CLOSING_CLS, E::e_SESSION_DOWN, S::e_CLOSED},
        //
        {S::e_OPENING_OPN_EXPIRED, E::e_LATE_RESP, S::e_CLOSING_CLS},
        {S::e_OPENING_OPN_EXPIRED, E::e_CHANNEL_DOWN, S::e_CLOSED},
        //
        {S::e_OPENING_CFG_EXPIRED, E::e_LATE_RESP, S::e_CLOSING_CFG},
        {S::e_OPENING_CFG_EXPIRED, E::e_CHANNEL_DOWN, S::e_CLOSED},
        //
        {S::e_CLOSING_CFG_EXPIRED, E::e_LATE_RESP, S::e_CLOSING_CLS},
        {S::e_CLOSING_CFG_EXPIRED, E::e_CHANNEL_DOWN, S::e_CLOSED},
        //
        {S::e_CLOSING_CLS_EXPIRED, E::e_LATE_RESP, S::e_CLOSED},
        {S::e_CLOSING_CLS_EXPIRED, E::e_CHANNEL_DOWN, S::e_CLOSED},
        //
        {S::e_PENDING, E::e_CHANNEL_UP, S::e_REOPENING_OPN},
        {S::e_PENDING, E::e_CONFIG_CMD, S::e_PENDING},
        {S::e_PENDING, E::e_CLOSE_CMD, S::e_CLOSED},
        {S::e_PENDING, E::e_RESP_BAD, S::e_PENDING},
        {S::e_PENDING, E::e_RESP_TIMEOUT, S::e_PENDING},
        {S::e_PENDING, E::e_RESP_EXPIRED, S::e_PENDING},
        {S::e_PENDING, E::e_SESSION_DOWN, S::e_CLOSED}};

    const int size    = sizeof(table) / sizeof(table[0]);
    d_transitionTable = bsl::vector<QueueStateTransition>(
        table,
        table + size,
        d_session.d_allocator_p);
}

// MANIPULATORS
bmqt::OpenQueueResult::Enum BrokerSession::QueueFsm::handleOpenRequest(
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval&            timeout,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueState::Enum      state = queue->state();
    const QueueFsmEvent::Enum   event = QueueFsmEvent::e_OPEN_CMD;
    bmqt::OpenQueueResult::Enum res   = bmqt::OpenQueueResult::e_UNKNOWN;

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    // We don't support open the same queue multiple times, or concurrently
    switch (state) {
    case QueueState::e_CLOSED: {
        // Generate and set QueueId
        setQueueId(queue, context);

        d_timestampMap[queue->uri().asString()] =
            bmqsys::Time::highResolutionTimer();

        // Switch to OPENING_OPN
        setQueueState(queue, QueueState::e_OPENING_OPN, event);

        const bmqt::OpenQueueResult::Enum rc = actionOpenQueue(queue,
                                                               context,
                                                               timeout);
        if (rc != bmqt::OpenQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }

        // Return SUCCESS result.  Any error will be reported through the
        // context.
        res = bmqt::OpenQueueResult::e_SUCCESS;
    } break;
    case QueueState::e_OPENED: {
        res = bmqt::OpenQueueResult::e_ALREADY_OPENED;
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG: {
        res = bmqt::OpenQueueResult::e_ALREADY_IN_PROGRESS;
    } break;
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_PENDING: {
        res = bmqt::OpenQueueResult::e_NOT_SUPPORTED;
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
    return res;
}

void BrokerSession::QueueFsm::handleRequestNotSent(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context,
    bmqp_ctrlmsg::StatusCategory::Value  status)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueState::Enum    state = queue->state();
    const QueueFsmEvent::Enum event = QueueFsmEvent::e_REQ_NOT_SENT;

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        // Failed to send standalone configure queue request
        BSLS_ASSERT_SAFE(isConfigure(context->request(), context->response()));
        // Check error response is set
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Keep the OPENED state
        setQueueState(queue, QueueState::e_OPENED, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify about configure queue result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN: {
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Open queue request has failed.  Queue is not active
        BSLS_ASSERT_SAFE(!d_session.lookupQueue(queue->uri()));

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // Set user response
        injectErrorResponse(context,
                            status,
                            "Failed to send open queue request to the broker");

        logOperationTime(queue->uri().asString(), "Open queue");

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN: {
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // Remove queue from the queue container
        actionRemoveQueue(queue);

        // Set user response
        injectErrorResponse(
            context,
            status,
            "Failed to send reopen-queue request to the broker");

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify about reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_OPENING_CFG: {
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isOpenQueueResponseValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // The queue was not configured, so no need to deconfigure it
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        // Set user response
        injectErrorResponse(
            context,
            status,
            "Failed to send open-configure-queue request to the broker");

        // Send close queue request
        actionCloseQueue(queue);

        // REVISIT: we could wait for the close response.  Otherwise, it is
        //          non-deterministic: the queue may be CLOSING or CLOSED by
        //          the time user gets callback/semaphore post.

        logOperationTime(queue->uri().asString(), "Open queue");

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_CFG: {
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // The queue was not configured, so no need to deconfigure it
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        // Send close queue request
        actionCloseQueue(queue);

        // Set user response
        injectErrorResponse(
            context,
            status,
            "Failed to send reopen-configure-queue request to the broker");

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify about reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_CLOSING_CFG: {
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Request was canceled (connection lost with upstream).  We do not
        // proceed to send a closeQueue request upstream (there is no
        // channel!).  Instead, we notify about unsuccessful closeQueue
        // response while keeping the queue among the expired queues so as to
        // allow the user to process the failed closeQueue and keep the queue
        // unavailable for operations until 'CHANNEL_DOWN' event.
        setQueueState(queue, QueueState::e_CLOSING_CFG_EXPIRED, event);

        // Set user response
        injectErrorResponse(
            context,
            status,
            "The request was canceled [reason: connection was lost]");

        logOperationTime(queue->uri().asString(), "Close queue");

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_CLOSING_CLS: {
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());

        // Failed to close queue. Mark the queue as EXPIRED so it will be
        // closed on channel down.
        setQueueState(queue, QueueState::e_CLOSING_CLS_EXPIRED, event);

        // Set user response
        injectErrorResponse(
            context,
            status,
            "Failed to send close queue request to the broker");

        logOperationTime(queue->uri().asString(), "Close queue");

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_PENDING:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleReopenRequest(
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval&            timeout,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueState::Enum    state = queue->state();
    const QueueFsmEvent::Enum event = QueueFsmEvent::e_CHANNEL_UP;

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_PENDING: {
        d_timestampMap[queue->uri().asString()] =
            bmqsys::Time::highResolutionTimer();

        // Set REOPENING_OPN state
        setQueueState(queue, QueueState::e_REOPENING_OPN, event);

        // Send reopen request
        const bmqt::OpenQueueResult::Enum res = actionReopenQueue(queue,
                                                                  context,
                                                                  timeout);
        if (res != bmqt::OpenQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(res));
        }
    } break;
    case QueueState::e_OPENED:
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG:
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::QueueFsm::handleConfigureRequest(
    const bsl::shared_ptr<Queue>&  queue,
    const bmqt::QueueOptions&      options,
    const bsls::TimeInterval&      timeout,
    RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum        event = QueueFsmEvent::e_CONFIG_CMD;
    const QueueState::Enum           state = queue->state();
    bmqt::ConfigureQueueResult::Enum rc =
        bmqt::ConfigureQueueResult::e_UNKNOWN;

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        d_timestampMap[queue->uri().asString()] =
            bmqsys::Time::highResolutionTimer();

        // Keep the state OPENED
        setQueueState(queue, QueueState::e_OPENED, event);

        // Do not check the action result.  In case of error it will be
        // reported to the user by the caller.
        rc = actionConfigureQueue(queue, options, timeout, context);
    } break;
    case QueueState::e_PENDING: {
        d_timestampMap[queue->uri().asString()] =
            bmqsys::Time::highResolutionTimer();

        // Keep the state PENDING
        setQueueState(queue, QueueState::e_PENDING, event);

        // The request will be buffered and retransmitted once the session
        // is reconnected.
        // Do not check the action result.  In case of error it will be
        // reported to the user by the caller.
        rc = actionConfigureQueue(queue, options, timeout, context);
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG:
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        rc = bmqt::ConfigureQueueResult::e_NOT_SUPPORTED;
    } break;
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        rc = bmqt::ConfigureQueueResult::e_INVALID_QUEUE;
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }

    return rc;
}

bmqt::CloseQueueResult::Enum BrokerSession::QueueFsm::handleCloseRequest(
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval&            timeout,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());

    if (!d_session.d_acceptRequests) {
        // The session is being/has been stopped or not started: client should
        // not send any request to the broker.
        BALL_LOG_ERROR << "Unable to process closeQueue request "
                       << "[reason: 'SESSION_STOPPED']";
        return bmqt::CloseQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    if (!d_session.lookupQueue(queue->uri())) {
        // The queue is not found or opened
        return bmqt::CloseQueueResult::e_UNKNOWN_QUEUE;  // RETURN
    }

    const QueueFsmEvent::Enum    event = QueueFsmEvent::e_CLOSE_CMD;
    const QueueState::Enum       state = queue->state();
    bmqt::CloseQueueResult::Enum res   = bmqt::CloseQueueResult::e_SUCCESS;

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        d_timestampMap[queue->uri().asString()] =
            bmqsys::Time::highResolutionTimer();

        // Set CLOSING_CFG state
        setQueueState(queue, QueueState::e_CLOSING_CFG, event);

        // Cancel all pending requests for this queue (buffered and
        // non-buffered) using E_NOT_SUPPORTED status to indicate this is an
        // internal FSM cancellation which should not generate new FSM event.
        d_session.cancel(
            queue,
            bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED,
            "The request was canceled [reason: queue being closed]");

        // Skip deconfigure step for the writer
        if (!bmqt::QueueFlagsUtil::isReader(queue->flags())) {
            // Decrement queue substream count immediately
            d_session.d_queueManager.decrementSubStreamCount(
                queue->uri().canonical());
            handleResponseOk(queue,
                             context,
                             bmqsys::Time::nowMonotonicClock() + timeout);
            break;  // BREAK
        }

        // Send deconfigure request.
        bmqt::ConfigureQueueResult::Enum rc = actionDeconfigureQueue(queue,
                                                                     context,
                                                                     timeout);
        if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_PENDING: {
        // The queue is closed on the broker side, now user wants to close it
        // locally, that means this queue will not be reopened.

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove queue from the queue container
        actionRemoveQueue(queue);

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_CLOSED: {
        res = bmqt::CloseQueueResult::e_ALREADY_CLOSED;
    } break;
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_CLOSING_CFG: {
        res = bmqt::CloseQueueResult::e_ALREADY_IN_PROGRESS;
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG: {
        // Queue is not opened (i.e., it's being opened) we don't support
        // concurrent operations like that
        res = bmqt::CloseQueueResult::e_NOT_SUPPORTED;
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED: {
        // TODO: do we want to let the user to explicitly close expired
        //       queues?
        res = bmqt::CloseQueueResult::e_NOT_SUPPORTED;
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }

    return res;
}

void BrokerSession::QueueFsm::handleResponseError(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context,
    const bsls::TimeInterval&            absTimeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_RESP_BAD;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_PENDING:
    case QueueState::e_OPENED: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request()));
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Keep the state
        setQueueState(queue, state, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, and it was never opened, so decrement
        // substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Open queue");

        // Remove queue from the queue list
        actionRemoveQueue(queue);

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, it has failed to reopen, so decrement
        // substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Remove queue and notify about open queue result
        actionRemoveQueue(queue);

        // Notify about reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_OPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set CLOSING_CLS state
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        // The queue was never opened.  Here the final close request will be
        // sent, so decrement substream count.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Open queue");

        // Send close queue request
        actionCloseQueue(queue);

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set CLOSING_CLS state
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        // The queue has not been reopened.  Here the final close request
        // will be sent, so decrement substream count.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        // Send close queue request
        actionCloseQueue(queue);

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify about reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_CLOSING_CLS: {
        // Expect 'CloseQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Close response with an error.  In this case we consider the queue
        // is closed.
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove queue
        actionRemoveQueue(queue);

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_CLOSING_CFG: {
        // Expect 'CloseQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Deconfigure has failed, try to close
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        const bmqt::GenericResult::Enum rc = actionCloseQueue(context,
                                                              queue,
                                                              absTimeout);
        if (rc != bmqt::GenericResult::e_SUCCESS) {
            // Failed to send close request.
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleSessionDown(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_SESSION_DOWN;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, decrement substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Open queue");

        // Remove queue from the queue list
        actionRemoveQueue(queue);

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG: {
        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, it has failed to reopen, so decrement
        // substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Remove queue and notify about open queue result
        actionRemoveQueue(queue);

        // Notify about reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS: {
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Request was canceled (connection lost with upstream).  Notify
        // about unsuccessful closeQueue response and close the queue.
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove queue from the queue container
        actionRemoveQueue(queue);

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_PENDING:
    case QueueState::e_OPENED: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request()));
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, decrement substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Remove queue from the queue list
        actionRemoveQueue(queue);

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleRequestCanceled(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_REQ_CANCELED;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request()));
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Keep the state
        setQueueState(queue, state, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Keep the state and do not notify about request's result.
        // If the request is canceled it is expected that CHANNEL_DOWN signal
        // will follow and the queue state will be set back to PENDING.
        setQueueState(queue, QueueState::e_REOPENING_OPN, event);
    } break;
    case QueueState::e_REOPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Keep the state and do not notify about request's result.
        // If the request is canceled it is expected that CHANNEL_DOWN signal
        // will follow and the queue state will be set back to PENDING.
        setQueueState(queue, QueueState::e_REOPENING_CFG, event);
    } break;
    case QueueState::e_CLOSING_CFG: {
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Request was canceled (connection lost with upstream).  We do not
        // proceed to send a closeQueue request upstream (there is no
        // channel!).  Instead, we notify about unsuccessful closeQueue
        // response and close the queue.
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove queue from the queue container
        actionRemoveQueue(queue);

        // Set user response
        injectErrorResponse(
            context,
            bmqp_ctrlmsg::StatusCategory::E_CANCELED,
            "The request was canceled [reason: connection was lost]");

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_CLOSING_CLS: {
        // Expect 'CloseQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // Close response with an error.  In this case we consider the queue
        // is closed.
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove queue
        actionRemoveQueue(queue);

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_OPENING_CFG:
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_PENDING:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleResponseTimeout(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_RESP_TIMEOUT;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request()));

        // Keep OPENED state
        setQueueState(queue, QueueState::e_OPENED, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_OPENING_OPN_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Open queue");

        // Notify open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_OPENING_OPN_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_OPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_OPENING_CFG_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Open queue");

        // Notify open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_OPENING_CFG_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_CLOSING_CFG: {
        // Expect 'CloseQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_CLOSING_CFG_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_CLOSING_CLS: {
        // Expect 'CloseQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());

        // Set EXPIRED state
        setQueueState(queue, QueueState::e_CLOSING_CLS_EXPIRED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Notify about close result
        context->signal();
    } break;
    case QueueState::e_PENDING: {
        BSLS_ASSERT_SAFE(isConfigure(context->request()));

        // Buffered request timed out.  Keep the state and notify the caller.
        setQueueState(queue, state, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure queue result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleResponseExpired(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_RESP_EXPIRED;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_PENDING: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request()));

        // Keep PENDING state
        setQueueState(queue, QueueState::e_PENDING, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG: {
        // Expect 'OpenQueue' request and 'Status' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isStatusValue());

        // The opening has timed out and there is no connetion to the broker.
        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        // The queue is closed, and it was never opened, so decrement
        // substream count
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        logOperationTime(queue->uri().asString(), "Open queue");

        // Remove queue from the queue list
        actionRemoveQueue(queue);

        // Notify about open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG:
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_OPENED:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleResponseOk(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context,
    const bsls::TimeInterval&            timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_RESP_OK;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENED: {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(isConfigure(context->request(), context->response()));

        // Keep the state OPENED
        setQueueState(queue, QueueState::e_OPENED, event);

        logOperationTime(queue->uri().asString(), "Configure queue");

        // Notify configure result
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN: {
        // Expect 'OpenQueue' request and 'OpenQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isOpenQueueResponseValue());

        // Set OPENING_CFG state and Send configure queue request
        setQueueState(queue, QueueState::e_OPENING_CFG, event);

        // If queue is sensitive to host health and the host is unhealthy,
        // then transition directly to suspended state.
        queue->setIsSuspended(queue->options().suspendsOnBadHostHealth() &&
                              !d_session.isHostHealthy());
        queue->setIsSuspendedWithBroker(queue->isSuspended());

        // Skip configure step for suspended and write-only queues.
        if (queue->isSuspended() ||
            !bmqt::QueueFlagsUtil::isReader(queue->flags())) {
            handleResponseOk(queue, context, bsls::TimeInterval(0));
            break;  // BREAK
        }

        bmqpi::DTSpan::Baggage baggage(d_session.d_allocator_p);
        fillDTSpanQueueBaggage(&baggage, *queue);
        bsl::shared_ptr<bmqpi::DTSpan> configureSpan(
            d_session.createDTSpan("bmq.queue.openConfigure", baggage));
        bslma::ManagedPtr<void> scopedSpan(
            d_session.activateDTSpan(configureSpan));

        RequestManagerType::RequestSp configureQueueContext =
            d_session.createConfigureQueueContext(queue,
                                                  queue->options(),
                                                  false,  // isDeconfigure
                                                  true);  // isBuffered
        configureQueueContext->setDTSpan(configureSpan);

        bmqt::ConfigureQueueResult::Enum rc = actionOpenConfigureQueue(
            context,
            configureQueueContext,
            queue,
            timeout,
            false);  // isReopenRequest
        if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_REOPENING_OPN: {
        // Expect 'OpenQueue' request and 'OpenQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isOpenQueueResponseValue());

        // Set REOPENING_CFG state and Send configure queue request
        setQueueState(queue, QueueState::e_REOPENING_CFG, event);

        // If queue is sensitive to host health and the host is unhealthy,
        // then transition directly to suspended state.
        queue->setIsSuspended(queue->options().suspendsOnBadHostHealth() &&
                              !d_session.isHostHealthy());
        queue->setIsSuspendedWithBroker(queue->isSuspended());

        // Skip configure step for suspended and write-only queues.
        if (queue->isSuspended() ||
            !bmqt::QueueFlagsUtil::isReader(queue->flags())) {
            handleResponseOk(queue, context, bsls::TimeInterval(0));
            break;  // BREAK
        }

        RequestManagerType::RequestSp configureQueueContext =
            d_session.createConfigureQueueContext(queue,
                                                  queue->options(),
                                                  false,   // isDeconfigure
                                                  false);  // isBuffered

        bmqt::ConfigureQueueResult::Enum rc = actionOpenConfigureQueue(
            context,
            configureQueueContext,
            queue,
            timeout,
            true);  // isReopenRequest
        if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_OPENING_CFG: {
        // Expect 'OpenQueue' request and 'OpenQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isOpenQueueResponseValue());

        // Successfully opened the queue.  Set the queue as OPENED
        setQueueState(queue, QueueState::e_OPENED, event);

        // Create the stat context if needed
        actionInitQueue(queue, context, false);  // false - isReopenRequest

        logOperationTime(queue->uri().asString(), "Open queue");

        // Notify open queue result
        context->signal();
    } break;
    case QueueState::e_REOPENING_CFG: {
        // Expect 'OpenQueue' request and 'OpenQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isOpenQueueResponseValue());

        // Successfully reopened the queue.  Set the queue as OPENED
        setQueueState(queue, QueueState::e_OPENED, event);

        // Create the stat context if needed
        actionInitQueue(queue, context, true);  // true - isReopenRequest

        logOperationTime(queue->uri().asString(), "Reopen queue");

        // Notify reopen queue result
        context->signal();

        // Check STATE_RESTORED condition
        d_session.enqueueStateRestoredIfNeeded();
    } break;
    case QueueState::e_CLOSING_CFG: {
        // Expect 'CloseQueue' request and 'CloseQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(context->response().choice().isUndefinedValue());

        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        const bmqt::GenericResult::Enum rc = actionCloseQueue(context,
                                                              queue,
                                                              timeout);
        if (rc != bmqt::GenericResult::e_SUCCESS) {
            // Failed to send close request.
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_CLOSING_CLS: {
        // Expect 'CloseQueue' request and 'CloseQueueResponse' response.
        BSLS_ASSERT_SAFE(context->request().choice().isCloseQueueValue());
        BSLS_ASSERT_SAFE(
            context->response().choice().isCloseQueueResponseValue());

        // Set CLOSED state
        setQueueState(queue, QueueState::e_CLOSED, event);

        logOperationTime(queue->uri().asString(), "Close queue");

        // Remove active queue from the list
        actionRemoveQueue(queue);

        // Notify about close result.
        context->signal();
    } break;
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_PENDING:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleLateResponse(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_LATE_RESP;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENING_OPN_EXPIRED: {
        // We received an openQueue *SUCCESS* response but we already timed
        // the request out; in order to keep consistent state across all
        // brokers, we must send a closeQueue (as far as upstream brokers are
        // concerned, this client has the queue opened, which is not true, so
        // roll it back).

        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Set queue state to CLOSING_CLS and send close queue request.
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        // The queue was never opened.  The final close request will be sent,
        // so decrement substream count.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());

        actionCloseQueue(queue);
    } break;
    case QueueState::e_OPENING_CFG_EXPIRED: {
        // 2nd part of openQueue.
        // We received a configureQueue *SUCCESS* response but we already
        // timed the request out.  If a queue is open, then we need to
        // synchronize the upstream state with the state here in the SDK.
        // Note that there are three different possible scenarios for the
        // context in which the original configureQueue request was sent out
        // (and for which we are processing the late response here):

        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));
        BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

        // Set queue state to CLOSING_CFG and send deconfigure queue request.
        setQueueState(queue, QueueState::e_CLOSING_CFG, event);

        bmqt::ConfigureQueueResult::Enum rc = actionDeconfigureExpiredQueue(
            queue);
        if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_CLOSING_CFG_EXPIRED: {
        // 1st part of closeQueue.

        BSLS_ASSERT_SAFE(isConfigureResponse(context->response()));
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));

        // Set queue state to CLOSING_CLS and send deconfigure queue request.
        setQueueState(queue, QueueState::e_CLOSING_CLS, event);

        actionCloseQueue(queue);
    } break;
    case QueueState::e_CLOSING_CLS_EXPIRED: {
        BSLS_ASSERT_SAFE(
            context->response().choice().isCloseQueueResponseValue());

        // Set queue state to CLOSED
        setQueueState(queue, QueueState::e_CLOSED, event);

        // Remove active queue from the list
        actionRemoveQueue(queue);
    } break;
    case QueueState::e_OPENED: {
        // Standalone configureQueue.  We are processing a late response for
        // a queue that is *currently* open.  If the stream parameters that
        // upstream is confirming here are different than the queue options
        // in the SDK, send a configure request with the *current* SDK
        // parameters.

        // Expect standalone configure response only for reader
        BSLS_ASSERT_SAFE(bmqt::QueueFlagsUtil::isReader(queue->flags()));
        BSLS_ASSERT_SAFE(isConfigureResponse(context->response()));

        // Keep the state OPENED
        setQueueState(queue, QueueState::e_OPENED, event);

        bmqt::ConfigureQueueResult::Enum rc =
            actionReconfigureQueue(queue, context->response());
        if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
            handleRequestNotSent(
                queue,
                context,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc));
        }
    } break;
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG:
    case QueueState::e_PENDING:
    case QueueState::e_CLOSED: {
        BALL_LOG_ERROR << "Unexpected queue state: " << *queue
                       << " when handling " << event;
        BSLS_ASSERT_SAFE(false);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleChannelDown(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_CHANNEL_DOWN;
    const QueueState::Enum    state = queue->state();

    BALL_LOG_INFO << "Queue FSM Event: " << event << " ["
                  << "QueueState: " << state << "]";

    switch (state) {
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED: {
        // Set queue state to CLOSED for expired queue
        setQueueState(queue, QueueState::e_CLOSED, event);

        // Channel is down, so there will be no more incoming responses for
        // the expired queues.  Decrement subStream count and remove queue
        // from the queue container.
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());
        actionRemoveQueue(queue);
    } break;
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS_EXPIRED: {
        // Set queue state to CLOSED for expired queue
        setQueueState(queue, QueueState::e_CLOSED, event);

        // Remove queue from the queue container
        actionRemoveQueue(queue);
    } break;
    case QueueState::e_OPENED: {
        // Set queue state to PENDING
        setQueueState(queue, QueueState::e_PENDING, event);
    } break;
    case QueueState::e_REOPENING_OPN:
    case QueueState::e_REOPENING_CFG: {
        // The channel goes down while reopening.  All the pending reopen
        // requests (non buffered) are already cancelled by the session.
        // Set queue state back to PENDING.
        setQueueState(queue, QueueState::e_PENDING, event);

        // Decrement queue substream count immediately
        d_session.d_queueManager.decrementSubStreamCount(
            queue->uri().canonical());
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_OPENING_CFG:
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_CLOSED:
    case QueueState::e_PENDING: {
        BALL_LOG_INFO << "No actions for queue: " << *queue
                      << " when handling " << event;
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleQueueSuspend(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_SUSPEND;
    const QueueState::Enum    state = queue->state();

    switch (state) {
    case QueueState::e_OPENED: {
        setQueueState(queue, state, event);
        actionInitiateQueueSuspend(queue);
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_REOPENING_OPN: {
        // Queue will suspend on OPN response if host is still unhealthy.
    } break;
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_CFG: {
        // Waiting for an existing CFG to return - Suspend will re-launch
        // when the outstanding response is processed (if the host is still
        // unhealthy).
    } break;
    case QueueState::e_CLOSED:
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_PENDING: {
        BALL_LOG_INFO << "No actions for queue: " << *queue
                      << " when handling " << event;
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

void BrokerSession::QueueFsm::handleQueueResume(
    const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_session.d_fsmThreadChecker.inSameThread());

    const QueueFsmEvent::Enum event = QueueFsmEvent::e_RESUME;
    const QueueState::Enum    state = queue->state();

    switch (state) {
    case QueueState::e_OPENED: {
        setQueueState(queue, state, event);
        actionInitiateQueueResume(queue);
    } break;
    case QueueState::e_OPENING_OPN:
    case QueueState::e_REOPENING_OPN: {
        // Queue will resume on OPN response if host is still healthy.
    } break;
    case QueueState::e_OPENING_CFG:
    case QueueState::e_REOPENING_CFG: {
        // Waiting for an existing CFG to return - Resume will re-launch when
        // the outstanding response is processed (if host is still healthy).
    } break;
    case QueueState::e_CLOSING_CFG:
    case QueueState::e_CLOSING_CFG_EXPIRED:
    case QueueState::e_CLOSING_CLS:
    case QueueState::e_CLOSING_CLS_EXPIRED:
    case QueueState::e_OPENING_CFG_EXPIRED:
    case QueueState::e_OPENING_OPN_EXPIRED:
    case QueueState::e_PENDING:
    case QueueState::e_CLOSED: {
        if (d_session.d_numPendingHostHealthRequests == 0 &&
            d_session.isHostHealthy()) {
            d_session.d_sessionFsm.handleAllQueuesResumed();
        }
        queue->setIsSuspendedWithBroker(false);
        // Necessary for certain edge cases. For instance, if a request to
        // close cancels a pending 'suspend', the 'suspend'-callback will
        // try to resume the queue, reach this case, and push
        // 'ALL_QUEUES_RESUMED'. If 'isSuspendedWithBroker' is not flipped
        // here, then the response to the 'CLOSING_CFG' request will then
        // also try to resume, yielding a duplicate 'ALL_QUEUES_RESUMED'.
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unexpected Queue state");
    } break;
    }
}

// ----------------------------
// class BrokerSession_Executor
// ----------------------------

BrokerSession_Executor::BrokerSession_Executor(BrokerSession* session)
: d_owner_p(session)
{
    BSLS_ASSERT_SAFE(d_owner_p);
}

bool BrokerSession_Executor::operator==(
    const BrokerSession_Executor& rhs) const
{
    return d_owner_p == rhs.d_owner_p;
}

void BrokerSession_Executor::post(const bsl::function<void()>& f) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_owner_p);
    BSLS_ASSERT_SAFE(f);

    d_owner_p->postToFsm(f);
}

// -------------------
// class BrokerSession
// -------------------

bmqt::GenericResult::Enum
BrokerSession::sendRequest(const RequestManagerType::RequestSp& context,
                           const bmqp::QueueId&                 queueId,
                           bsls::TimeInterval                   timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    BSLA_MAYBE_UNUSED const bool isDisconnect =
        context->request().choice().isDisconnectValue();

    // For all requests except disconnect the d_acceptRequests should be true
    BSLS_ASSERT_SAFE(isDisconnect || d_acceptRequests);

    bsl::string nodeDescription("pending buffer", d_allocator_p);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(d_channel_sp)) {
        nodeDescription = d_channel_sp->peerUri();
    }

    bmqt::GenericResult::Enum rc = d_requestManager.sendRequest(
        context,
        bdlf::BindUtil::bind(&BrokerSession::requestWriterCb,
                             this,
                             context,
                             queueId,
                             bdlf::PlaceHolders::_1,  // blob
                             d_sessionOptions.channelHighWatermark()),
        nodeDescription,
        timeout);

    return rc;
}

void BrokerSession::sendConfirm(const bdlbb::Blob& blob, const int msgCount)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Dump if enabled
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_messageDumper
                .isEventDumpEnabled<bmqp::EventType::e_CONFIRM>())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        const bmqp::Event event(&blob, d_allocator_p);
        BALL_LOG_INFO_BLOCK
        {
            d_messageDumper.dumpConfirmEvent(BALL_LOG_OUTPUT_STREAM, event);
        }
    }

    bmqt::GenericResult::Enum res =
        writeOrBuffer(blob, d_sessionOptions.channelHighWatermark());

    // If write failed due to HWM, the event is in the extention buffer and
    // will be resent, so update the statistics as it was sent successfully.
    // For any other error return without updating the stats.
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            res != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Unable to confirm " << msgCount << " message(s)"
                       << " [reason: 'NOT_CONNECTED']";
        return;  // RETURN
    }

    // Update stats
    d_eventsStats.onEvent(EventsStatsEventType::e_CONFIRM,
                          blob.length(),
                          msgCount);
}

void BrokerSession::fsmThreadLoop()
{
    // executed by the FSM thread

    // Init FSM thread checker
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    BALL_LOG_INFO << "FSM thread started "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";

    while (true) {
        bsl::shared_ptr<Event> event;

        const int rc = d_fsmEventQueue.popFront(&event);
        BSLS_ASSERT_SAFE(rc == 0);
        (void)rc;
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event)) {  // PoisonPill
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Drain left over events (there should be none)
            while (d_fsmEventQueue.tryPopFront(&event) == 0) {
                BALL_LOG_WARN << "Dropping unhandled FSM event: " << *event;
            }

            // Terminate the thread
            break;  // BREAK
        }

        const Event::EventType::Enum eventType = event->type();
        switch (eventType) {
        case Event::EventType::e_RAW: {
            processRawEvent(event->rawEvent());
        } break;
        case Event::EventType::e_REQUEST: {
            BSLS_ASSERT_SAFE(event->eventCallback() != 0);
            event->eventCallback()(event);
        } break;
        case Event::EventType::e_SESSION:
        case Event::EventType::e_MESSAGE:
        case Event::EventType::e_UNINITIALIZED:
        default: {
            BALL_LOG_ERROR << "Unexpected FSM event: " << *event;
            BSLS_ASSERT_SAFE(false && "Unexpected FSM event");
        } break;
        }
    }

    BALL_LOG_INFO << "FSM thread terminated "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";
}

void BrokerSession::eventHandlerCbWrapper(
    const bsl::shared_ptr<Event>&           event,
    const EventQueue::EventHandlerCallback& eventHandlerCb)
{
    // executed by one of the *EVENT HANDLER* threads

    if (event->eventCallback()) {
        event->eventCallback()(event);
        return;  // RETURN
    }

    // If the current event is a special DISCONNECTED event with the status
    // code set to -1 then do not dispatch it to the user.  Instead check if
    // there are no active handlers and release the stop semaphore.
    const bool isDisconnected = (event->type() ==
                                     Event::EventType::e_SESSION &&
                                 event->sessionEventType() ==
                                     bmqt::SessionEventType::e_DISCONNECTED &&
                                 event->statusCode() == -1);

    ++d_inProgressEventHandlerCount;

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(!isDisconnected)) {
        eventHandlerCb(event);
    }
    else {
        d_isStopping = true;
    }

    // If the session is stopping and there are no more active handlers we
    // release the stop semaphore.
    const int remaining = --d_inProgressEventHandlerCount;
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_isStopping)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (remaining == 0) {
            if (d_isStopping.testAndSwap(true, false) == true) {
                d_stopSemaphore.post();
            }
        }
    }
}

void BrokerSession::asyncRequestNotifier(
    const RequestManagerType::RequestSp& context,
    bmqt::SessionEventType::Enum         eventType,
    const bmqt::CorrelationId&           correlationId,
    const bsl::shared_ptr<Queue>&        queue,
    const EventCallback&                 eventCallback)
{
    // executed by *ANY* thread

    if (!context) {
        // meaning it did not even reach the FSM
        enqueueSessionEvent(eventType,
                            bmqt::GenericResult::e_REFUSED,
                            k_ENQUEUE_ERROR_TEXT,
                            correlationId,
                            queue,
                            eventCallback);
    }
    else if (context->isError()) {
        bmqt::GenericResult::Enum result = context->result();

        enqueueSessionEvent(eventType,
                            result,
                            context->response().choice().status().message(),
                            correlationId,
                            queue,
                            eventCallback);
    }
    else {
        enqueueSessionEvent(eventType,
                            bmqt::GenericResult::e_SUCCESS,
                            "",
                            correlationId,
                            queue,
                            eventCallback);
    }
}

void BrokerSession::syncRequestNotifier(
    bslmt::Semaphore*                    semaphore,
    int*                                 status,
    const RequestManagerType::RequestSp& context)
{
    // executed by *ANY* thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(semaphore);
    BSLS_ASSERT_SAFE(status);

    bmqt::GenericResult::Enum result = bmqt::GenericResult::e_SUCCESS;

    if (!context) {
        result = bmqt::GenericResult::e_REFUSED;
    }
    else if (context->isError()) {
        result = context->result();
    }
    else {
        result = bmqt::GenericResult::e_SUCCESS;
    }
    *status = result;

    // This serves SYNC version which does not expect user callback.

    // There are two sync versions.  1) `Session::open/configure/closeQueue` is
    // deprecated but still in use and it can be called from user handler
    // thread.  Therefore, the `syncOperationSemaphore` must be posted from FSM
    // thread.  2) `Session::open/configure/closeQueueSync` can not be called
    // from user handler thread and the `syncOperationSemaphore` must be posted
    // from  user handler thread to ensure that all previous user events are
    // drained.

    semaphore->post();
}

void BrokerSession::manualSyncRequestNotifier(
    const RequestManagerType::RequestSp& context,
    bmqt::SessionEventType::Enum         eventType,
    const bmqt::CorrelationId&           correlationId,
    const bsl::shared_ptr<Queue>&        queue,
    const EventCallback&                 eventCallback)
{
    // executed by *ANY* thread

    bmqt::GenericResult::Enum result    = bmqt::GenericResult::e_SUCCESS;
    const char*               errorText = "";

    if (!context) {
        result    = bmqt::GenericResult::e_REFUSED;
        errorText = k_ENQUEUE_ERROR_TEXT;
    }
    else if (context->isError()) {
        result    = context->result();
        errorText = context->response().choice().status().message().c_str();
    }

    bsl::shared_ptr<Event> event = createEvent();
    event->configureAsSessionEvent(eventType,
                                   result,
                                   correlationId,
                                   errorText);
    if (queue) {
        // Only some session events have an associated queue (e.g. 'OPEN',
        // 'CLOSE', 'CONFIGURE)
        event->insertQueue(queue);
    }

    eventCallback(event);
}

void BrokerSession::processRawEvent(const bmqp::Event& event)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(event.isControlEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        processControlEvent(event);
    }
    else if (event.isPushEvent()) {
        processPushEvent(event);
    }
    else if (event.isAckEvent()) {
        processAckEvent(event);
    }
    else if (event.isPutEvent()) {
        processPutEvent(event);
    }
    else if (event.isConfirmEvent()) {
        processConfirmEvent(event);
    }
    else {
        BALL_LOG_WARN << "Received BlazingMQ event of unhandled type: "
                      << event.type();
    }
}

void BrokerSession::processControlEvent(const bmqp::Event& event)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);
    bmqp_ctrlmsg::ControlMessage          controlMessage(&localAllocator);

    int rc = event.loadControlEvent(&controlMessage);
    if (rc != 0) {
        BALL_LOG_ERROR << "Received invalid control message from broker "
                       << "[reason: 'failed to decode', rc: " << rc << "]"
                       << "\n"
                       << bmqu::BlobStartHexDumper(event.blob());
        return;  // RETURN
    }

    BALL_LOG_INFO << "Received " << controlMessage;
    d_eventsStats.onEvent(EventsStatsEventType::e_CONTROL,
                          event.blob()->length(),
                          0);

    if (controlMessage.rId().isNull()) {
        // Control Message has no id... it's a control message spontaneously
        // originating from the broker.. so we don't have any request context,
        BALL_LOG_WARN << "Received invalid control message from broker "
                      << controlMessage;
        return;  // RETURN
    }

    // Control message has an id, this is a response to a request
    rc = d_requestManager.processResponse(controlMessage);
    if (rc != 0) {
        BALL_LOG_ERROR << "Received an unrecognized response: "
                       << controlMessage;
        // Ignore the event.
    }
}

void BrokerSession::onHeartbeat()
{
    // executed by the *IO* thread
    // Add to the FSM event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doHandleHeartbeat,
                             this,
                             bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(queueEvent);
}

void BrokerSession::enableMessageRetransmission(
    const bmqp::PutMessageIterator& putIter,
    const bsls::TimeInterval&       sentTime)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(putIter.isValid());
    BSLS_ASSERT_SAFE(!putIter.header().messageGUID().isUnset());

    bdlbb::Blob appData(d_bufferFactory_p, d_allocator_p);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            putIter.loadApplicationData(&appData) != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Failed to load message payload";
        BSLS_ASSERT_SAFE(false && "Failed to load message payload");
        return;  // RETURN
    }

    // PUT messages without ACK_REQUESTED flag have not been added to the
    // CorrelationId container by the user.  Add them here.
    if (!bmqp::PutHeaderFlagUtil::isSet(
            putIter.header().flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED)) {
        bmqp::QueueId       qId(putIter.header().queueId());
        bmqt::CorrelationId emptyCorrId;

        BSLS_ASSERT_SAFE(0 != d_messageCorrelationIdContainer.find(
                                  &emptyCorrId,
                                  putIter.header().messageGUID()));

        d_messageCorrelationIdContainer.add(putIter.header().messageGUID(),
                                            emptyCorrId,
                                            qId);
    }

    d_messageCorrelationIdContainer.associateMessageData(putIter.header(),
                                                         appData,
                                                         sentTime);
}

void BrokerSession::processPutEvent(const bmqp::Event& event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    bool readyToSend = isStarted() && (d_numPendingReopenQueues == 0);

    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(readyToSend)) {
        // Post the event.
        bmqt::GenericResult::Enum res = writeOrBuffer(
            *event.blob(),
            d_sessionOptions.channelHighWatermark());

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                res != bmqt::GenericResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_ERROR << "Unable to post event [reason: 'NOT_CONNECTED']";

            // Channel is down. The blob hasn't been put into the extention
            // buffer. The messages will be put into the retransmitting buffer
            // to be sent once the session is reconnected.
            readyToSend = false;
        }
    }

    const bsls::TimeInterval sentTime = bmqsys::Time::nowMonotonicClock();
    bmqp::PutMessageIterator putIter(d_bufferFactory_p, d_allocator_p);

    // Get PUT iterator without decompression
    event.loadPutMessageIterator(&putIter);

    BSLS_ASSERT_SAFE(putIter.isValid());

    while (BSLS_PERFORMANCEHINT_PREDICT_LIKELY((putIter.next()) == 1)) {
        const bool ackRequested = bmqp::PutHeaderFlagUtil::isSet(
            putIter.header().flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED);

        // If the message has been sent and has no ACK_REQUESTED flag then it
        // shouldn't be retransmitted.
        const bool noNeedToResend = readyToSend && !ackRequested;

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(noNeedToResend)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_DEBUG << "PUT message is not added for retransmission";

            continue;  // CONTINUE
        }

        // Enable retransmission for the message.  Such message will be either
        // ACKed by the broker, or retransmitted, or NACKed if the session goes
        // down.
        enableMessageRetransmission(putIter, sentTime);
    }
}

void BrokerSession::processConfirmEvent(const bmqp::Event& event)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isStarted())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Not connected to broker, can't post the event.
        BALL_LOG_ERROR << "Unable to send confirm event "
                       << "[reason: 'NOT_CONNECTED']";
        return;  // RETURN
    }

    bmqp::ConfirmMessageIterator confirmIter;
    event.loadConfirmMessageIterator(&confirmIter);

    // Iterate over the messages to count number of messages
    int rc       = 0;
    int msgCount = 0;
    while ((rc = confirmIter.next()) == 1) {
        ++msgCount;
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc < 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to confirm event [reason: 'Iteration failed'"
                       << ",rc: " << rc << "]";
        return;  // RETURN
    }

    // Send the CONFIRM batch
    sendConfirm(*event.blob(), msgCount);
}

void BrokerSession::processPushEvent(const bmqp::Event& event)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    enum { e_NUM_BYTES_IN_BLOB_TO_DUMP = 256 };

    // Update stats
    bdlma::LocalSequentialAllocator<1024> iteratorLsa(d_allocator_p);
    bmqp::PushMessageIterator msgIterator(d_bufferFactory_p, &iteratorLsa);
    event.loadPushMessageIterator(&msgIterator);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!msgIterator.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to process PUSH event "
                       << "[reason: 'Invalid PushIterator']";
        return;  // RETURN
    }

    int  eventMessageCount                 = 0;
    int  eventByteCount                    = 0;
    bool hasMessageWithMultipleSubQueueIds = false;

    bdlma::LocalSequentialAllocator<16 * sizeof(bmqp::EventUtilEventInfo)>
        localAllocator(d_allocator_p);

    // TODO:  This 'flattening' needs refactoring, so that 'flat' events get
    // created on the fly rather than accumulating 'EventUtilEventInfo'.

    bsl::vector<bmqp::EventUtilEventInfo> eventInfos(&localAllocator);

    int rc = d_queueManager.onPushEvent(&eventInfos,
                                        &eventMessageCount,
                                        &hasMessageWithMultipleSubQueueIds,
                                        msgIterator);
    // Check for invalid message
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to process push event "
                       << "[reason: 'Iteration failed', rc: " << rc << "]";
        return;  // RETURN
    }

    // Flatten event if needed
    if (hasMessageWithMultipleSubQueueIds) {
        // Need to flatten the PushEvent
        eventInfos.clear();

        rc = bmqp::EventUtil::flattenPushEvent(&eventInfos,
                                               event,
                                               d_bufferFactory_p,
                                               d_blobSpPool_p,
                                               d_allocator_p);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_ERROR << "Unable to flatten PUSH event" << " [rc: " << rc
                           << ", length: " << event.blob()->length()
                           << ", eventMessageCount: " << eventMessageCount
                           << "]" << bsl::endl
                           << bmqu::BlobStartHexDumper(
                                  event.blob(),
                                  e_NUM_BYTES_IN_BLOB_TO_DUMP);
            return;  // RETURN
        }
    }
    else {  // !hasMessageWithMultipleSubQueueIds
        // No need to flatten, can use the original blob.
    }

    bsl::shared_ptr<Event> queueEvent;

    for (QueueManager::EventInfos::size_type i = 0; i < eventInfos.size();) {
        const bmqp::EventUtilEventInfo& currEventInfo = eventInfos[i];

        if (hasMessageWithMultipleSubQueueIds) {
            queueEvent = createEvent();
            const bmqp::Event rawEvent(&currEventInfo.d_blob,
                                       d_allocator_p,
                                       true);
            queueEvent->configureAsMessageEvent(rawEvent);
        }
        else if (i == 0) {
            queueEvent = createEvent();
            queueEvent->configureAsMessageEvent(event);
        }
        // Insert queues in event
        const bmqp::EventUtilEventInfo::Ids& sIds = currEventInfo.d_ids;
        for (bmqp::EventUtilEventInfo::Ids::const_iterator citer =
                 sIds.begin();
             citer != sIds.end();
             ++citer) {
            bmqt::CorrelationId         correlationId;
            unsigned int                subscriptionHandleId;
            const QueueManager::QueueSp queue =
                d_queueManager.observePushEvent(&correlationId,
                                                &subscriptionHandleId,
                                                *citer);

            BSLS_ASSERT(queue);
            queueEvent->insertQueue(citer->d_subscriptionId, queue);

            // Use 'subscriptionHandle' instead of the internal
            // 'citer->d_subscriptionId' so that
            // 'bmqimp::Event::subscriptionId()' returns 'subscriptionHandle'

            queueEvent->addCorrelationId(correlationId, subscriptionHandleId);
        }

        // Update event bytes
        eventByteCount += queueEvent->rawEvent().blob()->length();

        ++i;

        if (hasMessageWithMultipleSubQueueIds || i == eventInfos.size()) {
            // Dump if enabled
            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    d_messageDumper
                        .isEventDumpEnabled<bmqp::EventType::e_PUSH>())) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                BALL_LOG_INFO_BLOCK
                {
                    d_messageDumper.dumpPushEvent(BALL_LOG_OUTPUT_STREAM,
                                                  queueEvent->rawEvent());
                }
            }

            // Add to event queue
            d_eventQueue.pushBack(queueEvent);
        }
    }

    // Update event stats
    d_eventsStats.onEvent(EventsStatsEventType::e_PUSH,
                          eventByteCount,
                          eventMessageCount);
}

void BrokerSession::processAckEvent(const bmqp::Event& event)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_ACK>())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO_BLOCK
        {
            d_messageDumper.dumpAckEvent(BALL_LOG_OUTPUT_STREAM, event);
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            state() == bmqimp::BrokerSession::State::e_CLOSING_SESSION)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Disconnecting from the broker. Pending PUTs have been already NACKed
        // locally.
        BALL_LOG_WARN << "Ignore ACK event [reason: 'DISCONNECTING']";
        return;  // RETURN
    }

    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsMessageEvent(event);

    // Iterate over all messages in this ACK event and retrieve their
    // correlationIds, while removing the entry from the underlying
    // 'internal correlationId' => 'user-provided correlationId' map, if
    // applicable (i.e., if internal correlationId is non-null)

    bmqp::AckMessageIterator it;
    event.loadAckMessageIterator(&it);
    int numAckMsgs = 0;
    while (it.next()) {
        ++numAckMsgs;
        const bmqp::AckMessage& ackMsg = it.message();

        // Lookup queue
        const bsl::shared_ptr<Queue>& queue = d_queueManager.lookupQueue(
            bmqp::QueueId(ackMsg.queueId()));
        BSLS_ASSERT_SAFE(queue);

        if (ackMsg.status() != 0) {
            // Non-zero ack status. Log it.
            BMQU_THROTTLEDACTION_THROTTLE(
                d_throttledFailedAckMessages,
                BALL_LOG_ERROR
                    << "Failed ACK for queue '" << queue->uri()
                    << "' [status: "
                    << bmqp::ProtocolUtil::ackResultFromCode(ackMsg.status())
                    << ", GUID: " << ackMsg.messageGUID() << "]";);
        }

        bmqt::CorrelationId correlationId;
        if (d_messageCorrelationIdContainer.remove(ackMsg.messageGUID(),
                                                   &correlationId) != 0) {
            // There is no correlationId associated with this GUID.
            // Per contract, broker does not send ACKs where status is zero and
            // correlationId is null.
            BSLS_ASSERT_SAFE(0 != ackMsg.status());
        }

        // Keep track of user-provided CorrelationId (it may be unset)
        queueEvent->addCorrelationId(correlationId);

        // Insert queue into event
        queueEvent->insertQueue(queue);
    }

    BSLS_ASSERT_SAFE(numAckMsgs == queueEvent->numCorrrelationIds());

    // Add to event queue. Note that we are now forwarding an ACK event which
    // may contain certain unset correlationIds with non-zero ack status.
    d_eventQueue.pushBack(queueEvent);

    // Update stats
    d_eventsStats.onEvent(EventsStatsEventType::e_ACK,
                          event.blob()->length(),
                          numAckMsgs);
}

bmqt::OpenQueueResult::Enum
BrokerSession::openQueueImp(const bsl::shared_ptr<Queue>&  queue,
                            bsls::TimeInterval             timeout,
                            RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (!d_acceptRequests) {
        // The session is being/has been stopped or not started: client should
        // not send any request to the broker.
        BALL_LOG_ERROR << "Unable to process openQueue request "
                       << "[reason: 'SESSION_STOPPED']: " << *queue;
        return bmqt::OpenQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    // Ensure queue correlationId is unique
    {
        const bsl::shared_ptr<Queue>& queueLookup = lookupQueue(
            queue->correlationId());

        if (queueLookup) {
            return bmqt::OpenQueueResult::
                e_CORRELATIONID_NOT_UNIQUE;  // RETURN
        }
    }

    // If queue can be found by URI the FSM will work with this existed object
    bsl::shared_ptr<Queue> queueLookup = lookupQueue(queue->uri());

    if (!queueLookup) {
        // Not found. Proceed with a new queue object
        queueLookup = queue;
    }

    BALL_LOG_INFO << "Opening queue "
                  << "[queue: " << *queueLookup << ", timeout: " << timeout
                  << "]";

    return d_queueFsm.handleOpenRequest(queueLookup, timeout, context);
}

bmqt::OpenQueueResult::Enum BrokerSession::sendOpenQueueRequest(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue,
    bsls::TimeInterval                   timeout)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->request().choice().isOpenQueueValue());

    const bsls::TimeInterval absTimeout = bmqsys::Time::nowMonotonicClock() +
                                          timeout;

    RequestManagerType::RequestType::ResponseCb response =
        bdlf::BindUtil::bind(&BrokerSession::onOpenQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // context
                             queue,
                             absTimeout);
    context->setResponseCb(response);

    bmqt::GenericResult::Enum rc = sendRequest(
        context,
        bmqp::QueueId(queue->id(), queue->subQueueId()),
        timeout);

    return static_cast<bmqt::OpenQueueResult::Enum>(rc);
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::configureQueueImp(const RequestManagerType::RequestSp& context,
                                 const bsl::shared_ptr<Queue>&        queue,
                                 const bmqt::QueueOptions& newClientOptions,
                                 const bsls::TimeInterval  timeout,
                                 const ConfiguredCallback& configuredCb,
                                 const bool                checkConcurrent)
{
    // executed by the FSM thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(queue->state() != QueueState::e_CLOSED);

    BALL_LOG_INFO << "Configure queue "
                  << " [queue: " << *queue
                  << ", newClientOptions: " << newClientOptions
                  << ", timeout: " << timeout << "]";

    // Check if there is a concurrent configure
    // NOTE: It is not possible to have a concurrent configure for 'queue' if
    //       this configure is the 2nd part of the openQueue flow.
    if (checkConcurrent &&
        queue->pendingConfigureId() != Queue::k_INVALID_CONFIGURE_ID) {
        BALL_LOG_WARN << "Unable to process configureQueue request "
                      << "[reason: 'CONFIGURE_PENDING', "
                      << " pendingConfigureId: " << queue->pendingConfigureId()
                      << "]: " << context->request();

        return bmqt::ConfigureQueueResult::e_ALREADY_IN_PROGRESS;  // RETURN
    }

    // Check if disconnecting
    if (!d_acceptRequests) {
        // The session is being/has been stopped or not started: client should
        // not send any request to the broker.
        BALL_LOG_ERROR << "Unable to process configureQueue request "
                       << "[reason: 'SESSION_STOPPED']: "
                       << context->request();

        return bmqt::ConfigureQueueResult::e_NOT_CONNECTED;  // RETURN
    }

    // Set the new queue options, while maintaining a copy of the current
    // options to use in 'onConfigureQueueResponse'.
    //
    // In some circumstances (e.g., resuming a suspended queue), we push queue
    // options to the broker without updating the client's preferred options
    // for the queue. In such cases, we leave 'queue->options()' unmodified.
    bmqt::QueueOptions previousOptions(queue->options(), d_allocator_p);
    queue->setOptions(newClientOptions);

    // Set the response callback
    RequestManagerType::RequestType::ResponseCb response =
        bdlf::BindUtil::bind(&BrokerSession::onConfigureQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // context
                             queue,
                             previousOptions,
                             configuredCb);
    context->setResponseCb(response);

    bmqt::GenericResult::Enum rc = sendRequest(
        context,
        bmqp::QueueId(queue->id(), queue->subQueueId()),
        timeout);

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        // Failure to send
        BALL_LOG_ERROR << "Error while sending configureQueue request: [rc: "
                       << rc << ", request: " << context->request() << "]";

        // Revert to previous QueueOptions
        queue->setOptions(previousOptions);
    }
    else if (queue->pendingConfigureId() == Queue::k_INVALID_CONFIGURE_ID) {
        // Set the pending configure request id
        queue->setPendingConfigureId(context->request().rId().value());
    }

    return static_cast<bmqt::ConfigureQueueResult::Enum>(rc);
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::sendConfigureRequest(const bsl::shared_ptr<Queue>&  queue,
                                    const bmqt::QueueOptions&      options,
                                    const bsls::TimeInterval       timeout,
                                    RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Configure queue currently only makes sense for a reader, and is
    // therefore a no-op for a writer only.
    if (!bmqt::QueueFlagsUtil::isReader(queue->flags())) {
        BALL_LOG_INFO << "Skipping configure queue (reason: not a reader): "
                      << *queue;
        // Will signal and trigger the event callback if there is one.
        // The below signaling on the request is what the 'configuredCb' would
        // have done (in the case of a standalone configure, 'configuredCb' is
        // set to 'onConfigureQueueConfigured').
        context->signal();
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    if (queue->options().suspendsOnBadHostHealth() && !isHostHealthy()) {
        queue->setOptions(options);
        context->signal();
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    const ConfiguredCallback configuredCb = bdlf::BindUtil::bind(
        &BrokerSession::onConfigureQueueConfigured,
        this,
        bdlf::PlaceHolders::_1,   // context
        bdlf::PlaceHolders::_2);  // queue

    return configureQueueImp(context, queue, options, timeout, configuredCb);
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::sendReconfigureRequest(const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    RequestManagerType::RequestSp context = createConfigureQueueContext(
        queue,
        queue->options(),
        false,   // isDeconfigure
        false);  // isBuffered
    if (queue->isSuspended()) {
        context->signal();
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    const ConfiguredCallback configuredCb = bdlf::BindUtil::bind(
        &BrokerSession::onConfigureQueueConfigured,
        this,
        bdlf::PlaceHolders::_1,   // context
        bdlf::PlaceHolders::_2);  // queue

    return configureQueueImp(context,
                             queue,
                             queue->options(),
                             d_sessionOptions.configureQueueTimeout(),
                             configuredCb);
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::sendSuspendRequest(const bsl::shared_ptr<Queue>&  queueSp,
                                  const bmqt::QueueOptions&      options,
                                  const bsls::TimeInterval       timeout,
                                  RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Increment number of Host Health requests outstanding.
    ++d_numPendingHostHealthRequests;

    // Configure queue currently only makes sense for a reader, and is
    // therefore a no-op for a writer only.
    if (!bmqt::QueueFlagsUtil::isReader(queueSp->flags())) {
        BALL_LOG_INFO << "Skipping configure queue (reason: not a reader): "
                      << *queueSp;
        // Will signal and trigger the event callback if there is one.
        // The below signaling on the request is what the 'configuredCb' would
        // have done (in the case of a standalone configure, 'configuredCb' is
        // set to 'onConfigureQueueConfigured').
        queueSp->setOptions(options);
        onSuspendQueueConfigured(context, queueSp, false);
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    // Call 'configureQueueImp' to configure 'queue' via the request 'context'
    // with the provided 'suspendOptions' and 'timeout'. The bound callback
    // will be invoked asynchronously, if and only if 'rc' is 'e_SUCCESS'.
    BALL_LOG_INFO << "Suspending queue [queue: " << *queueSp << "]";
    bmqt::ConfigureQueueResult::Enum rc = configureQueueImp(
        context,
        queueSp,
        options,
        timeout,
        bdlf::BindUtil::bind(&BrokerSession::onSuspendQueueConfigured,
                             this,
                             bdlf::PlaceHolders::_1,  // context
                             bdlf::PlaceHolders::_2,  // queue
                             false));

    if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // If the request failed, inject an 'e_UNKNOWN' into the response, and
        // invoke the callback ourselves. Include whether the request was
        // deferred (i.e. another configure is already in progress); this is
        // used to avoid issuing session-events to the user.
        d_queueFsm.injectErrorResponse(context,
                                       bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                       "");
        onSuspendQueueConfigured(
            context,
            queueSp,
            rc == bmqt::ConfigureQueueResult::e_ALREADY_IN_PROGRESS);
    }
    return rc;  // RETURN
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::sendResumeRequest(const bsl::shared_ptr<Queue>&  queueSp,
                                 const bmqt::QueueOptions&      options,
                                 const bsls::TimeInterval       timeout,
                                 RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Increment number of Host Health requests outstanding.
    ++d_numPendingHostHealthRequests;

    // Configure queue currently only makes sense for a reader, and is
    // therefore a no-op for a writer only.
    if (!bmqt::QueueFlagsUtil::isReader(queueSp->flags())) {
        BALL_LOG_INFO << "Skipping configure queue (reason: not a reader): "
                      << *queueSp;
        // Will signal and trigger the event callback if there is one.
        // The below signaling on the request is what the 'configuredCb' would
        // have done (in the case of a standalone configure, 'configuredCb' is
        // set to 'onConfigureQueueConfigured').
        queueSp->setOptions(options);
        onResumeQueueConfigured(context, queueSp, false);
        return bmqt::ConfigureQueueResult::e_SUCCESS;  // RETURN
    }

    // Call 'configureQueueImp' to configure 'queue' via the request 'context'
    // with the provided 'options' and 'timeout'. The bound callback will be
    // invoked asynchronously, if and only if 'rc' is 'e_SUCCESS'.
    bmqt::ConfigureQueueResult::Enum rc = configureQueueImp(
        context,
        queueSp,
        options,
        timeout,
        bdlf::BindUtil::bind(&BrokerSession::onResumeQueueConfigured,
                             this,
                             bdlf::PlaceHolders::_1,  // context
                             bdlf::PlaceHolders::_2,  // queue
                             false));

    if (rc != bmqt::ConfigureQueueResult::e_SUCCESS) {
        // If the request failed, inject an 'e_UNKNOWN' into the response, and
        // invoke the callback ourselves. Include whether the request was
        // deferred (i.e. another configure is already in progress); this is
        // used to avoid issuing session-events to the user.
        d_queueFsm.injectErrorResponse(context,
                                       bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                       "");
        onResumeQueueConfigured(
            context,
            queueSp,
            rc == bmqt::ConfigureQueueResult::e_ALREADY_IN_PROGRESS);
    }
    return rc;  // RETURN
}

bmqt::ConfigureQueueResult::Enum
BrokerSession::sendDeconfigureRequest(const bsl::shared_ptr<Queue>& queue)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Prepare the null queue options (stream parameters)
    bmqt::QueueOptions options(d_allocator_p);
    options.setMaxUnconfirmedMessages(0)
        .setMaxUnconfirmedBytes(0)
        .setConsumerPriority(bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);

    // Set the configure callback
    RequestManagerType::RequestSp closeQueueContext =
        d_requestManager.createRequest();
    closeQueueContext->request().choice().makeCloseQueue();
    closeQueueContext->setGroupId(k_NON_BUFFERED_REQUEST_GROUP_ID);

    const bsls::TimeInterval absTimeout =
        bmqsys::Time::nowMonotonicClock() +
        d_sessionOptions.configureQueueTimeout();
    const ConfiguredCallback configuredCb = bdlf::BindUtil::bind(
        &BrokerSession::onCloseQueueConfigured,
        this,
        bdlf::PlaceHolders::_1,  // configureQueue request
        bdlf::PlaceHolders::_2,  // queue
        closeQueueContext,       // closeQueue request
        absTimeout,
        true);  // isFinal

    RequestManagerType::RequestSp configureQueueContext =
        createConfigureQueueContext(queue,
                                    options,
                                    true,    // isDeconfigure
                                    false);  // isBuffered

    return configureQueueImp(configureQueueContext,
                             queue,
                             options,
                             d_sessionOptions.configureQueueTimeout(),
                             configuredCb);
}

bmqt::ConfigureQueueResult::Enum BrokerSession::sendDeconfigureRequest(
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& closeContext,
    bsls::TimeInterval                   timeout)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(closeContext->request().choice().isCloseQueueValue());
    BSLS_ASSERT_SAFE(queue->pendingConfigureId() ==
                     Queue::k_INVALID_CONFIGURE_ID);

    // Set the 'isFinal' flag
    const bool isFinal = d_queueManager.subStreamCount(
                             queue->uri().canonical()) == 1;

    // Set the configure callback
    const bsls::TimeInterval absTimeout = bmqsys::Time::nowMonotonicClock() +
                                          timeout;
    const ConfiguredCallback configuredCb = bdlf::BindUtil::bind(
        &BrokerSession::onCloseQueueConfigured,
        this,
        bdlf::PlaceHolders::_1,  // configureQueue context
        bdlf::PlaceHolders::_2,  // queue
        closeContext,            // closeQueue context
        absTimeout,
        isFinal);  // isFinal

    // Prepare the queue options (stream parameters)
    bmqt::QueueOptions options(d_allocator_p);
    options.setMaxUnconfirmedMessages(0)
        .setMaxUnconfirmedBytes(0)
        .setConsumerPriority(bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> configureSpan(
        createDTSpan("bmq.queue.closeConfigure", baggage));
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(configureSpan));

    RequestManagerType::RequestSp configureQueueContext =
        createConfigureQueueContext(queue,
                                    options,
                                    true,    // isDeconfigure
                                    false);  // isBuffered
    configureQueueContext->setDTSpan(configureSpan);

    return configureQueueImp(configureQueueContext,
                             queue,
                             options,
                             timeout,
                             configuredCb);
}

void BrokerSession::doHandleStartTimeout(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    d_sessionFsm.handleStartTimeout();
}

void BrokerSession::doHandlePendingPutExpirationTimeout(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    bmqp::AckEventBuilder          ackBuilder(d_blobSpPool_p, d_allocator_p);
    bsl::vector<bmqt::MessageGUID> expiredKeys(d_allocator_p);
    bsl::shared_ptr<Event>         ackEvent = createEvent();
    bsl::shared_ptr<Queue>         queueSp;

    MessageCorrelationIdContainer::KeyIdsCb callback = bdlf::BindUtil::bind(
        &BrokerSession::cancelPendingMessageImp,
        this,
        &ackBuilder,
        &ackEvent,
        bdlf::PlaceHolders::_1,
        queueSp,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    const bsls::TimeInterval nextExpirationTime =
        d_messageCorrelationIdContainer.getExpiredIds(
            &expiredKeys,
            d_queueRetransmissionTimeoutMap,
            bmqsys::Time::nowMonotonicClock());
    d_messageCorrelationIdContainer.iterateAndInvoke(expiredKeys, callback);

    // Push the final ack event if there are any messages in the builder
    if (ackBuilder.messageCount()) {
        transferAckEvent(&ackBuilder, &ackEvent);
    }

    if (nextExpirationTime > 0) {
        // Setup next timer
        setupPutExpirationTimer(nextExpirationTime);
    }
}

void BrokerSession::doHandleChannelWatermark(
    bmqio::ChannelWatermarkType::Enum type,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Assume that watermark notification cannot come after channel down event
    BSLS_ASSERT_SAFE(d_channel_sp);

    // HWM condition is detected by the channel 'write' result, so we do not
    // handle HWM here
    if (type == bmqio::ChannelWatermarkType::e_HIGH_WATERMARK) {
        BALL_LOG_INFO << "HWM: Channel is not writable";
        return;  // RETURN
    }

    BALL_LOG_INFO << "LWM: Channel is writable";

    while (!d_extensionBlobBuffer.empty()) {
        // We expect the channel is ready for writing so we write control blobs
        // one by one.  If writing fails we won't notify user threads because
        // either there is the HWM again and we will expect the next LWM event,
        // or the channel is down and the user threads will be released when
        // the channel down event is handled (see 'handleChannelDown').
        bmqio::Status status(d_allocator_p);
        d_channel_sp->write(&status,
                            d_extensionBlobBuffer.front(),
                            d_sessionOptions.channelHighWatermark());

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_ERROR << "Failed to send buffered control messages ";

            break;  // BREAK
        }
        d_extensionBlobBuffer.pop_front();
    }
    if (d_extensionBlobBuffer.empty()) {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_extensionBufferLock);  // LOCK
        d_extensionBufferEmpty = true;

        BALL_LOG_INFO << "LWM: Channel is ready for user messages";
        d_extensionBufferCondition.broadcast();
    }
}

void BrokerSession::doHandleHeartbeat(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // The broker sent a heartbeat to check on us, simply reply with a
    // heartbeat response.
    //
    // NOTE: the client doesn't check on the broker, therefore it will never
    //       send 'HEARTBEAT_REQ' and hence we don't have to handle
    //       'HEARTBEAT_RSP' type.
    if (d_channel_sp) {
        d_channel_sp->write(0,  // status
                            bmqp::ProtocolUtil::heartbeatRspBlob(),
                            d_sessionOptions.channelHighWatermark());
        // We explicitly ignore any failure as failure implies issues with the
        // channel, which is what the heartbeat is trying to expose.
    }
}

void BrokerSession::enqueueStateRestoredIfNeeded()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < d_numPendingReopenQueues);

    // Send pending PUTs and enqueue a STATE_RESTORED event once all queues
    // have been reopened
    if (--d_numPendingReopenQueues == 0) {
        retransmitPendingMessages();
        enqueueSessionEvent(bmqt::SessionEventType::e_STATE_RESTORED);
    }
}

void BrokerSession::onSuspendQueueConfigured(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queueSp,
    const bool                           deferred)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    BALL_LOG_INFO << "Handling suspend response [queue: " << *queueSp
                  << ", deferred: " << deferred << "]";

    if (context->isLateResponse()) {
        // For suspend request, response timeout is interpreted as a severe
        // problem with the underlying connections. We therefore drop the
        // channel, which means we should not get late responses.
        //
        // Nonetheless, we exit early here to avoid double-decrementing the
        // 'd_numPendingHostHealthRequests' counter, just in case.
        BALL_LOG_WARN << "Received late suspend response; ignoring";
        return;  // RETURN
    }

    // Indicate one fewer host-health queue request is pending.
    //
    // A `resume` request may fire a HOST_HEALTH_RESTORED event only when this
    // counter reaches zero.
    BSLS_ASSERT_SAFE(0 < d_numPendingHostHealthRequests);
    --d_numPendingHostHealthRequests;

    // If there was already a configure request in progress (e.g. from client),
    // then this queue-suspend is "deferred", and will re-execute when that
    // response returns (if the host is still unhealthy). In such cases, the
    // session-events will be enqueued at that time.
    if (!deferred) {
        // We don't handle late suspend responses, so clear this if attempted
        // to send suspend request.
        queueSp->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);
        queueSp->setIsSuspendedWithBroker(true);

        if (queueSp->isOpened() &&
            context->result() != bmqt::GenericResult::e_CANCELED) {
            bmqt::GenericResult::Enum status = context->result();

            // We issue two e_QUEUE_SUSPENDED events in succession.
            // 1. An event with callback attached, which blocks PUTs to queue.
            // 2. An event notifying client of the queue's suspension.
            // When user reads (1), the callback will be executed and consumed
            // by the SDK; (2) will then be returned to the user. This will be
            // performed transactionally within a single application thread.
            enqueueSessionEvent(
                bmqt::SessionEventType::e_QUEUE_SUSPENDED,
                status,
                "",
                bmqt::CorrelationId(),
                queueSp,
                bdlf::BindUtil::bind(&applyQueueSuspension, queueSp, true));
            enqueueSessionEvent(bmqt::SessionEventType::e_QUEUE_SUSPENDED,
                                status,
                                "",
                                bmqt::CorrelationId(),
                                queueSp);

            // If the request wasn't successful or canceled, drop the channel.
            if (context->result() != bmqt::GenericResult::e_SUCCESS) {
                BALL_LOG_ERROR << "Got suspend error; dropping channel "
                               << "[result: " << context->result()
                               << ", queue: " << *queueSp << "]";
                if (d_channel_sp) {
                    d_channel_sp->close();
                }
            }
        }
    }

    if (d_numPendingHostHealthRequests == 0 && isHostHealthy() &&
        context->result() == bmqt::GenericResult::e_CANCELED) {
        d_sessionFsm.handleAllQueuesResumed();
    }

    // Finish the response.
    context->signal();
}

void BrokerSession::onResumeQueueConfigured(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queueSp,
    const bool                           deferred)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (context->isLateResponse()) {
        BALL_LOG_WARN << "Received late resume response; ignoring";
        return;  // RETURN
    }

    // Indicate one fewer host-health queue request is pending.
    BSLS_ASSERT_SAFE(0 < d_numPendingHostHealthRequests);
    --d_numPendingHostHealthRequests;

    // If there was already a configure request in progress (e.g. from client),
    // then this queue-resume is "deferred", and will re-execute when that
    // response returns (if the host is still unhealthy). In such cases, the
    // session-events will be enqueued at that time.
    if (!deferred) {
        // We don't handle late suspend responses, so clear this if atempted
        // to send resume request.
        queueSp->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);
        queueSp->setIsSuspendedWithBroker(false);

        if (queueSp->isOpened() &&
            context->result() != bmqt::GenericResult::e_CANCELED) {
            bmqt::GenericResult::Enum status = context->result();

            // We issue two e_QUEUE_RESUMED events in succession.
            // 1. An event with callback attached, which allows PUTs to queue.
            // 2. An event notifying client of the queue's resumption.
            // When user reads (1), the callback will be executed and consumed
            // by the SDK; (2) will then be returned to the user. This will be
            // performed transactionally within a single application thread.
            enqueueSessionEvent(
                bmqt::SessionEventType::e_QUEUE_RESUMED,
                status,
                "",
                bmqt::CorrelationId(),
                queueSp,
                bdlf::BindUtil::bind(&applyQueueSuspension, queueSp, false));
            enqueueSessionEvent(bmqt::SessionEventType::e_QUEUE_RESUMED,
                                status,
                                "",
                                bmqt::CorrelationId(),
                                queueSp);
        }

        // If there are no more outstanding requests, AND the host remains
        // healthy, then issue a HOST_HEALTH_RESTORED event.
        if (d_numPendingHostHealthRequests == 0 && isHostHealthy()) {
            d_sessionFsm.handleAllQueuesResumed();
        }
    }

    // Finish the response.
    context->signal();
}

void BrokerSession::transferAckEvent(bmqp::AckEventBuilder*  ackBuilder,
                                     bsl::shared_ptr<Event>* ackEvent)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(ackBuilder);
    BSLS_ASSERT_SAFE(ackEvent);

    // Our event is full at this point so send this ack event to the user and
    // reset the builder to append the ack that was rejected.

    bmqp::Event event(&ackBuilder->blob(), d_allocator_p, true);
    // clone = true
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_ACK>())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO_BLOCK
        {
            d_messageDumper.dumpAckEvent(BALL_LOG_OUTPUT_STREAM, event);
        }
    }

    (*ackEvent)->configureAsMessageEvent(event);

    BSLS_ASSERT_SAFE(ackBuilder->messageCount() ==
                     (*ackEvent)->numCorrrelationIds());

    // Add to event queue. Note that we are now forwarding an ACK event which
    // may contain certain unset correlationIds with non-zero ack status.
    d_eventQueue.pushBack(*ackEvent);

    // Update stats
    d_eventsStats.onEvent(EventsStatsEventType::e_ACK,
                          event.blob()->length(),
                          ackBuilder->messageCount());

    ackBuilder->reset();
    *ackEvent = createEvent();
}

void BrokerSession::cancel(const bsl::shared_ptr<Queue>&       queue,
                           bmqp_ctrlmsg::StatusCategory::Value status,
                           const bslstl::StringRef&            reason)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Cancel pending configureQueue
    if (queue->pendingConfigureId() != Queue::k_INVALID_CONFIGURE_ID) {
        // Prepare canceled response.
        bmqp_ctrlmsg::ControlMessage controlMessage;
        controlMessage.rId().makeValue(queue->pendingConfigureId());

        bmqp::ControlMessageUtil::makeStatus(&controlMessage,
                                             status,
                                             -1,
                                             reason);

        d_requestManager.processResponse(controlMessage);
    }

    // Cancel buffered requests related to this queue.
    if (queue->requestGroupId().has_value()) {
        cancel(queue->requestGroupId().value(), status, reason);
    }
}

void BrokerSession::cancel(bmqp_ctrlmsg::StatusCategory::Value status,
                           const bslstl::StringRef&            reason)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    cancel(-1, status, reason);
}

void BrokerSession::cancel(int                                 groupId,
                           bmqp_ctrlmsg::StatusCategory::Value status,
                           const bslstl::StringRef&            reason)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Cancel all pending requests with CANCELED reason.
    bmqp_ctrlmsg::ControlMessage controlMessage;
    bmqp::ControlMessageUtil::makeStatus(&controlMessage, status, -1, reason);
    if (groupId >= 0) {
        d_requestManager.cancelAllRequests(controlMessage, groupId);
    }
    else {
        d_requestManager.cancelAllRequests(controlMessage);
    }
}

bool BrokerSession::cancelPendingMessageImp(
    bmqp::AckEventBuilder*                                      ackBuilder,
    bsl::shared_ptr<Event>*                                     ackEvent,
    bool*                                                       deleteItem,
    const bsl::shared_ptr<Queue>&                               queueSp,
    const bmqt::MessageGUID&                                    guid,
    const MessageCorrelationIdContainer::QueueAndCorrelationId& qac)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    const bool res = false;  // never interrupt
    *deleteItem    = false;

    if (queueSp) {
        bmqp::QueueId qid(queueSp->id(), queueSp->subQueueId());
        if (!(qid == qac.d_queueId)) {
            // If the specified queueId is not invalid we are looking for a
            // specific queue. If the current queueId doesn't match that
            // specific queue then we would like to skip any logic associated
            // with sending acks.
            return res;  // RETURN
        }
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(qac.d_messageType ==
                                              bmqp::EventType::e_CONTROL)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Buffered CONTROL message will be deleted when related request is
        // cancelled.  If the cancelation is for the particular queue fire
        // assert because such item should have been already deleted.
        if (queueSp) {
            BALL_LOG_ERROR << "Unexpected pending request: "
                           << qac.d_requestContext->request();
            BSLS_ASSERT_SAFE(false && "Unexpected pending request");
        }
        return res;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            qac.d_queueId.id() == bmqimp::Queue::k_INVALID_QUEUE_ID)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // The item has no associated queueId.  This may happen if the user has
        // started an event but hasn't post it yet.  In this case the NACK
        // should not be sent and the item should not be removed at this point.
        return res;  // RETURN
    }

    // The item will be deleted from the retransmission buffer.
    *deleteItem = true;

    if (!bmqp::PutHeaderFlagUtil::isSet(
            qac.d_header.flags(),
            bmqp::PutHeaderFlags::e_ACK_REQUESTED)) {
        // If the message header doesn't have ACK_REQUESTED flag such message
        // shouldn't be NACKed, just deleted.
        return res;  // RETURN
    }

    const int k_ACK_STATUS_SUCCESS = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_SUCCESS);
    const int k_ACK_STATUS_UNKNOWN = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_UNKNOWN);

    int ackStatus = k_ACK_STATUS_UNKNOWN;

    if (queueSp) {
        ackStatus = queueSp->atMostOnce() ? k_ACK_STATUS_SUCCESS
                                          : k_ACK_STATUS_UNKNOWN;
    }

    bmqt::EventBuilderResult::Enum rc = bmqp::ProtocolUtil::buildEvent(
        bdlf::BindUtil::bind(&bmqp::AckEventBuilder::appendMessage,
                             ackBuilder,
                             ackStatus,
                             bmqp::AckMessage::k_NULL_CORRELATION_ID,
                             guid,
                             qac.d_queueId.id()),
        bdlf::BindUtil::bind(&BrokerSession::transferAckEvent,
                             this,
                             ackBuilder,
                             ackEvent));

    if (rc != bmqt::EventBuilderResult::e_SUCCESS) {
        BALL_LOG_ERROR << "Failed to append ACK/NACK [rc: " << rc
                       << ", GUID: " << guid << ", queueId: " << qac.d_queueId;
        return res;  // RETURN
    }

    // Keep track of user-provided CorrelationId (it may be unset)
    (*ackEvent)->addCorrelationId(qac.d_correlationId);

    // Insert queue into event
    bsl::shared_ptr<Queue> queue = queueSp;
    if (!queue) {
        // Lookup queue
        queue = d_queueManager.lookupQueue(bmqp::QueueId(qac.d_queueId));
        BSLS_ASSERT_SAFE(queue);
    }
    (*ackEvent)->insertQueue(queue);

    return res;
}

void BrokerSession::cancelPendingMessages(
    const bsl::shared_ptr<Queue>& queueSp)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    bmqp::AckEventBuilder  ackBuilder(d_blobSpPool_p, d_allocator_p);
    bsl::shared_ptr<Event> ackEvent = createEvent();

    MessageCorrelationIdContainer::KeyIdsCb callback = bdlf::BindUtil::bind(
        &BrokerSession::cancelPendingMessageImp,
        this,
        &ackBuilder,
        &ackEvent,
        bdlf::PlaceHolders::_1,
        queueSp,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    d_messageCorrelationIdContainer.iterateAndInvoke(callback);

    // Push the final ack event if there are any messages in the builder
    if (ackBuilder.messageCount()) {
        transferAckEvent(&ackBuilder, &ackEvent);
    }
}

bool BrokerSession::retransmitControlMessage(
    bool*                                                       deleteItem,
    const MessageCorrelationIdContainer::QueueAndCorrelationId& qac)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(qac.d_messageType == bmqp::EventType::e_CONTROL);
    BSLS_ASSERT_SAFE(qac.d_requestContext);
    BSLS_ASSERT_SAFE(qac.d_messageData.length() > 0);

    *deleteItem = false;

    BALL_LOG_DEBUG << "Adding control request for retransmission: "
                   << qac.d_requestContext->request();

    // Control event blob is ready, send it to the channel.  The request
    // should't be deleted from the retransmission buffer until the response
    // comes or the request timeout happens.
    bmqt::GenericResult::Enum res = writeOrBuffer(
        qac.d_messageData,
        d_sessionOptions.channelHighWatermark());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            res != bmqt::GenericResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Failed to send pending CONTROL message: " << res;

        // Interrupt iteration
        return true;  // RETURN
    }

    return false;
}

bool BrokerSession::appendOrSend(
    bool*                                                       interrupt,
    bmqp::PutEventBuilder&                                      builder,
    const MessageCorrelationIdContainer::QueueAndCorrelationId& qac)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(qac.d_messageType == bmqp::EventType::e_PUT);
    BSLS_ASSERT_SAFE(qac.d_messageData.length() > 0);

    *interrupt = false;

    builder.startMessage();
    builder.setMessagePayload(&qac.d_messageData)
        .setMessageGUID(qac.d_header.messageGUID())
        .setCrc32c(qac.d_header.crc32c())
        .setCompressionAlgorithmType(qac.d_header.compressionAlgorithmType())
        .setFlags(qac.d_header.flags())
        .setMessagePropertiesInfo(bmqp::MessagePropertiesInfo(qac.d_header));

    BALL_LOG_DEBUG << "Adding PUT message for retransmission ["
                   << "GUID: '" << builder.messageGUID() << "'] ";

    const bmqt::EventBuilderResult::Enum result = builder.packMessageRaw(
        qac.d_queueId.id());
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(
            result == bmqt::EventBuilderResult::e_SUCCESS)) {
        return true;  // RETURN
    }

    if (result == bmqt::EventBuilderResult::e_PAYLOAD_TOO_BIG) {
        // Send the current event, reset the builder.
        bmqt::GenericResult::Enum res = writeOrBuffer(
            builder.blob(),
            d_sessionOptions.channelHighWatermark());

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                res != bmqt::GenericResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_ERROR << "Failed to send pending PUT event: " << res;

            *interrupt = true;
        }

        builder.reset();

        return false;  // RETURN
    }
    BALL_LOG_ERROR << "Failed to append PUT message [rc: " << result
                   << ", correlationId: " << qac.d_header.correlationId()
                   << ", queueId: " << qac.d_queueId << "]";

    BSLS_ASSERT_SAFE(false && "Failed to build PUT event");

    return false;
}

bool BrokerSession::handlePendingMessage(
    bmqp::PutEventBuilder*                                      putBuilder,
    bool*                                                       deleteItem,
    const bmqt::MessageGUID&                                    guid,
    const MessageCorrelationIdContainer::QueueAndCorrelationId& qac)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    *deleteItem    = false;
    bool interrupt = false;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(qac.d_messageType ==
                                              bmqp::EventType::e_UNDEFINED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // The item my be added from the user thread but not yet handled in the
        // FSM.  Skip it.
        BALL_LOG_DEBUG << "Skip pending message [no payload]. GUID: " << guid;
        return interrupt;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(qac.d_messageType ==
                                              bmqp::EventType::e_CONTROL)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        return retransmitControlMessage(deleteItem, qac);  // RETURN
    }

    BSLS_ASSERT_SAFE(qac.d_messageType == bmqp::EventType::e_PUT);
    BSLS_ASSERT_SAFE(qac.d_messageData.length() > 0);

    bmqp::PutEventBuilder& builder      = *putBuilder;
    const bool             ackRequested = bmqp::PutHeaderFlagUtil::isSet(
        qac.d_header.flags(),
        bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    // Expect to append PUT message to the event from the first attempt.  If it
    // fails due to PAYLOAD_TOO_BIG error, build and send the current event,
    // start a new one and repeat the message appending.
    bool appended = appendOrSend(&interrupt, builder, qac);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(interrupt)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return interrupt;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!appended)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Second attemt to append.
        appended = appendOrSend(&interrupt, builder, qac);
    }

    BSLS_ASSERT_SAFE(appended);

    // Mark the messages without ACK_REQUESTED flag to be deleted.
    *deleteItem = !ackRequested;

    return interrupt;
}

void BrokerSession::retransmitPendingMessages()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Cancel pending PUTs expiration timer
    d_scheduler_p->cancelEvent(&d_messageExpirationTimeoutHandle);

    bmqp::PutEventBuilder putBuilder(d_blobSpPool_p, d_allocator_p);

    MessageCorrelationIdContainer::KeyIdsCb callback = bdlf::BindUtil::bind(
        &BrokerSession::handlePendingMessage,
        this,
        &putBuilder,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    BALL_LOG_DEBUG << "Pending messages [PUTs: "
                   << d_messageCorrelationIdContainer.numberOfPuts()
                   << "] [CONTROLs: "
                   << d_messageCorrelationIdContainer.numberOfControls()
                   << "] [Total: " << d_messageCorrelationIdContainer.size()
                   << "]";

    const bool allIterated = d_messageCorrelationIdContainer.iterateAndInvoke(
        callback);
    if (!allIterated) {
        BALL_LOG_ERROR << "Stop message retransmission. Bad channel.";
        return;  // RETURN
    }

    // Send the final PUT event if there are any messages in the builder
    if (putBuilder.messageCount()) {
        bmqt::GenericResult::Enum res = writeOrBuffer(
            putBuilder.blob(),
            d_sessionOptions.channelHighWatermark());

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                res != bmqt::GenericResult::e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BALL_LOG_ERROR << "Failed to send pending PUT event: " << res;
        }
    }
}

bmqt::GenericResult::Enum
BrokerSession::enqueueFsmEvent(bsl::shared_ptr<Event>& event)
{
    // executed by IO or application thread

    BALL_LOG_TRACE_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "Enqueuing to FSM: ";
        if (event) {
            BALL_LOG_OUTPUT_STREAM << *event;
        }
        else {
            BALL_LOG_OUTPUT_STREAM << " *NULL* event";
        }
    }

    int rc = d_fsmEventQueue.pushBack(bslmf::MovableRefUtil::move(event));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false && "Impossible - failed to enqueue FSM event");
        BALL_LOG_ERROR << "Failed to enqueue FSM event: " << *event;
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

void BrokerSession::doStart(
    bslmt::Semaphore*            semaphore,
    int*                         status,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp,
    const bsl::shared_ptr<bmqpi::DTSpan>&                span)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // If Distributed Trace is enabled, activate this span for the lifetime of
    // the operation.
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(span));

    if (d_sessionOptions.hostHealthMonitor()) {
        // Add a host-health callback to our HostHealthMonitor.
        bdlmt::SignalerConnection conn =
            d_sessionOptions.hostHealthMonitor()->observeHostHealth(
                bdlf::BindUtil::bind(&BrokerSession::onHostHealthStateChange,
                                     this,
                                     bdlf::PlaceHolders::_1));

        // Wrap it in a SignalerConnectionGuard for easy RAII semantics.
        bdlmt::SignalerConnectionGuard guard(
            bslmf::MovableRefUtil::move(conn));
        d_hostHealthSignalerConnectionGuard = bslmf::MovableRefUtil::move(
            guard);
        // Track SignalerConnectionGuard using internally kept field.

        // Derive the initial host health state, and let the client know if the
        // host is unhealthy during session startup.
        bool wasHostHealthy = isHostHealthy();
        d_hostHealthState = d_sessionOptions.hostHealthMonitor()->hostState();
        if (wasHostHealthy && !isHostHealthy()) {
            enqueueSessionEvent(bmqt::SessionEventType::e_HOST_UNHEALTHY);
        }
    }

    *status = d_sessionFsm.handleStartRequest();
    if (*status != 0) {
        // Get out of the STARTING state since there going to be no
        // START_TIMEOUT
        d_sessionFsm.handleStartSynchronousFailure();
    }

    semaphore->post();
}

void BrokerSession::doStop(
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp,
    const bsl::shared_ptr<bmqpi::DTSpan>&                span)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // If Distributed Trace is enabled, activate this span for the lifetime of
    // the operation.
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(span));

    d_hostHealthSignalerConnectionGuard.release().disconnect();
    d_sessionFsm.handleStopRequest();
}

void BrokerSession::doOpenQueue(
    const bsl::shared_ptr<Queue>& queue,
    const bsls::TimeInterval      timeout,
    const FsmCallback&            fsmCallback,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp,
    const bsl::shared_ptr<bmqpi::DTSpan>&                span)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(fsmCallback);

    // If Distributed Trace is enabled, activate this span for the lifetime of
    // the operation.
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(span));

    // Create request and mark it as buffered so that it could be retransmitted
    // if it is not sent due to the session is not connected.
    RequestManagerType::RequestSp context = createOpenQueueContext(
        queue,
        fsmCallback,
        true);  // isBuffered
    context->setDTSpan(span);

    bmqt::OpenQueueResult::Enum rc = openQueueImp(queue, timeout, context);

    if (rc == bmqt::OpenQueueResult::e_SUCCESS) {
        // An event will be enqueued on the event queue and the caller will be
        // notified when the event is popped
        return;  // RETURN
    }

    // Failure - we still want to notify the user, so we need to "manually"
    //           enqueue an event on the event queue.
    // Inject an error response which will be handled outside the Queue FSM
    d_queueFsm.injectErrorResponse(
        context,
        static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc),
        bmqt::OpenQueueResult::toAscii(rc));

    context->signal();
}

void BrokerSession::doConfigureQueue(
    const bsl::shared_ptr<Queue>& queue,
    const bmqt::QueueOptions&     options,
    const bsls::TimeInterval      timeout,
    const FsmCallback&            fsmCallback,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp,
    const bsl::shared_ptr<bmqpi::DTSpan>&                span)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(fsmCallback);

    // If Distributed Trace is enabled, activate this span for the lifetime of
    // the operation.
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(span));

    bmqt::QueueOptions updatedOptions(queue->options());
    updatedOptions.merge(options);

    RequestManagerType::RequestSp context =
        createStandaloneConfigureQueueContext(queue,
                                              updatedOptions,
                                              fsmCallback);
    context->setDTSpan(span);

    bmqt::ConfigureQueueResult::Enum rc = d_queueFsm.handleConfigureRequest(
        queue,
        updatedOptions,
        timeout,
        context);

    if (rc == bmqt::ConfigureQueueResult::e_SUCCESS) {
        // An event will be enqueued on the event queue and the caller will be
        // notified when the event is popped
        return;  // RETURN
    }

    // Failure - we still want to notify the user, so we need to "manually"
    //           enqueue an event on the event queue.
    d_queueFsm.injectErrorResponse(
        context,
        static_cast<bmqp_ctrlmsg::StatusCategory::Value>(rc),
        bmqt::ConfigureQueueResult::toAscii(rc));

    context->signal();
}

void BrokerSession::doCloseQueue(
    const bsl::shared_ptr<Queue>& queue,
    const bsls::TimeInterval      timeout,
    const FsmCallback&            fsmCallback,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp,
    const bsl::shared_ptr<bmqpi::DTSpan>&                span)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(fsmCallback);

    // If Distributed Trace is enabled, activate this span for the lifetime of
    // the operation.
    bslma::ManagedPtr<void> scopedSpan(activateDTSpan(span));

    BALL_LOG_INFO << "Close queue [queue: " << *queue
                  << ", timeout: " << timeout << "]";

    RequestManagerType::RequestSp closeContext = createCloseQueueContext(
        fsmCallback);
    closeContext->setDTSpan(span);

    bmqt::CloseQueueResult::Enum res =
        d_queueFsm.handleCloseRequest(queue, timeout, closeContext);

    if (res != bmqt::CloseQueueResult::e_SUCCESS) {
        // Failure - we still want to notify the user, so we need to "manually"
        //           enqueue an event on the event queue.
        d_queueFsm.injectErrorResponse(
            closeContext,
            static_cast<bmqp_ctrlmsg::StatusCategory::Value>(res),
            bmqt::CloseQueueResult::toAscii(res));
        closeContext->signal();
    }
}

void BrokerSession::doSetChannel(
    const bsl::shared_ptr<bmqio::Channel> channel,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (channel) {  // We are now connected to bmqbrkr
        BALL_LOG_INFO << "Setting channel [host: " << channel->peerUri()
                      << "]";

        d_sessionFsm.handleChannelUp(channel);
    }
    else {  // We lost connection with bmqbrkr
        BALL_LOG_INFO << "Channel is RESET, state: " << d_sessionFsm.state();

        // Cancel pending requests before notifying the queues about channel
        // down.  This is needed to move all in-progress queues into EXPIRED
        // state and then close them, see 'QueueFsm::handleChannelDown'.  The
        // same happens if below 'notifyQueuesChannelDown' is called after
        // 'd_sessionFsm.handleChannelDown()'.  But we want to be sure that the
        // session FSM is called as the final action because it may release the
        // stop semaphore and further session destructor.
        cancel(k_NON_BUFFERED_REQUEST_GROUP_ID,
               bmqp_ctrlmsg::StatusCategory::E_CANCELED,
               "The request was canceled [reason: connection was lost]");
        // keep buffered requests

        notifyQueuesChannelDown();
        d_sessionFsm.handleChannelDown();
    }
}

void BrokerSession::disconnectChannel()
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (d_channel_sp) {
        d_channel_sp->close();
    }
    else {
        // There is no active channel and there will be no channel down event.
        // So the session is fully stopped.
        d_sessionFsm.handleChannelDown();
    }
}

void BrokerSession::resetState()
{
    // Reset the state of the brokerSession, so that start will work again
    d_acceptRequests = false;
    d_messageCorrelationIdContainer.reset();
    d_queueManager.resetState();
    d_numPendingReopenQueues = 0;
    d_messageDumper.reset();

    // Setting state back to e_HEALTHY ensures that if the session is reopened,
    // newly opened queues do not initiate into suspended state.
    d_hostHealthState = bmqt::HostHealthState::e_HEALTHY;

    // Reset the queues stat context to eliminate any references to queues used
    // in a previous run of the session
    if (d_queuesStats.d_statContext_mp) {
        d_queuesStats.d_statContext_mp->clearSubcontexts();
    }
    // Reset Event statistics
    d_eventsStats.resetStats();

    // Remove queue retransmission timeout data
    d_queueRetransmissionTimeoutMap.clear();

    // Release any writing user threads
    d_extensionBlobBuffer.clear();

    bslmt::LockGuard<bslmt::Mutex> guard(&d_extensionBufferLock);
    // LOCK
    d_extensionBufferEmpty = true;
    d_extensionBufferCondition.broadcast();
}

bmqt::GenericResult::Enum BrokerSession::disconnectBroker()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    RequestManagerType::RequestSp context = d_requestManager.createRequest();
    context->request().choice().makeDisconnect();
    context->setGroupId(k_NON_BUFFERED_REQUEST_GROUP_ID);

    RequestManagerType::RequestType::ResponseCb response =
        bdlf::BindUtil::bind(&BrokerSession::onDisconnectResponse,
                             this,
                             bdlf::PlaceHolders::_1);  // context
    context->setResponseCb(response);

    bmqp::QueueId queueId(bmqimp::Queue::k_INVALID_QUEUE_ID);

    bmqt::GenericResult::Enum rc =
        sendRequest(context, queueId, d_sessionOptions.disconnectTimeout());
    return rc;
}

BrokerSession::RequestManagerType::RequestSp
BrokerSession::createOpenQueueContext(const bsl::shared_ptr<Queue>& queue,
                                      const FsmCallback& fsmCallback,
                                      bool               isBuffered)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    int grId = k_NON_BUFFERED_REQUEST_GROUP_ID;
    queue->setRequestGroupId(++d_nextRequestGroupId);

    // Prepare the request context
    RequestManagerType::RequestSp context = d_requestManager.createRequest();
    if (isBuffered) {
        grId = queue->requestGroupId().value();
    }
    context->setGroupId(grId);

    bmqp_ctrlmsg::OpenQueue& openQueue =
        context->request().choice().makeOpenQueue();
    openQueue.handleParameters() = queue->handleParameters();

    // The result of this operation needs to be processed after all
    // pending/enqueued events in the event buffer.  The request may be
    // buffered if no connection and retransmitted once the session is
    // reconnected.
    context->setAsyncNotifierCb(fsmCallback);

    return context;
}

BrokerSession::RequestManagerType::RequestSp
BrokerSession::createConfigureQueueContext(const bsl::shared_ptr<Queue>& queue,
                                           const bmqt::QueueOptions& options,
                                           bool isDeconfigure,
                                           bool isBuffered)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(queue->requestGroupId().has_value());

    int grId = k_NON_BUFFERED_REQUEST_GROUP_ID;

    // Prepare the request context
    RequestManagerType::RequestSp context = d_requestManager.createRequest();
    if (isBuffered) {
        grId = queue->requestGroupId().value();
    }
    context->setGroupId(grId);

    if (d_doConfigureStream) {
        // Make ConfigureStream request
        bmqp_ctrlmsg::ConfigureStream& configureStream =
            context->request().choice().makeConfigureStream();
        configureStream.qId() = queue->id();

        // Populate the request's streamParameters
        bmqp_ctrlmsg::StreamParameters& streamParams =
            configureStream.streamParameters();

        if (!queue->hasDefaultSubQueueId()) {
            streamParams.appId() = queue->uri().id();
        }  // else  "__default"

        if (isDeconfigure) {
            // Empty Subscriptions
            return context;  // RETURN
        }

        bmqt::QueueOptions::SubscriptionsSnapshot snapshot(d_allocator_p);
        options.loadSubscriptions(&snapshot);

        for (bmqt::QueueOptions::SubscriptionsSnapshot::const_iterator cit =
                 snapshot.begin();
             cit != snapshot.end();
             ++cit) {
            bmqp_ctrlmsg::ConsumerInfo ci;
            const bmqt::Subscription&  from = cit->second;
            if (from.hasMaxUnconfirmedMessages()) {
                ci.maxUnconfirmedMessages() = from.maxUnconfirmedMessages();
            }
            else {
                ci.maxUnconfirmedMessages() = options.maxUnconfirmedMessages();
            }
            if (from.hasMaxUnconfirmedBytes()) {
                ci.maxUnconfirmedBytes() = from.maxUnconfirmedBytes();
            }
            else {
                ci.maxUnconfirmedBytes() = options.maxUnconfirmedBytes();
            }
            if (from.hasConsumerPriority()) {
                ci.consumerPriority() = from.consumerPriority();
            }
            else {
                ci.consumerPriority() = options.consumerPriority();
            }

            ci.consumerPriorityCount() = 1;

            bmqp_ctrlmsg::Subscription subscription(d_allocator_p);

            const unsigned int internalSubscriptionId =
                ++d_nextInternalSubscriptionId;

            subscription.sId() = internalSubscriptionId;
            // Using unique id instead of 'SubscriptionHandle::id()'

            subscription.consumers().emplace_back(ci);

            bmqp_ctrlmsg::ExpressionVersion::Value version;

            switch (from.expression().version()) {
            case bmqt::SubscriptionExpression::e_NONE:
                version = bmqp_ctrlmsg::ExpressionVersion::E_UNDEFINED;
                break;
            case bmqt::SubscriptionExpression::e_VERSION_1:
                version = bmqp_ctrlmsg::ExpressionVersion::E_VERSION_1;
                break;
            default:
                BSLS_ASSERT_SAFE(false);
                version = bmqp_ctrlmsg::ExpressionVersion::E_UNDEFINED;
                break;
            }
            subscription.expression().version() = version;
            subscription.expression().text()    = from.expression().text();

            streamParams.subscriptions().emplace_back(subscription);
            queue->registerInternalSubscriptionId(internalSubscriptionId,
                                                  cit->first.id(),
                                                  cit->first.correlationId());
        }
        return context;  // RETURN
    }

    // Make ConfigureQueueStream request
    bmqp_ctrlmsg::ConfigureQueueStream& configureQueueStream =
        context->request().choice().makeConfigureQueueStream();
    configureQueueStream.qId() = queue->id();

    // Populate the request's streamParameters
    bmqp_ctrlmsg::QueueStreamParameters& streamParams =
        configureQueueStream.streamParameters();

    // Set the SubQueueIdInfo if non-default subQueueId
    if (!queue->hasDefaultSubQueueId()) {
        bmqp_ctrlmsg::SubQueueIdInfo& sqidInfo =
            streamParams.subIdInfo().makeValueInplace();
        sqidInfo.appId() = queue->uri().id();
        sqidInfo.subId() = queue->subQueueId();
    }

    streamParams.maxUnconfirmedMessages() = options.maxUnconfirmedMessages();
    streamParams.maxUnconfirmedBytes()    = options.maxUnconfirmedBytes();
    streamParams.consumerPriority()       = options.consumerPriority();

    // Set consumerPriority and consumerPriorityCount
    if (isDeconfigure) {
        streamParams.consumerPriority() =
            bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID;
        streamParams.consumerPriorityCount() = 0;
    }
    else {
        streamParams.consumerPriority()      = options.consumerPriority();
        streamParams.consumerPriorityCount() = 1;

        queue->registerInternalSubscriptionId(queue->subQueueId(),
                                              queue->subQueueId(),
                                              bmqt::CorrelationId());
    }

    return context;
}

BrokerSession::RequestManagerType::RequestSp
BrokerSession::createStandaloneConfigureQueueContext(
    const bsl::shared_ptr<Queue>& queue,
    const bmqt::QueueOptions&     options,
    const FsmCallback&            fsmCallback)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Prepare the request context
    RequestManagerType::RequestSp context = createConfigureQueueContext(
        queue,
        options,
        false,  // isDeconfigure
        true);  // isBuffered

    // The result of this operation needs to be processed after all
    // pending/enqueued events in the event buffer.  The request may be
    // buffered and retransmitted.
    (*context).setAsyncNotifierCb(fsmCallback);

    return context;
}

BrokerSession::RequestManagerType::RequestSp
BrokerSession::createCloseQueueContext(const FsmCallback& fsmCallback)
{
    RequestManagerType::RequestSp closeContext =
        d_requestManager.createRequest();
    closeContext->setGroupId(k_NON_BUFFERED_REQUEST_GROUP_ID);
    closeContext->request().choice().makeCloseQueue();

    // The result of this operation needs to be processed after all
    // pending/enqueued events in the event buffer.
    closeContext->setAsyncNotifierCb(fsmCallback);

    return closeContext;
}

bool BrokerSession::isHostHealthy() const
{
    return (d_hostHealthState == bmqt::HostHealthState::e_HEALTHY);
}

void BrokerSession::onHostHealthStateChange(bmqt::HostHealthState::Enum state)
{
    // executed by thread determined by *HostHealthMonitor*

    bsl::shared_ptr<Event> event = createEvent();
    event->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doHandleHostHealthStateChange,
                             this,
                             state,
                             bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(event);
}

void BrokerSession::doHandleHostHealthStateChange(
    bmqt::HostHealthState::Enum  state,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<Event>& eventSp)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    bool wasHostHealthy = isHostHealthy();
    d_hostHealthState   = state;
    if (isHostHealthy() == wasHostHealthy) {
        return;
    }

    if (isHostHealthy()) {
        d_sessionFsm.handleHostHealthy();
    }
    else {
        d_sessionFsm.handleHostUnhealthy();
    }
}

void BrokerSession::actionSuspendHealthSensitiveQueues()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Fetch list of all queues.
    const int allocSize = 32 * sizeof(bsl::shared_ptr<Queue>);
    bdlma::LocalSequentialAllocator<allocSize> localAllocator(d_allocator_p);
    bsl::vector<bsl::shared_ptr<Queue> >       allQueues(&localAllocator);
    d_queueManager.getAllQueues(&allQueues);

    // If there are already outstanding suspend/resume requests, then we have
    // moved from (Unhealthy -> Healthy -> Unhealthy) host-health state without
    // having finished resuming all queues in the time between the two
    // unhealthy states. Our session in this case never entered a completely
    // "healthy" state, so we never issued a corresponding "health restored"
    // event. We therefore also elide this intermediate "host unhealthy" event.
    if (d_numPendingHostHealthRequests == 0) {
        enqueueSessionEvent(bmqt::SessionEventType::e_HOST_UNHEALTHY);
    }

    // Attempt to suspend each queue.
    for (bsl::vector<bsl::shared_ptr<Queue> >::size_type idx = 0;
         idx != allQueues.size();
         ++idx) {
        const bmqt::QueueOptions& options = allQueues[idx]->options();
        if (options.suspendsOnBadHostHealth()) {
            d_queueFsm.handleQueueSuspend(allQueues[idx]);
        }
    }
}

void BrokerSession::actionResumeHealthSensitiveQueues()
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Fetch list of all queues.
    const int allocSize = 32 * sizeof(bsl::shared_ptr<Queue>);
    bdlma::LocalSequentialAllocator<allocSize> localAllocator(d_allocator_p);
    bsl::vector<bsl::shared_ptr<Queue> >       allQueues(&localAllocator);
    d_queueManager.getAllQueues(&allQueues);

    // An increment on the number of pending requests acts as a "guard" to
    // ensure that no calls to 'handleQueueResume' inadvertently issue a
    // redundant 'HOST_HEALTH_RESUMED' event (which could otherwise occur in
    // certain cases, e.g., write-only queues, request failure, etc).
    d_numPendingHostHealthRequests++;

    // Attempt to resume each queue.
    for (bsl::vector<bsl::shared_ptr<Queue> >::size_type idx = 0;
         idx != allQueues.size();
         ++idx) {
        const bmqt::QueueOptions& options = allQueues[idx]->options();
        if (options.suspendsOnBadHostHealth()) {
            d_queueFsm.handleQueueResume(allQueues[idx]);
        }
    }

    // Decrement our counter "guard".
    d_numPendingHostHealthRequests--;

    // If we aren't waiting for any queues to resume, then just immediately
    // publish the session event ourselves.
    if (d_numPendingHostHealthRequests == 0) {
        d_sessionFsm.handleAllQueuesResumed();
    }
}

bmqt::GenericResult::Enum
BrokerSession::requestWriterCb(const RequestManagerType::RequestSp& context,
                               const bmqp::QueueId&                 queueId,
                               const bsl::shared_ptr<bdlbb::Blob>&  blob_sp,
                               bsls::Types::Int64                   watermark)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    const bool isBuffered = context->groupId() !=
                            k_NON_BUFFERED_REQUEST_GROUP_ID;

    if (isBuffered) {
        const bmqt::MessageGUID guid =
            d_messageCorrelationIdContainer.add(context, queueId, *blob_sp);
        char guidHex[bmqt::MessageGUID::e_SIZE_HEX];
        guid.toHex(guidHex);
        context->adoptUserData(
            bdld::Datum::copyString(guidHex,
                                    bmqt::MessageGUID::e_SIZE_HEX,
                                    d_allocator_p));
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_channel_sp)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BSLS_ASSERT_SAFE(isBuffered);

        // The request is stored for further retransmission once the session is
        // reconnected.
        return bmqt::GenericResult::e_SUCCESS;  // RETURN
    }

    bmqt::GenericResult::Enum res = writeOrBuffer(*blob_sp, watermark);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            res != bmqt::GenericResult::e_SUCCESS && isBuffered)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // The channel is bad, but the request is already stored for further
        // retransmission once the session is reconnected.
        res = bmqt::GenericResult::e_SUCCESS;
    }

    return res;
}

bmqt::GenericResult::Enum
BrokerSession::writeOrBuffer(const bdlbb::Blob& eventBlob,
                             bsls::Types::Int64 highWaterMark)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(d_channel_sp);

    bmqio::Status             status(d_allocator_p);
    bmqt::GenericResult::Enum res = bmqt::GenericResult::e_SUCCESS;

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !d_extensionBlobBuffer.empty())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BSLS_ASSERT_SAFE(!d_extensionBufferEmpty);

        // Buffer the blob and return success
        d_extensionBlobBuffer.push_back(eventBlob);
        return res;  // RETURN
    }

    d_channel_sp->write(&status,
                        eventBlob,
                        highWaterMark - k_CONTROL_DATA_WATERMARK_EXTRA);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!status)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        if (status.category() == bmqio::StatusCategory::e_LIMIT) {
            d_extensionBlobBuffer.push_back(eventBlob);
            d_extensionBufferEmpty = false;
        }
        else {
            res = bmqt::GenericResult::e_NOT_CONNECTED;
            // Critical error. Close the channel.
            BALL_LOG_ERROR << "Unrecoverable channel error: " << status;
            d_channel_sp->close();
        }
    }

    return res;
}

BrokerSession::BrokerSession(
    bdlmt::EventScheduler*                  scheduler,
    bdlbb::BlobBufferFactory*               bufferFactory,
    BlobSpPool*                             blobSpPool_p,
    const bmqt::SessionOptions&             sessionOptions,
    const EventQueue::EventHandlerCallback& eventHandlerCb,
    const StateFunctor&                     stateCb,
    bslma::Allocator*                       allocator)
: d_allocators(allocator)
, d_eventPool(bdlf::BindUtil::bind(&poolCreateEvent,
                                   bdlf::PlaceHolders::_1,  // address
                                   bufferFactory,
                                   bdlf::PlaceHolders::_2),  // allocator
              -1,
              d_allocators.get("EventPool"))
, d_sessionOptions(sessionOptions)
, d_scheduler_p(scheduler)
, d_bufferFactory_p(bufferFactory)
, d_blobSpPool_p(blobSpPool_p)
, d_channel_sp()
, d_extensionBlobBuffer(allocator)
, d_acceptRequests(false)
, d_extensionBufferEmpty(true)
, d_extensionBufferCondition(bsls::SystemClockType::e_MONOTONIC)
, d_queuesStats(allocator)
, d_eventsStats(allocator)
, d_stateCb(bsl::allocator_arg, allocator, stateCb)
, d_usingSessionEventHandler(eventHandlerCb)  // UnspecifiedBool operator ...
, d_messageCorrelationIdContainer(
      d_allocators.get("messageCorrelationIdContainer"))
, d_fsmThread(bslmt::ThreadUtil::invalidHandle())
, d_fsmThreadChecker()
, d_fsmEventQueue(k_FSMQUEUE_INITIAL_CAPACITY,
                  d_allocators.get("FSMEventQueue"))
, d_eventQueue(&d_eventPool,
               bsl::min(k_EVENTQUEUE_INITIAL_CAPACITY,
                        sessionOptions.eventQueueHighWatermark()),
               sessionOptions.eventQueueLowWatermark(),
               sessionOptions.eventQueueHighWatermark(),
               (eventHandlerCb ? bdlf::BindUtil::bind(
                                     &BrokerSession::eventHandlerCbWrapper,
                                     this,
                                     bdlf::PlaceHolders::_1,  // EventSp
                                     eventHandlerCb)
                               : eventHandlerCb),
               // supply an empty handler callback if session is not
               // configured to use event handler
               (eventHandlerCb ? sessionOptions.numProcessingThreads() : 0),
               d_allocators.get("EventQueue"))
, d_requestManager(bmqp::EventType::e_CONTROL,
                   blobSpPool_p,
                   scheduler,
                   true,  // lateResponseMode
                   BrokerSession_Executor(this),
                   d_sessionOptions.traceContext(),
                   allocator)
, d_queueManager(allocator)
, d_numPendingReopenQueues(0)
, d_numPendingHostHealthRequests(0)
, d_throttledFailedPostMessage(5000, 1)  // 1 log per 5s interval
, d_throttledFailedAckMessages(3000, 1)  // 1 log per 3s interval
, d_messageDumper(&d_queueManager,
                  &d_messageCorrelationIdContainer,
                  bufferFactory,
                  allocator)
, d_allocator_p(allocator)
, d_sessionFsm(*this)
, d_queueFsm(*this)
, d_hostHealthState(bmqt::HostHealthState::e_HEALTHY)
, d_hostHealthSignalerConnectionGuard()
, d_startSemaphore(bsls::SystemClockType::e_MONOTONIC)
, d_inProgressEventHandlerCount(0)
, d_isStopping(false)
, d_messageExpirationTimeoutHandle()
, d_nextRequestGroupId(k_NON_BUFFERED_REQUEST_GROUP_ID)
, d_queueRetransmissionTimeoutMap(allocator)
, d_nextInternalSubscriptionId(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID)
, d_doConfigureStream(0)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_scheduler_p->clockType() ==
                     bsls::SystemClockType::e_MONOTONIC);
    BSLS_ASSERT_SAFE(!d_usingSessionEventHandler ||
                     sessionOptions.numProcessingThreads() > 0);

    // Call 'resetState()' now just so that we ensure it is resetting the
    // values to their original default values (in order to support
    // start/stop/start sequence).
    //
    // Make sure 'resetState' and the initial values set in the constructor
    // remain in sync to prevent misleading values.
    resetState();

    // Spawn the FSM thread
    bslmt::ThreadAttributes threadAttributes =
        bmqsys::ThreadUtil::defaultAttributes();
    threadAttributes.setThreadName("bmqFSMEvtQ");
    if (bslmt::ThreadUtil::createWithAllocator(
            &d_fsmThread,
            threadAttributes,
            bdlf::MemFnUtil::memFn(&BrokerSession::fsmThreadLoop, this),
            d_allocator_p) != 0) {
        BSLS_ASSERT_OPT(false && "Failed to create the FSM thread");
    }

    // Start user event queue
    if (d_eventQueue.start() != bmqt::GenericResult::e_SUCCESS) {
        BSLS_ASSERT_OPT(false && "Failed to start user event queue");
    }
}

BrokerSession::~BrokerSession()
{
    // Enqueue a poison pill event to terminate the event loop
    bsl::shared_ptr<Event> queueEvent;  // PoisonPill is just a null ptr
    enqueueFsmEvent(queueEvent);

    d_fsmEventQueue.disablePushBack();
    // This is a bit racy, as a different thread potentially could have
    // enqueued an event after the above poison pill, but this is just for
    // safety anyway.

    // Join on the FSM thread
    bslmt::ThreadUtil::join(d_fsmThread);

    // Now stop user event queue
    d_eventQueue.stop();
}

void BrokerSession::initializeStats(
    bmqst::StatContext*                       rootStatContext,
    const bmqst::StatValue::SnapshotLocation& start,
    const bmqst::StatValue::SnapshotLocation& end)
{
    d_eventQueue.initializeStats(rootStatContext, start, end);

    QueueStatsUtil::initializeStats(&d_queuesStats,
                                    rootStatContext,
                                    start,
                                    end,
                                    d_allocator_p);

    d_eventsStats.initializeStats(rootStatContext, start, end);
}

bmqt::GenericResult::Enum
BrokerSession::processPacket(const bdlbb::Blob& packet)
{
    // executed by the *IO* thread
    // or *APPLICATION* thread

    enum { e_NUM_BYTES_IN_BLOB_TO_DUMP = 256 };

    // Create a raw event with a cloned blob
    bmqp::Event event(&packet, d_allocator_p, true);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Received an invalid packet: "
                       << bmqu::BlobStartHexDumper(
                              &packet,
                              e_NUM_BYTES_IN_BLOB_TO_DUMP);
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(event.isHeartbeatReqEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        onHeartbeat();
        return bmqt::GenericResult::e_SUCCESS;  // RETURN
    }

    // Add to event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRawEvent(event);
    return enqueueFsmEvent(queueEvent);
}

void BrokerSession::setChannel(const bsl::shared_ptr<bmqio::Channel>& channel)
{
    // executed by the *IO* thread

    if (bmqsys::ThreadUtil::k_SUPPORT_THREAD_NAME) {
        bmqsys::ThreadUtil::setCurrentThreadNameOnce("bmqTCPIO");
    }

    if (channel) {  // We are now connected to bmqbrkr
        BALL_LOG_INFO << "Channel is CREATED [host: " << channel->peerUri()
                      << "]";
    }
    else {  // We lost connection with bmqbrkr
        BALL_LOG_INFO << "Channel is RESET";
    }

    // Add to the FSM event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doSetChannel,
                             this,
                             channel,
                             bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(queueEvent);
}

int BrokerSession::start(const bsls::TimeInterval& timeout)
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_startStopMutex);  // LOCKED

    // Reset the start semaphore: since we unconditionally post on it, make
    // sure we don't have left over state from a previous call to start (in
    // order to support multiple consecutive sequence of start()/stop() calls).
    while (d_startSemaphore.tryWait() == 0) {
        // nothing
    }

    int rc = startAsync();
    if (rc != 0) {
        BALL_LOG_ERROR << "Failed to start brokerSession [rc: " << rc << "]";
        return rc;  // RETURN
    }

    rc = d_startSemaphore.timedWait(bmqsys::Time::nowMonotonicClock() +
                                    timeout);
    if (rc != 0) {
        // Timeout
        BALL_LOG_ERROR << "Start (SYNC) has timed out";

        // Stop the brokerSession.  Being under the mutex lock use async stop.
        while (d_stopSemaphore.tryWait() == 0) {
            // nothing
        }

        stopAsync();
        d_stopSemaphore.wait();

        return bmqt::GenericResult::e_TIMEOUT;  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

int BrokerSession::startAsync()
{
    // Use local semaphore to wait until start request is accepted by the FSM.
    // The FSM may return an error code that startAsync will use as its return
    // value.
    bslmt::Semaphore fsmAcceptedSemaphore;
    int              startStatus;

    // Add to the FSM event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doStart,
                             this,
                             &fsmAcceptedSemaphore,
                             &startStatus,
                             bdlf::PlaceHolders::_1,  // eventImpl
                             createDTSpan("bmq.session.start")));

    if (enqueueFsmEvent(queueEvent) != bmqt::GenericResult::e_SUCCESS) {
        return bmqt::GenericResult::e_REFUSED;  // RETURN
    }

    fsmAcceptedSemaphore.wait();

    return startStatus;
}

void BrokerSession::onDisconnectResponse(
    const RequestManagerType::RequestSp& context)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->groupId() == k_NON_BUFFERED_REQUEST_GROUP_ID);
    BSLS_ASSERT_SAFE(context->userData().isNull());

    BALL_LOG_INFO << "OnDisconnectResponse";

    if (context->response().choice().isDisconnectResponseValue() == false) {
        // Error... this is a disconnect response.. nothing much to do, just
        //          log error and continue normally as if we received a success
        //          response.
        //          Usually it's a status response but in theory it may be any
        //          message.
        BALL_LOG_ERROR << "Got error disconnect response: "
                       << context->response() << "\n";

        const bool wasCanceled = context->result() ==
                                 bmqt::GenericResult::e_CANCELED;
        if (wasCanceled) {
            // The request is cancelled withing the FSM action, no need to call
            // the FSM again.
            return;  // RETURN
        }
    }

    // Disconnect is a special response, no need to signal the request
    d_sessionFsm.handleSessionClosed();
}

void BrokerSession::handleQueueFsmEvent(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue,
    bool                                 isLocalTimeout,
    bool                                 isLateResponse,
    const bsls::TimeInterval             absTimeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (isLocalTimeout) {
        BSLS_ASSERT_SAFE(!isLateResponse);

        if (isStarted()) {
            d_queueFsm.handleResponseTimeout(queue, context);
        }
        else {
            d_queueFsm.handleResponseExpired(queue, context);
        }
    }
    else if (isLateResponse) {
        d_queueFsm.handleLateResponse(queue, context);
    }
    else if (context->isError()) {
        const bmqt::GenericResult::Enum res = context->result();

        // Request with e_NOT_SUPPORTED status is FSM internal and should not
        // be handled as new FSM event.
        BSLS_ASSERT_SAFE(res != bmqt::GenericResult::e_NOT_SUPPORTED);

        if (res == bmqt::GenericResult::e_CANCELED) {
            d_queueFsm.handleRequestCanceled(queue, context);
        }
        else if (res == bmqt::GenericResult::e_NOT_CONNECTED) {
            d_queueFsm.handleSessionDown(queue, context);
        }
        else {
            d_queueFsm.handleResponseError(queue, context, absTimeout);
        }
    }
    else {
        d_queueFsm.handleResponseOk(queue, context, absTimeout);
    }
}

void BrokerSession::onOpenQueueResponse(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval             absTimeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    const bool isBuffered = !context->userData().isNull();
    if (isBuffered) {
        removePendingControlMessage(context);
    }

    if (context->isLocalTimeout()) {
        BALL_LOG_ERROR << "Timeout while opening queue: [request: "
                       << context->request().choice().openQueue() << "]";
    }
    else if (context->isError()) {
        BALL_LOG_ERROR << "Error while opening queue: [status: "
                       << context->response().choice().status()
                       << ", request: " << context->request() << "]";
    }
    else if (!context->isLateResponse()) {
        // Temporary; shall remove after 2nd roll out of "new style" brokers.
        BSLS_ASSERT_SAFE(d_channel_sp);  // just got the response
        int isMPsEx;

        if (d_channel_sp->properties().load(
                &isMPsEx,
                NegotiatedChannelFactory::k_CHANNEL_PROPERTY_MPS_EX)) {
            BSLS_ASSERT_SAFE(isMPsEx);
            queue->setOldStyle(false);
        }
    }

    handleQueueFsmEvent(context,
                        queue,
                        context->isLocalTimeout(),
                        context->isLateResponse(),
                        absTimeout);
}

void BrokerSession::onCloseQueueResponse(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(context->groupId() == k_NON_BUFFERED_REQUEST_GROUP_ID);
    BSLS_ASSERT_SAFE(context->userData().isNull());

    if (context->isLocalTimeout()) {
        BALL_LOG_ERROR << "Timeout while closing queue: [request: "
                       << context->request().choice().closeQueue() << "]";
    }
    else if (context->isError()) {
        BALL_LOG_ERROR << "Error while closing queue: [status: "
                       << context->response().choice().status()
                       << ", request: "
                       << context->request().choice().closeQueue() << "]";
    }

    handleQueueFsmEvent(context,
                        queue,
                        context->isLocalTimeout(),
                        context->isLateResponse(),
                        bsls::TimeInterval(0));
}

bmqt::ConfigureQueueResult::Enum BrokerSession::sendOpenConfigureQueue(
    const RequestManagerType::RequestSp& openQueueContext,
    const RequestManagerType::RequestSp& configQueueContext,
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval             absTimeout,
    bool                                 isReopenRequest)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // We expect the response to be 'OpenQueueResponse'.
    BSLS_ASSERT_SAFE(
        openQueueContext->response().choice().isOpenQueueResponseValue());

    // Do the 2nd step in openQueue flow: configure queue
    const ConfiguredCallback configuredCb = bdlf::BindUtil::bind(
        &BrokerSession::onOpenQueueConfigured,
        this,
        bdlf::PlaceHolders::_1,  // configureQueue context
        bdlf::PlaceHolders::_2,  // queue
        openQueueContext,        // openQueue context
        isReopenRequest);
    const bsls::TimeInterval timeout = absTimeout -
                                       bmqsys::Time::nowMonotonicClock();

    // For reopen request do not reset pendingConfigureId which could have been
    // set by buffered configure request.
    return configureQueueImp(configQueueContext,
                             queue,
                             queue->options(),
                             timeout,
                             configuredCb,
                             !isReopenRequest);
    // checkConcurrent false for reopen request.  Do
    // not check pending configure id when
    // reopening.
}

bmqt::GenericResult::Enum BrokerSession::sendCloseQueue(
    const RequestManagerType::RequestSp& closeQueueContext,
    const bsl::shared_ptr<Queue>&        queue,
    const bsls::TimeInterval             absTimeout,
    bool                                 isFinal)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // Prepare the close queue request
    bmqp_ctrlmsg::CloseQueue& closeQueue =
        closeQueueContext->request().choice().makeCloseQueue();
    closeQueue.handleParameters() = queue->handleParameters();
    closeQueue.isFinal()          = isFinal;
    // Set the 'isFinal' flag only before closing the last subStream for
    // this canonical uri.

    RequestManagerType::RequestType::ResponseCb response =
        bdlf::BindUtil::bind(&BrokerSession::onCloseQueueResponse,
                             this,
                             bdlf::PlaceHolders::_1,  // context
                             queue);
    closeQueueContext->setResponseCb(response);

    const bsls::TimeInterval timeout = absTimeout -
                                       bmqsys::Time::nowMonotonicClock();
    return sendRequest(closeQueueContext,
                       bmqp::QueueId(queue->id(), queue->subQueueId()),
                       timeout);
}

void BrokerSession::onOpenQueueConfigured(
    const RequestManagerType::RequestSp& configureQueueContext,
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& openQueueContext,
    bool                                 isReopenRequest)
{
    // executed by the FSM thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (!configureQueueContext->userData().isNull()) {
        removePendingControlMessage(configureQueueContext);
    }

    const int pendingId = queue->pendingConfigureId();
    const int currentId = configureQueueContext->request().rId().value();

    if (pendingId == currentId || pendingId == Queue::k_INVALID_CONFIGURE_ID) {
        // There was only one pending configure request. Now it has a response,
        // so reset the pending id value.
        queue->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);
    }
    else {
        // The pending id differs from the incoming request id.  This is
        // possible only if there is a buffered configure request and the
        // current request is reopen-configure.  In this case keep the
        // 'pendingConfigureId' value until the pending request is
        // retransmitted and its response is handled.
        BSLS_ASSERT_SAFE(isReopenRequest);
    }

    if (configureQueueContext->isError()) {
        // NOTE: We explicitly set here the response of the openQueue request
        //       to that of the configureQueue request because it is possible
        //       that the first request (openQueue) succeeded while the second
        //       (configureQueue) failed, and the sync. 'openQueue' relies
        //       on the result of the first request object ('openQueueContext')
        openQueueContext->response() = configureQueueContext->response();
    }

    if (configureQueueContext->isLocalTimeout()) {
        // The configureQueue timed out locally in the SDK.  When we get back a
        // response to a timed out request from upstream we will roll back the
        // operation.  If a response to a locally canceled request eventually
        // arrives (it may or may not), it will be treated as a response to a
        // request that timed out locally.

        BALL_LOG_ERROR << "Timed out locally while opening queue: "
                       << "[queue: " << (*queue)
                       << ", isReopenRequest: " << isReopenRequest << "]";
    }
    else if (configureQueueContext->isLateResponse()) {
        // The initial open request has timed out and now late configure
        // response comes
        BALL_LOG_INFO << "Late open-configure response: [queue: " << (*queue)
                      << ", isReopenRequest: " << isReopenRequest << "]";
    }
    else if (configureQueueContext->isError()) {
        BALL_LOG_ERROR << "Error opening queue: [queue: " << (*queue)
                       << ", isReopenRequest: " << isReopenRequest << "]";
    }

    handleQueueFsmEvent(openQueueContext,
                        queue,
                        configureQueueContext->isLocalTimeout(),
                        configureQueueContext->isLateResponse(),
                        bsls::TimeInterval(0));
}

void BrokerSession::onConfigureQueueResponse(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue,
    const bmqt::QueueOptions&            previousOptions,
    const ConfiguredCallback&            configuredCb)
{
    // executed by the FSM thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (context->isError()) {
        // Ignore cancelled request that has already timed out
        if (context->isLateResponse()) {
            const bmqt::GenericResult::Enum res = context->result();

            BSLS_ASSERT_SAFE(res == bmqt::GenericResult::e_CANCELED ||
                             res == bmqt::GenericResult::e_NOT_CONNECTED ||
                             res == bmqt::GenericResult::e_NOT_SUPPORTED);

            (void)res;
            BALL_LOG_INFO << "Ignore cancelled request: "
                          << context->request();
            return;  // RETURN
        }
        // Response indicates failure
        BALL_LOG_ERROR << "Error while configuring queue: [status: "
                       << context->response().choice().status()
                       << ", request: " << context->request() << "]";

        // Revert to previous QueueOptions. Note that if the client's options
        // weren't updated by the request in the first place, then this is a
        // no-op.
        queue->setOptions(previousOptions);
    }
    else {
        // Successfully configured the stream.  Since there are no concurrent
        // configureQueue, can simply do nothing.

        // Validation

        bmqp_ctrlmsg::ConfigureStream respAdaptor;

        // TODO: temporarily
        bmqp_ctrlmsg::ConfigureStream& resp = respAdaptor;

        if (context->request().choice().isConfigureQueueStreamValue()) {
            BSLS_ASSERT_SAFE(context->response()
                                 .choice()
                                 .isConfigureQueueStreamResponseValue());
            bmqp::ProtocolUtil::convert(&respAdaptor,
                                        context->response()
                                            .choice()
                                            .configureQueueStreamResponse()
                                            .request());
        }
        else {
            BSLS_ASSERT_SAFE(
                context->response().choice().isConfigureStreamResponseValue());

            resp = context->response()
                       .choice()
                       .configureStreamResponse()
                       .request();
        }

        const bmqp_ctrlmsg::StreamParameters& in = resp.streamParameters();
#ifdef BSLS_ASSERT_SAFE_IS_ACTIVE
        bmqp_ctrlmsg::ConfigureStream  reqAdaptor;
        bmqp_ctrlmsg::ConfigureStream& req = reqAdaptor;
        if (context->request().choice().isConfigureQueueStreamValue()) {
            bmqp::ProtocolUtil::convert(
                &reqAdaptor,
                context->request().choice().configureQueueStream());
        }
        else {
            BSLS_ASSERT_SAFE(
                context->request().choice().isConfigureStreamValue());

            req = context->request().choice().configureStream();
        }
        const bmqp_ctrlmsg::StreamParameters& out = req.streamParameters();
        // Verify correct queue
        BSLS_ASSERT_SAFE(static_cast<unsigned int>(queue->id()) == resp.qId());

        // Verify that the requested parameters match those that we received
        // in the response (i.e. we got what we asked for)
        // NOTE: This may change in the future
        BSLS_ASSERT_SAFE(out.subscriptions().size() ==
                         in.subscriptions().size());

        typedef bsl::unordered_map<bsls::Types::Uint64,
                                   bmqp_ctrlmsg::Subscription>
            Subscriptions;

        Subscriptions subscriptions;

        for (size_t i = 0; i < out.subscriptions().size(); ++i) {
            BSLS_ASSERT_SAFE(subscriptions
                                 .emplace(out.subscriptions()[i].sId(),
                                          out.subscriptions()[i])
                                 .second);
        }
        for (size_t i = 0; i < in.subscriptions().size(); ++i) {
            Subscriptions::const_iterator cit = subscriptions.find(
                in.subscriptions()[i].sId());
            BSLS_ASSERT_SAFE(cit != subscriptions.end());
            BSLS_ASSERT_SAFE(cit->second == in.subscriptions()[i]);
        }
#endif  // BSLS_ASSERT_SAFE_IS_ACTIVE

        // Update QueueManager, so it can look up by new Subscription Ids
        // Remove 'previousOptions' and insert new ones.
        d_queueManager.updateSubscriptions(queue, in);
        queue->setConfig(in);
    }

    configuredCb(context, queue);

    // If the host-health changed while this queue was being configured, then
    // start a new configuration-request to suspend or resume the queue.
    if (queue->isSuspendedWithBroker()) {
        if (!queue->options().suspendsOnBadHostHealth() || isHostHealthy()) {
            d_queueFsm.handleQueueResume(queue);
        }
    }
    else if (queue->options().suspendsOnBadHostHealth() && !isHostHealthy()) {
        d_queueFsm.handleQueueSuspend(queue);
    }
}

void BrokerSession::onConfigureQueueConfigured(
    const RequestManagerType::RequestSp& context,
    const bsl::shared_ptr<Queue>&        queue)
{
    // executed by the FSM thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    if (!context->userData().isNull()) {
        removePendingControlMessage(context);
    }

    // Reset the pending configure request id for any standalone configure
    // response except local timeout (in this case late configure response may
    // come from the broker, thus keep the pending request id).  If local
    // timeout happens when the session has no connection with the broker that
    // means a pending configure request has timed out and the pending id
    // should also be reset.
    if (!context->isLocalTimeout() || !isStarted()) {
        queue->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);
    }

    if (context->isError() &&
        context->response().choice().status().category() ==
            bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED) {
        // This is a locally canceled standalone configure request.  It was
        // canceled within the FSM action, so here we won't call the FSM
        // again, instead we just notify about the operation result in case
        // if the response is not the late response.  For the late respose
        // the notification had already been done when related request
        // timed out.
        if (!context->isLateResponse()) {
            context->signal();
        }
        return;  // RETURN
    }

    handleQueueFsmEvent(context,
                        queue,
                        context->isLocalTimeout(),
                        context->isLateResponse(),
                        bsls::TimeInterval(0));
}

void BrokerSession::onCloseQueueConfigured(
    const RequestManagerType::RequestSp& configureQueueContext,
    const bsl::shared_ptr<Queue>&        queue,
    const RequestManagerType::RequestSp& closeQueueContext,
    const bsls::TimeInterval             absTimeout,
    bool                                 isFinal)
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(configureQueueContext->groupId() ==
                     k_NON_BUFFERED_REQUEST_GROUP_ID);
    BSLS_ASSERT_SAFE(configureQueueContext->userData().isNull());
    BSLS_ASSERT_SAFE(closeQueueContext->groupId() ==
                     k_NON_BUFFERED_REQUEST_GROUP_ID);
    BSLS_ASSERT_SAFE(closeQueueContext->userData().isNull());

    // Unconditionally reset the pending configure request id.  It may be not
    // empty if the queue has locally expired standalone configure request.
    queue->setPendingConfigureId(Queue::k_INVALID_CONFIGURE_ID);

    RequestManagerType::RequestSp context = closeQueueContext;

    if (configureQueueContext->isLateResponse()) {
        context = configureQueueContext;
    }
    else if (configureQueueContext->isError()) {
        // Response indicates failure
        bmqt::GenericResult::Enum result = configureQueueContext->result();

        BALL_LOG_ERROR << "Error configuring while closing queue: [queue: "
                       << (*queue) << ", result: " << result << "]";

        // NOTE: Even if the configure failed, we still want to send the close
        //       queue request because we want to indicate to upstream that we
        //       are closing the queue regardless of the result of the
        //       configure.
        // NOTE: An underlying assumption is that if the failure was due to
        //       timeout, then the configureQueue with null parameters that we
        //       sent eventually succeeds, so sending a closeQueue as below
        //       is appropriate.  And if the failure was not due to timeout,
        //       then we expect the close queue below to fail for the same
        //       reasons (e.g. handle not found, etc.).

        if (result == bmqt::GenericResult::e_CANCELED ||
            result == bmqt::GenericResult::e_NOT_CONNECTED) {
            // Request was canceled (connection lost with upstream).  We do not
            // proceed to send a closeQueue request upstream (there is no
            // channel!).  Instead, we inject an unsuccessful closeQueue
            // response and close the queue.

            // Prepare the close queue request
            bmqp_ctrlmsg::CloseQueue& closeQueue =
                closeQueueContext->request().choice().makeCloseQueue();
            closeQueue.handleParameters() = queue->handleParameters();
            closeQueue.isFinal()          = isFinal;

            // Set user response
            d_queueFsm.injectErrorResponse(
                closeQueueContext,
                static_cast<bmqp_ctrlmsg::StatusCategory::Value>(result),
                "The request was canceled [reason: connection was lost]");
        }
        else if (configureQueueContext->isLocalTimeout()) {
            // Set close context response to that from the configure context
            // which carries the error status
            closeQueueContext->response() = configureQueueContext->response();
        }
    }

    handleQueueFsmEvent(context,
                        queue,
                        configureQueueContext->isLocalTimeout(),
                        configureQueueContext->isLateResponse(),
                        absTimeout);
}

void BrokerSession::reopenQueues()
{
    // executed by the FSM thread
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());

    // First, get the number of queues that need to be reopened.
    // 'd_numPendingReopenQueues' variable needs to be updated before we start
    // sending reopen request to the broker to prevent a potential race
    // condition that would issue a 'STATE_RESTORED' event before all queues
    // that needed to be re-opened have been re-opened.
    // NOTE: Worth to use a local allocator here, and 32 is an 'arbitrarily'
    //       chosen number.
    const int allocSize = 32 * sizeof(bsl::shared_ptr<Queue>);

    bdlma::LocalSequentialAllocator<allocSize> localAllocator(d_allocator_p);
    bsl::vector<bsl::shared_ptr<Queue> >       pendingQueues(&localAllocator);
    d_queueManager.lookupQueuesByState(&pendingQueues, QueueState::e_PENDING);

    d_numPendingReopenQueues = pendingQueues.size();
    if (d_numPendingReopenQueues == 0) {
        // Fast path
        BALL_LOG_INFO << "No queues need to be reopened.";

        BSLS_ASSERT_SAFE(
            (d_messageCorrelationIdContainer.numberOfPuts() == 0) &&
            "There should be no pending PUTs without queues to be reopened");

        if (d_messageCorrelationIdContainer.numberOfControls() > 0) {
            retransmitPendingMessages();
        }

        enqueueSessionEvent(bmqt::SessionEventType::e_STATE_RESTORED);
        return;  // RETURN
    }

    BALL_LOG_INFO << "Number of queues that need to be reopened: "
                  << d_numPendingReopenQueues;

    // Reset the subStreamCount before initiating reopen logic
    for (bsl::vector<bsl::shared_ptr<Queue> >::size_type idx = 0;
         idx != pendingQueues.size();
         ++idx) {
        const bsl::shared_ptr<Queue>& queueSp = pendingQueues[idx];
        d_queueManager.resetSubStreamCount(queueSp->uri().canonical());
    }

    for (bsl::vector<bsl::shared_ptr<Queue> >::size_type idx = 0;
         idx != pendingQueues.size();
         ++idx) {
        const FsmCallback fsmCallback = bdlf::BindUtil::bind(
            &BrokerSession::asyncRequestNotifier,
            this,
            bdlf::PlaceHolders::_1,  // request
            bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
            pendingQueues[idx]->correlationId(),
            pendingQueues[idx],
            EventCallback());

        RequestManagerType::RequestSp context = createOpenQueueContext(
            pendingQueues[idx],
            fsmCallback,
            false);  // isBuffered
        d_queueFsm.handleReopenRequest(pendingQueues[idx],
                                       d_sessionOptions.openQueueTimeout(),
                                       context);
    }
}

void BrokerSession::notifyQueuesChannelDown()
{
    // NOTE: Worth to use a local allocator here, and 32 is an 'arbitrarily'
    //       chosen number.
    const int allocSize = 32 * sizeof(bsl::shared_ptr<Queue>);

    bdlma::LocalSequentialAllocator<allocSize> localAllocator(d_allocator_p);
    bsl::vector<bsl::shared_ptr<Queue> >       allQueues(&localAllocator);
    d_queueManager.getAllQueues(&allQueues);

    for (bsl::vector<bsl::shared_ptr<Queue> >::size_type idx = 0;
         idx != allQueues.size();
         ++idx) {
        d_queueFsm.handleChannelDown(allQueues[idx]);
    }
}

void BrokerSession::stop()
{
    bslmt::LockGuard<bslmt::Mutex> lock(&d_startStopMutex);  // LOCKED

    // Reset the stop semaphore: since we unconditionally post on it, make
    // sure we don't have left over state from a previous call to stop (in
    // order to support multiple consecutive sequence of start()/stop() calls).
    while (d_stopSemaphore.tryWait() == 0) {
        // nothing
    }

    stopAsync();

    d_stopSemaphore.wait();
}

void BrokerSession::stopAsync()
{
    bsl::shared_ptr<bmqpi::DTSpan> span;
    if (d_acceptRequests) {
        // If we aren't accepting requests, then we've either already stopped
        // the session with the broker or never had one in the first place.
        // Either way, this prevents "double stop-spans" that can occur if the
        // 'bmqimp::Application' d'tor calls 'BrokerSession::stop()' after the
        // client has already explicitly shut down the session.
        span = createDTSpan("bmq.session.stop");
    }
    d_acceptRequests = false;

    // Add to event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doStop,
                             this,
                             bdlf::PlaceHolders::_1,  // eventImpl
                             span));
    enqueueFsmEvent(queueEvent);
}

bsl::shared_ptr<Event>
BrokerSession::nextEvent(const bsls::TimeInterval& timeout)
{
    // executed by one of the *APPLICATION* threads

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_usingSessionEventHandler &&
                     "nextEvent() should be used without EventHandler");

    const bsls::TimeInterval beginTime = bmqsys::Time::nowMonotonicClock();
    bsl::shared_ptr<Event>   event     = d_eventQueue.timedPopFront(timeout);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            event->type() == Event::EventType::e_SESSION &&
            event->sessionEventType() ==
                bmqt::SessionEventType::e_DISCONNECTED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // By contract with the user, the 'DISCONNECTED' event is the event
        // that their event loop should use to exit.  We have no control over
        // how many threads the user have which are calling 'nextEvent',
        // therefore we automatically immediately re-enqueue a DISCONNECTED
        // event once we popped one out.
        bsl::shared_ptr<Event> disconnectEvent = createEvent();
        disconnectEvent->configureAsSessionEvent(
            bmqt::SessionEventType::e_DISCONNECTED,
            0,
            bmqt::CorrelationId(),
            "");
        // Dispatch event to the user event queue
        d_eventQueue.pushBack(disconnectEvent);
    }

    if (event->eventCallback()) {
        // This is a serialized SESSION event with a user-specified callback.
        // Such events are invoked inplace for serialization, and after that we
        // will try to pop the next event for the user if timeout has not
        // expired.
        event->eventCallback()(event);

        // Continue waiting for next event for the duration of the remaining
        // timeout (if any)
        const bsls::TimeInterval now = bmqsys::Time::nowMonotonicClock();
        const bsls::TimeInterval remainingTimeout = timeout -
                                                    (now - beginTime);
        if (remainingTimeout > bsls::TimeInterval(0, 0)) {
            return nextEvent(remainingTimeout);  // RETURN
        }

        // We timed out.. create and return a timeout event
        bsl::shared_ptr<Event> timeoutEvent = createEvent();
        timeoutEvent->configureAsSessionEvent(
            bmqt::SessionEventType::e_TIMEOUT,
            -1,  // rc
            bmqt::CorrelationId(),
            "No events to pop from queue during"
            "the specified timeInterval");
        return timeoutEvent;  // RETURN
    }

    return event;
}

int BrokerSession::openQueue(const bsl::shared_ptr<Queue>& queue,
                             bsls::TimeInterval            timeout)
{
    bslmt::Semaphore syncOperationSemaphore;
    int              rc = bmqt::GenericResult::e_NOT_READY;

    const bmqimp::BrokerSession::FsmCallback fsmCallback =
        bdlf::BindUtil::bind(&BrokerSession::syncRequestNotifier,
                             this,
                             &syncOperationSemaphore,
                             &rc,
                             bdlf::PlaceHolders::_1);  // request

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> openSpan(
        createDTSpan("bmq.queue.open", baggage));

    toFsm(fsmCallback,
          bdlf::BindUtil::bind(&BrokerSession::doOpenQueue,
                               this,
                               queue,
                               timeout,
                               fsmCallback,
                               bdlf::PlaceHolders::_1,  // event
                               openSpan),
          true);  // always call 'syncRequestNotifier'

    // Wait result
    syncOperationSemaphore.wait();

    return rc;
}

int BrokerSession::openQueueAsync(const bsl::shared_ptr<Queue>& queue,
                                  bsls::TimeInterval            timeout,
                                  const EventCallback&          eventCallback)
{
    const FsmCallback fsmCallback = bdlf::BindUtil::bind(
        &BrokerSession::asyncRequestNotifier,
        this,
        bdlf::PlaceHolders::_1,  // request
        bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
        queue->correlationId(),
        queue,
        eventCallback);

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> openSpan(
        createDTSpan("bmq.queue.open", baggage));

    int rc = toFsm(fsmCallback,
                   bdlf::BindUtil::bind(&BrokerSession::doOpenQueue,
                                        this,
                                        queue,
                                        timeout,
                                        fsmCallback,
                                        bdlf::PlaceHolders::_1,  // event
                                        openSpan),
                   eventCallback !=
                       0);  // if there is no callback, do not call
                            // 'asyncRequestNotifier' on failure.
    return rc;
}

void BrokerSession::openQueueSync(const bsl::shared_ptr<Queue>& queue,
                                  bsls::TimeInterval            timeout,
                                  const EventCallback&          eventCallback)
{
    bslmt::Semaphore    syncOperationSemaphore;
    const EventCallback callbackAdapter = bdlf::BindUtil::bind(
        &eventCallbackAdapter,
        &syncOperationSemaphore,
        eventCallback,
        bdlf::PlaceHolders::_1);  // event
    // The adapter executes given 'eventCallback' and posts the semaphore.

    if (d_usingSessionEventHandler) {
        openQueueAsync(queue, timeout, callbackAdapter);  // event
    }
    else {
        const FsmCallback fsmCallback = bdlf::BindUtil::bind(
            &BrokerSession::manualSyncRequestNotifier,
            this,
            bdlf::PlaceHolders::_1,  // request
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            queue->correlationId(),
            queue,
            callbackAdapter);  // adapter

        bmqpi::DTSpan::Baggage baggage(d_allocator_p);
        fillDTSpanQueueBaggage(&baggage, *queue);
        bsl::shared_ptr<bmqpi::DTSpan> openSpan(
            createDTSpan("bmq.queue.open", baggage));

        toFsm(fsmCallback,
              bdlf::BindUtil::bind(&BrokerSession::doOpenQueue,
                                   this,
                                   queue,
                                   timeout,
                                   fsmCallback,
                                   bdlf::PlaceHolders::_1,  // event
                                   openSpan),
              true);  // always call 'manualSyncRequestNotifier'
    }

    // BS does enqueue/call the `eventCallback` even if the call fails.
    // Wait result
    syncOperationSemaphore.wait();

    // The result is communicated to the caller in the `eventCallback`.
}

int BrokerSession::configureQueue(const bsl::shared_ptr<Queue>& queue,
                                  const bmqt::QueueOptions&     options,
                                  bsls::TimeInterval            timeout)
{
    bslmt::Semaphore syncOperationSemaphore;
    int              rc = bmqt::GenericResult::e_NOT_READY;

    const bmqimp::BrokerSession::FsmCallback fsmCallback =
        bdlf::BindUtil::bind(&BrokerSession::syncRequestNotifier,
                             this,
                             &syncOperationSemaphore,
                             &rc,
                             bdlf::PlaceHolders::_1);  // request

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> configureSpan(
        createDTSpan("bmq.queue.configure", baggage));

    toFsm(fsmCallback,
          bdlf::BindUtil::bind(&BrokerSession::doConfigureQueue,
                               this,
                               queue,
                               options,
                               timeout,
                               fsmCallback,
                               bdlf::PlaceHolders::_1,  // event
                               configureSpan),
          true);  // always call 'syncRequestNotifier'

    // Wait result
    syncOperationSemaphore.wait();

    return rc;
}

void BrokerSession::configureQueueSync(const bsl::shared_ptr<Queue>& queue,
                                       const bmqt::QueueOptions&     options,
                                       bsls::TimeInterval            timeout,
                                       const EventCallback& eventCallback)
{
    bslmt::Semaphore syncOperationSemaphore;

    // The adapter executes given `eventCallback` and posts the semaphore.
    const EventCallback callbackAdapter = bdlf::BindUtil::bind(
        &eventCallbackAdapter,
        &syncOperationSemaphore,
        eventCallback,
        bdlf::PlaceHolders::_1);  // event

    if (d_usingSessionEventHandler) {
        configureQueueAsync(queue,
                            options,
                            timeout,
                            callbackAdapter);  // adapter
    }
    else {
        const bmqimp::BrokerSession::FsmCallback fsmCallback =
            bdlf::BindUtil::bind(
                &BrokerSession::manualSyncRequestNotifier,
                this,
                bdlf::PlaceHolders::_1,  // request
                bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
                queue->correlationId(),
                queue,
                callbackAdapter);  // adapter

        bmqpi::DTSpan::Baggage baggage(d_allocator_p);
        fillDTSpanQueueBaggage(&baggage, *queue);
        bsl::shared_ptr<bmqpi::DTSpan> configureSpan(
            createDTSpan("bmq.queue.configure", baggage));

        toFsm(fsmCallback,
              bdlf::BindUtil::bind(&BrokerSession::doConfigureQueue,
                                   this,
                                   queue,
                                   options,
                                   timeout,
                                   fsmCallback,
                                   bdlf::PlaceHolders::_1,  // event
                                   configureSpan),
              true);  // always call 'manualSyncRequestNotifier'
    }
    // BS does enqueue/call the `eventCallback` even if the call fails.
    // Wait result
    syncOperationSemaphore.wait();

    // The result is communicated to the caller in the `eventCallback`.
}

int BrokerSession::configureQueueAsync(const bsl::shared_ptr<Queue>& queue,
                                       const bmqt::QueueOptions&     options,
                                       bsls::TimeInterval            timeout,
                                       const EventCallback& eventCallback)
{
    const bmqimp::BrokerSession::FsmCallback fsmCallback =
        bdlf::BindUtil::bind(&BrokerSession::asyncRequestNotifier,
                             this,
                             bdlf::PlaceHolders::_1,  // request
                             bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
                             queue->correlationId(),
                             queue,
                             eventCallback);

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> configureSpan(
        createDTSpan("bmq.queue.configure", baggage));

    int rc = toFsm(fsmCallback,
                   bdlf::BindUtil::bind(&BrokerSession::doConfigureQueue,
                                        this,
                                        queue,
                                        options,
                                        timeout,
                                        fsmCallback,
                                        bdlf::PlaceHolders::_1,  // event
                                        configureSpan),
                   eventCallback !=
                       0);  // if there is no callback, do not call
                            // 'asyncRequestNotifier' on failure.
    return rc;
}

int BrokerSession::toFsm(const bmqimp::BrokerSession::FsmCallback& fsmCallback,
                         const EventCallback&                      fsmEntrance,
                         bool everCallBack)
{
    int rc = bmqt::GenericResult::e_SUCCESS;

    // Check if disconnecting or not started
    if (!d_acceptRequests) {
        // The session is being/has been stopped or not started: client should
        // not send any request to the broker.
        BALL_LOG_ERROR << "Unable to process queue request "
                       << "[reason: 'SESSION_STOPPED']";
        rc = bmqt::GenericResult::e_REFUSED;
    }
    else {
        // Add to event queue
        bsl::shared_ptr<Event> queueEvent = createEvent();

        queueEvent->configureAsRequestEvent(fsmEntrance);

        rc = enqueueFsmEvent(queueEvent);
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::GenericResult::e_SUCCESS && everCallBack)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Since FSM did not work, need to execute the `eventCallback`.
        fsmCallback(0);  // empty context indicates "failed to enqueue".
    }

    return rc;
}

int BrokerSession::closeQueue(const bsl::shared_ptr<Queue>& queue,
                              bsls::TimeInterval            timeout)
{
    bslmt::Semaphore syncOperationSemaphore;
    int              rc = bmqt::GenericResult::e_NOT_READY;

    const bmqimp::BrokerSession::FsmCallback fsmCallback =
        bdlf::BindUtil::bind(&BrokerSession::syncRequestNotifier,
                             this,
                             &syncOperationSemaphore,
                             &rc,
                             bdlf::PlaceHolders::_1);  // request

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> closeSpan(
        createDTSpan("bmq.queue.close", baggage));

    toFsm(fsmCallback,
          bdlf::BindUtil::bind(&BrokerSession::doCloseQueue,
                               this,
                               queue,
                               timeout,
                               fsmCallback,
                               bdlf::PlaceHolders::_1,  // event
                               closeSpan),
          true);  // always call 'syncRequestNotifier'

    // Wait result
    syncOperationSemaphore.wait();

    return rc;
}

int BrokerSession::closeQueueAsync(const bsl::shared_ptr<Queue>& queue,
                                   bsls::TimeInterval            timeout,
                                   const EventCallback&          eventCallback)
{
    const bmqimp::BrokerSession::FsmCallback fsmCallback =
        bdlf::BindUtil::bind(&bmqimp::BrokerSession::asyncRequestNotifier,
                             this,
                             bdlf::PlaceHolders::_1,  // request
                             bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                             queue->correlationId(),
                             queue,
                             eventCallback);

    bmqpi::DTSpan::Baggage baggage(d_allocator_p);
    fillDTSpanQueueBaggage(&baggage, *queue);
    bsl::shared_ptr<bmqpi::DTSpan> closeSpan(
        createDTSpan("bmq.queue.close", baggage));

    int rc = toFsm(fsmCallback,
                   bdlf::BindUtil::bind(&BrokerSession::doCloseQueue,
                                        this,
                                        queue,
                                        timeout,
                                        fsmCallback,
                                        bdlf::PlaceHolders::_1,  // event
                                        closeSpan),
                   eventCallback !=
                       0);  // if there is no callback, do not call
                            // 'asyncRequestNotifier' on failure.
    return rc;
}

void BrokerSession::closeQueueSync(const bsl::shared_ptr<Queue>& queue,
                                   bsls::TimeInterval            timeout,
                                   const EventCallback&          eventCallback)
{
    bslmt::Semaphore    syncOperationSemaphore;
    const EventCallback callbackAdapter = bdlf::BindUtil::bind(
        &eventCallbackAdapter,
        &syncOperationSemaphore,
        eventCallback,
        bdlf::PlaceHolders::_1);  // event
    // The adapter executes given 'eventCallback' and posts the semaphore.

    if (d_usingSessionEventHandler) {
        closeQueueAsync(queue, timeout, callbackAdapter);  // use adapter
    }
    else {
        const bmqimp::BrokerSession::FsmCallback fsmCallback =
            bdlf::BindUtil::bind(&BrokerSession::manualSyncRequestNotifier,
                                 this,
                                 bdlf::PlaceHolders::_1,  // request
                                 bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                                 queue->correlationId(),
                                 queue,
                                 callbackAdapter);  // adapter

        bmqpi::DTSpan::Baggage baggage(d_allocator_p);
        fillDTSpanQueueBaggage(&baggage, *queue);
        bsl::shared_ptr<bmqpi::DTSpan> closeSpan(
            createDTSpan("bmq.queue.close", baggage));

        toFsm(fsmCallback,
              bdlf::BindUtil::bind(&BrokerSession::doCloseQueue,
                                   this,
                                   queue,
                                   timeout,
                                   fsmCallback,
                                   bdlf::PlaceHolders::_1,  // event
                                   closeSpan),
              true);  // always call 'manualSyncRequestNotifier'
    }

    // BS does enqueue/call the callback even if the call fails.
    // Wait result.
    syncOperationSemaphore.wait();

    // The result is communicated to the caller in the `eventCallback`.
}

bsl::shared_ptr<Queue> BrokerSession::lookupQueue(const bmqt::Uri& uri) const

{
    return d_queueManager.lookupQueue(uri);
}

bsl::shared_ptr<Queue>
BrokerSession::lookupQueue(const bmqt::CorrelationId& correlationId) const
{
    return d_queueManager.lookupQueue(correlationId);
}

bsl::shared_ptr<Queue>
BrokerSession::lookupQueue(const bmqp::QueueId& queueId) const
{
    return d_queueManager.lookupQueue(queueId);
}

bool BrokerSession::acceptUserEvent(const bdlbb::Blob&        eventBlob,
                                    const bsls::TimeInterval& timeout)
{
    // executed by the APPLICATION thread

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_extensionBufferEmpty)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        const bsls::TimeInterval expireAfter =
            bmqsys::Time::nowMonotonicClock() + timeout;

        bslmt::LockGuard<bslmt::Mutex> guard(&d_extensionBufferLock);
        // LOCK
        while (!d_extensionBufferEmpty) {
            const int rc = d_extensionBufferCondition.timedWait(
                &d_extensionBufferLock,
                expireAfter);
            if (rc == -1) {    // timed out
                return false;  // RETURN
            }
        }
    }

    // Accept the blob to the FSM
    bmqt::GenericResult::Enum res = processPacket(eventBlob);

    return res == bmqt::GenericResult::e_SUCCESS;
}

void BrokerSession::setupPutExpirationTimer(const bsls::TimeInterval& timeout)
{
    // executed by the FSM thread

    BSLS_ASSERT_SAFE(d_fsmThreadChecker.inSameThread());
    BSLS_ASSERT_SAFE(timeout > 0);

    BALL_LOG_INFO << "Setup PUT expiration timer to " << timeout;

    d_scheduler_p->scheduleEvent(
        &d_messageExpirationTimeoutHandle,
        timeout,
        bdlf::BindUtil::bind(&BrokerSession::onPendingPutExpirationTimeout,
                             this));
}

void BrokerSession::removePendingControlMessage(
    const RequestManagerType::RequestSp& context)
{
    // Remove the request from the retransmission buffer
    bmqt::MessageGUID guid;
    guid.fromHex(context->userData().theString().data());
    const int rc = d_messageCorrelationIdContainer.remove(guid);

    BSLS_ASSERT_SAFE(0 == rc);
    (void)rc;

    // Reset request id
    context->adoptUserData(bdld::Datum::createNull());
}

bslma::ManagedPtr<void>
BrokerSession::activateDTSpan(const bsl::shared_ptr<bmqpi::DTSpan>& span)
{
    const bsl::shared_ptr<bmqpi::DTContext>& context =
        d_sessionOptions.traceContext();
    bslma::ManagedPtr<void> result;
    if (span && context) {
        result = context->scope(span);
    }
    return result;
}

bsl::shared_ptr<bmqpi::DTSpan>
BrokerSession::createDTSpan(bsl::string_view              operation,
                            const bmqpi::DTSpan::Baggage& baggage) const
{
    const bsl::shared_ptr<bmqpi::DTTracer>& tracer = d_sessionOptions.tracer();
    const bsl::shared_ptr<bmqpi::DTContext>& context =
        d_sessionOptions.traceContext();
    bsl::shared_ptr<bmqpi::DTSpan> result;
    if (tracer && context) {
        bsl::shared_ptr<bmqpi::DTSpan> parent(context->span());
        result = tracer->createChildSpan(parent, operation, baggage);
    }
    return result;
}

int BrokerSession::post(const bdlbb::Blob&        eventBlob,
                        const bsls::TimeInterval& timeout)
{
    // Prevent send of an empty/invalid blob: when using the
    // MessageEventBuilder, if no messages were added (i.e., 'PackMessage()'
    // was not called), it returns a blob composed only of an EventHeader; no
    // need to send those.
    if (eventBlob.length() <= static_cast<int>(sizeof(bmqp::EventHeader))) {
        return bmqt::PostResult::e_INVALID_ARGUMENT;  // RETURN
    }

    bmqp::Event event(&eventBlob, d_allocator_p);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event.isPutEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to post event [reason: 'Not a PUT event']";
        return bmqt::PostResult::e_INVALID_ARGUMENT;  // RETURN
    }

    bmqp::PutMessageIterator putIter(d_bufferFactory_p, d_allocator_p);
    event.loadPutMessageIterator(&putIter);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!putIter.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to post event "
                       << "[reason: 'Invalid PutIterator']";
        return bmqt::PostResult::e_INVALID_ARGUMENT;  // RETURN
    }

    // NOTE: We need to validate the queues again (queue is valid and opened
    //       with write flags), despite those being checked in
    //       'bmqa::MessageEventBuilder::packMessage()' because the user could
    //       potentially build an event, close a queue and then post the event;
    //       which is forbidden (once queue close has been called, the queue
    //       should only be used in 'read-only' mode).  We still do it in
    //       'packMessage' though so that we can reject a single message; here,
    //       if any message in the event is invalid, we reject the entire
    //       event.  Note also that we only iterate once over the messages, so
    //       we update stats as we validate; if any message fails, we stop, but
    //       we don't 'revert' the stats.

    // Iterate over the messages in the event, to validate queues and update
    // their associated stats.
    int msgCount = 0;
    int rc       = d_queueManager.updateStatsOnPutEvent(&msgCount, putIter);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to post event "
                       << "[reason: 'Invalid event', rc: " << rc << "]";

        return bmqt::PostResult::e_INVALID_ARGUMENT;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_acceptRequests)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // In the process of stopping or not started, can't post the event.
        BALL_LOG_ERROR << "Unable to post event [reason: 'SESSION_STOPPED']";
        return bmqt::PostResult::e_NOT_CONNECTED;  // RETURN
    }
    bool isAccepted = acceptUserEvent(eventBlob, timeout);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isAccepted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Unable to post event [reason: 'LIMIT']";
        return bmqt::PostResult::e_BW_LIMIT;  // RETURN
    }

    // Dump the PUT event, if requested by a dump command
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_messageDumper.isEventDumpEnabled<bmqp::EventType::e_PUT>())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_INFO_BLOCK
        {
            d_messageDumper.dumpPutEvent(BALL_LOG_OUTPUT_STREAM,
                                         event,
                                         d_bufferFactory_p);
        }
    }

    // Update stats
    d_eventsStats.onEvent(EventsStatsEventType::e_PUT,
                          event.blob()->length(),
                          msgCount);

    return bmqt::PostResult::e_SUCCESS;
}

int BrokerSession::confirmMessage(const bsl::shared_ptr<bmqimp::Queue>& queue,
                                  const bmqt::MessageGUID&  messageId,
                                  const bsls::TimeInterval& timeout)
{
    // PRECONDTIONS
    BSLS_ASSERT(queue && "non-null 'queue' must be specified");

    // Ensure the queueId is valid, and the queue is not being closed
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queue->id() ==
                                              Queue::k_INVALID_QUEUE_ID)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            !bmqt::QueueFlagsUtil::isReader(queue->flags()))) {
        // Confirmation requires a queue opened in read mode.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    // Ensure the queueId is valid, and the queue is not being closed
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queue->state() !=
                                              QueueState::e_OPENED)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    // Ensure confirm messages are expected by the queue
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queue->atMostOnce())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::GenericResult::e_SUCCESS;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_acceptRequests)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // In the process of stopping or not started, can't send confirmation
        // message.
        BALL_LOG_ERROR << "Unable to confirm message "
                       << "[reason: 'SESSION_STOPPED']";
        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    // Build event
    bmqp::ConfirmEventBuilder      builder(d_blobSpPool_p, d_allocator_p);
    bmqt::EventBuilderResult::Enum rc =
        builder.appendMessage(queue->id(), queue->subQueueId(), messageId);

    // no handling bmqt::EventBuilderResult::e_EVENT_TOO_BIG error since there
    // is exactly one message being appended.

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            rc != bmqt::EventBuilderResult::e_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to append confirm message to the builder: "
                       << rc;
        return rc;  // RETURN
    }
    bool isAccepted = acceptUserEvent(builder.blob(), timeout);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isAccepted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Unable to send confirm event [reason: 'LIMIT']";
        return bmqt::GenericResult::e_TIMEOUT;  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

int BrokerSession::confirmMessages(const bdlbb::Blob&        blob,
                                   const bsls::TimeInterval& timeout)
{
    if (blob.length() <= static_cast<int>(sizeof(bmqp::EventHeader))) {
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_acceptRequests)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Unable to confirm message "
                       << "[reason: 'SESSION_STOPPED']";

        return bmqt::GenericResult::e_NOT_CONNECTED;  // RETURN
    }

    bmqp::Event event(&blob, d_allocator_p);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!event.isConfirmEvent())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to confirm event [reason: 'Not a CONFIRM "
                       << "event ']";
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    bmqp::ConfirmMessageIterator confirmIter;
    event.loadConfirmMessageIterator(&confirmIter);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!confirmIter.isValid())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Unable to confirm event [reason: 'Invalid"
                       << "ConfirmIterator']";
        return bmqt::GenericResult::e_INVALID_ARGUMENT;  // RETURN
    }

    bool isAccepted = acceptUserEvent(blob, timeout);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!isAccepted)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOG_ERROR << "Unable to send confirm event [reason: 'LIMIT']";
        return bmqt::GenericResult::e_TIMEOUT;  // RETURN
    }

    return bmqt::GenericResult::e_SUCCESS;
}

void BrokerSession::postToFsm(const bsl::function<void()>& f)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(f);

    bsl::shared_ptr<Event> event = createEvent();

    // An alternative is to use event's 'doneCallback' (or another VoidFunctor
    // data member (or a union)) and new event type that will just call it.
    // Then we do not need 'callbackAdapter'.
    // Note that every callback set by 'configureAsRequestEvent' ignores the
    // 'eventSp' parameter.
    // Note that 'doneCallback' is not used anymore.

    event->configureAsRequestEvent(
        bdlf::BindUtil::bind(&callbackAdapter,
                             f,
                             bdlf::PlaceHolders::_1));  // eventSp
    int rc = d_fsmEventQueue.pushBack(bslmf::MovableRefUtil::move(event));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BALL_LOG_ERROR << "Failed to enqueue FSM event: " << *event;
    }
}

void BrokerSession::enqueueSessionEvent(
    bmqt::SessionEventType::Enum  type,
    int                           statusCode,
    const bslstl::StringRef&      errorDescription,
    const bmqt::CorrelationId&    correlationId,
    const bsl::shared_ptr<Queue>& queue,
    const EventCallback&          eventCallback)
{
    // executed by *ANY* thread

    bsl::shared_ptr<Event> event = createEvent();
    event->configureAsSessionEvent(type,
                                   statusCode,
                                   correlationId,
                                   errorDescription);
    if (queue) {
        // Only some session events have an associated queue (e.g. 'OPEN',
        // 'CLOSE', 'CONFIGURE)
        event->insertQueue(queue);
    }

    if (eventCallback) {
        // Either a synchronous operation requires signaling on a semaphore
        // once the event is popped off the queue, or an asynchronous operation
        // is supposed to invoke a user-provided callback instead of passing
        // the event to the 'EventHandler' object.
        event->setEventCallback(eventCallback);
    }
    d_eventQueue.pushBack(event);
}

void BrokerSession::onStartTimeout()
{
    // executed by the *SCHEDULER* thread

    bsl::shared_ptr<Event> event = createEvent();
    event->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doHandleStartTimeout,
                             this,
                             bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(event);
}

void BrokerSession::onPendingPutExpirationTimeout()
{
    // executed by the *SCHEDULER* thread

    bsl::shared_ptr<Event> event = createEvent();
    event->configureAsRequestEvent(bdlf::BindUtil::bind(
        &BrokerSession::doHandlePendingPutExpirationTimeout,
        this,
        bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(event);
}

void BrokerSession::handleChannelWatermark(
    bmqio::ChannelWatermarkType::Enum type)
{
    // executed by the *IO* thread

    // Add to the FSM event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();
    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&BrokerSession::doHandleChannelWatermark,
                             this,
                             type,
                             bdlf::PlaceHolders::_1));  // eventImpl
    enqueueFsmEvent(queueEvent);
}

void BrokerSession::printStats(bsl::ostream& stream, bool includeDelta) const
{
    stream << "::::: Events >>";
    d_eventsStats.printStats(stream, includeDelta);

    stream << "::::: Queues >>";
    d_queuesStats.printStats(stream, includeDelta);

    stream << "::::: Event Queue >>";
    d_eventQueue.printStats(stream, includeDelta);
}

void BrokerSession::processDumpCommand(
    const bmqp_ctrlmsg::DumpMessages& command)
{
    d_messageDumper.processDumpCommand(command);
}

bool BrokerSession::_synchronize()
{
    bslmt::Semaphore syncOperationSemaphore;

    // Add to event queue
    bsl::shared_ptr<Event> queueEvent = createEvent();

    queueEvent->configureAsRequestEvent(
        bdlf::BindUtil::bind(&releaseSemaphore,
                             &syncOperationSemaphore,
                             bdlf::PlaceHolders::_1));  // event

    int rc = enqueueFsmEvent(queueEvent);

    if (rc != bmqt::GenericResult::e_SUCCESS) {
        return false;  // RETURN
    }

    // Wait result
    syncOperationSemaphore.wait();

    return true;
}

}  // close package namespace
}  // close enterprise namespace
