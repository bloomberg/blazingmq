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

// bmqimp_brokersession.t.cpp                                         -*-C++-*-
#include <bmqimp_brokersession.h>

// BMQ
#include <bmqimp_event.h>
#include <bmqimp_manualhosthealthmonitor.h>
#include <bmqimp_queue.h>
#include <bmqp_ackeventbuilder.h>
#include <bmqp_blobpoolutil.h>
#include <bmqp_confirmeventbuilder.h>
#include <bmqp_crc32c.h>
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_event.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqp_protocol.h>
#include <bmqp_puteventbuilder.h>
#include <bmqp_queueid.h>
#include <bmqp_schemaeventbuilder.h>
#include <bmqpi_dtcontext.h>
#include <bmqpi_dtspan.h>
#include <bmqpi_dttracer.h>
#include <bmqscm_version.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqio_status.h>
#include <bmqio_testchannel.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlcc_deque.h>
#include <bdlf_memfn.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_signaler.h>
#include <bsl_memory.h>
#include <bslma_managedptr.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadutil.h>
#include <bslmt_timedsemaphore.h>
#include <bsls_assert.h>
#include <bsls_platform.h>
#include <bsls_systemclocktype.h>
#include <bsls_systemtime.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using bmqimp::ManualHostHealthMonitor;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// CONSTANTS
const char k_URI[] = "bmq://ts.trades.myapp/my.queue?id=my.app";

/// Struct to initialize system time component
struct TestClock {
    // DATA
    bdlmt::EventScheduler& d_scheduler;

    bdlmt::EventSchedulerTestTimeSource d_timeSource;

    // CREATORS
    TestClock(bdlmt::EventScheduler& scheduler)
    : d_scheduler(scheduler)
    , d_timeSource(&scheduler)
    {
        // NOTHING
    }

    // MANIPULATORS
    bsls::TimeInterval realtimeClock() { return d_timeSource.now(); }

    bsls::TimeInterval monotonicClock() { return d_timeSource.now(); }

    bsls::Types::Int64 highResTimer()
    {
        return d_timeSource.now().totalNanoseconds();
    }
};

static void eventHandler(bsl::shared_ptr<bmqimp::Event>*       resultEvent,
                         bslmt::TimedSemaphore&                eventSem,
                         const bsl::shared_ptr<bmqimp::Event>& event)
{
    PV_SAFE("Got an event: " << *event);

    *resultEvent = event;
    eventSem.post();
}

static void eventHandlerSyncCall(bsl::shared_ptr<bmqimp::Event>* resultEvent,
                                 bmqimp::BrokerSession*          session,
                                 bsl::shared_ptr<bmqimp::Queue>  queue,
                                 bslmt::TimedSemaphore&          eventSem,
                                 const bsl::shared_ptr<bmqimp::Event>& event)
{
    const bsls::TimeInterval& timeout = bsls::TimeInterval();
    bmqt::QueueOptions        queueOptions;

    PV_SAFE("Got an event: " << *event);

    *resultEvent = event;

    // Check SYNC queue API
    BMQTST_ASSERT_EQ(session->openQueue(queue, timeout),
                     bmqt::OpenQueueResult::e_NOT_CONNECTED);

    BMQTST_ASSERT_EQ(session->configureQueue(queue, queueOptions, timeout),
                     bmqt::ConfigureQueueResult::e_INVALID_QUEUE);

    BMQTST_ASSERT_EQ(session->closeQueue(queue, timeout),
                     bmqt::CloseQueueResult::e_UNKNOWN_QUEUE);

    eventSem.post();
}

/// REVISIT: usage of these semaphores here and everywhere after #1974.
static int stateCb(bmqimp::BrokerSession::State::Enum oldState,
                   bmqimp::BrokerSession::State::Enum newState,
                   bsls::AtomicInt*                   startCounter,
                   bsls::AtomicInt*                   stopCounter)
{
    PV_SAFE("StateCb: " << oldState << " -> " << newState);

    if (newState == bmqimp::BrokerSession::State::e_STARTING) {
        ++(*startCounter);
    }
    else if (newState == bmqimp::BrokerSession::State::e_STOPPED) {
        ++(*stopCounter);
    }

    return 0;
}

static int transitionCb(
    bmqimp::BrokerSession::State::Enum                   oldState,
    bmqimp::BrokerSession::State::Enum                   newState,
    bsl::vector<bmqimp::BrokerSession::StateTransition>* transitionTable,
    bsls::AtomicInt*                                     rc,
    bslmt::TimedSemaphore*                               doneSem)
{
    PV_SAFE("transitionCb: " << oldState << " -> " << newState
                             << " (tableSize:" << transitionTable->size()
                             << ")");

    bsl::vector<bmqimp::BrokerSession::StateTransition>::iterator it;
    for (it = transitionTable->begin(); it != transitionTable->end(); ++it) {
        if (it->d_currentState == oldState && it->d_newState == newState) {
            transitionTable->erase(it);
            break;
        }
    }

    if (transitionTable->empty()) {
        doneSem->post();
    }

    return *rc;
}

void channelSetEventHandler(const bsl::shared_ptr<bmqimp::Event>& event,
                            bsls::AtomicInt&                      eventCounter,
                            bslmt::TimedSemaphore&                stopSem,
                            int                                   maxEvents)
{
    const bmqt::CorrelationId k_EMPTY_CORRID;

    PV_SAFE("Incoming event: " << *event);

    BMQTST_ASSERT(event != 0);
    BMQTST_ASSERT_EQ(event->type(), bmqimp::Event::EventType::e_SESSION);

    ++eventCounter;
    if (eventCounter == maxEvents) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTION_LOST);
        return;  // RETURN
    }

    if (eventCounter + 1 > maxEvents) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_DISCONNECTED);
        stopSem.post();
        return;  // RETURN
    }

    int order = eventCounter % 3;

    if (eventCounter == 1) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTED);
    }
    else if (eventCounter == 2) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTION_LOST);
    }
    else if (order == 0) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_RECONNECTED);
    }
    else if (order == 1) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_STATE_RESTORED);
    }
    else if (order == 2) {
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTION_LOST);
    }
    else {
        BSLS_ASSERT_OPT(false && "Unreachable by design");
    }

    BMQTST_ASSERT_EQ(event->statusCode(), 0);
    BMQTST_ASSERT_EQ(event->correlationId(), k_EMPTY_CORRID);
}

void channelEventHandler(const bsl::shared_ptr<bmqimp::Event>& event,
                         bslmt::TimedSemaphore*                eventSem)
{
    PV_SAFE("Incoming event: " << *event);

    eventSem->post();
}

void sessionEventHandler(
    bdlcc::Deque<bsl::shared_ptr<bmqimp::Event> >* eventQueue,
    const bsl::shared_ptr<bmqimp::Event>&          event)
{
    BMQTST_ASSERT(event);

    // May be Session or Message event
    PV_SAFE("Incoming event: " << *event);

    eventQueue->pushBack(event);
}

bool waitRealTime(bslmt::TimedSemaphore* sem)
{
    BMQTST_ASSERT(sem);

    const bsls::TimeInterval k_REALTIME_TIMEOUT = bsls::TimeInterval(15);

    int rc = sem->timedWait(
        bsls::SystemTime::now(bsls::SystemClockType::e_REALTIME) +
        k_REALTIME_TIMEOUT);

    return rc == 0;
}

bool isConfigure(const bmqp_ctrlmsg::ControlMessage& request)
{
    return request.choice().isConfigureStreamValue() ||
           request.choice().isConfigureQueueStreamValue();
}

void makeResponse(bmqp_ctrlmsg::ControlMessage*       response,
                  const bmqp_ctrlmsg::ControlMessage& request)
{
    BMQTST_ASSERT(!request.rId().isNull());
    response->rId().makeValue(request.rId().value());

    if (request.choice().isConfigureStreamValue()) {
        response->choice().makeConfigureStreamResponse();
        response->choice().configureStreamResponse().request() =
            request.choice().configureStream();
    }
    else {
        response->choice().makeConfigureQueueStreamResponse();
        response->choice().configureQueueStreamResponse().request() =
            request.choice().configureQueueStream();
    }
}

void getConsumerInfo(bmqp_ctrlmsg::ConsumerInfo*  out,
                     bmqp_ctrlmsg::ControlMessage request)
{
    if (request.choice().isConfigureStreamValue()) {
        *out = bmqp::ProtocolUtil::consumerInfo(
            request.choice().configureStream().streamParameters());
    }
    else {
        bmqp_ctrlmsg::StreamParameters             temp;
        const bmqp_ctrlmsg::QueueStreamParameters& in =
            request.choice().configureQueueStream().streamParameters();
        bmqp::ProtocolUtil::convert(&temp, in);
        *out = bmqp::ProtocolUtil::consumerInfo(temp);
    }
}

/// This class provides a wrapper on top of the BrokerSession under test and
/// implements a few mechanisms to help testing the object.
struct TestSession BSLS_CPP11_FINAL {
  public:
    // CONSTANTS
    static const bsls::TimeInterval k_EVENT_TIMEOUT;

    static const bsls::TimeInterval k_TIME_SOURCE_STEP;

  public:
    // TYPES
    enum RequestType {
        e_REQ_OPEN_QUEUE   = 0,
        e_REQ_CONFIG_QUEUE = 1,
        e_REQ_CLOSE_QUEUE  = 2,
        e_REQ_DISCONNECT   = 3,
        e_REQ_UNDEFINED    = -1
    };

    enum ErrorResult {
        e_ERR_NOT_SENT      = 0,  // request failed to send
        e_ERR_BAD_RESPONSE  = 1,  // bad response
        e_ERR_LATE_RESPONSE = 2   // late response
    };

    enum QueueTestStep {
        e_OPEN_OPENING = 0  // pending open queue request
        ,
        e_OPEN_CONFIGURING = 1  // pending config queue request
        ,
        e_OPEN_OPENED = 2  // no pending requests
        ,
        e_REOPEN_OPENING = 3  // pending open queue request
        ,
        e_REOPEN_CONFIGURING = 4  // pending config queue request
        ,
        e_REOPEN_REOPENED = 5  // no pending requests
        ,
        e_CONFIGURING = 6  // pending config queue request
        ,
        e_CONFIGURING_RECONFIGURING = 7  // pending config queue request
        ,
        e_CONFIGURED = 8  // no pending requests
        ,
        e_CLOSE_CONFIGURING = 9  // pending config queue request
        ,
        e_CLOSE_CLOSING = 10  // pending close queue request
        ,
        e_CLOSE_CLOSED = 11  // no pending requests
    };

    enum LateResponseTestStep {
        e_LATE_OPEN_OPENING = 0  // late open 1st part respose
        ,
        e_LATE_OPEN_CONFIGURING_CFG = 1  // late open 2nd part respose
        ,
        e_LATE_OPEN_CONFIGURING_CLS = 2  // pending close queue request
        ,
        e_LATE_REOPEN_OPENING = 3  // late reopen 1st part respose
        ,
        e_LATE_REOPEN_CONFIGURING_CFG = 4  // late reopen 2st part respose
        ,
        e_LATE_REOPEN_CONFIGURING_CLS = 5  // pending close queue request
        ,
        e_LATE_RECONFIGURING = 6  // expired reconfigure request
        ,
        e_LATE_CLOSE_CONFIGURING = 7  // late close 1st part response
        ,
        e_LATE_CLOSE_CLOSING = 8  // late close 2st part response
    };

    typedef bdlcc::Deque<bsl::shared_ptr<bmqimp::Event> > EventQueue;

  public:
    // PUBLIC DATA

    bslma::Allocator* d_allocator_p;
    // Allocator to use

    bdlbb::PooledBlobBufferFactory d_blobBufferFactory;
    // Buffer factory provided to the
    // various builders

    /// Blob pool used to provide blobs to event builders.
    bmqp::BlobPoolUtil::BlobSpPool d_blobSpPool;

    bdlmt::EventScheduler& d_scheduler;
    // event scheduler used in the
    // broker session

    bmqio::TestChannel d_testChannel;
    // mocked network channel object
    // to be used in the broker session

    EventQueue d_eventQueue;
    // thread-safe deque to store
    // incoming BlazingMQ events

    bmqimp::BrokerSession d_brokerSession;
    // the broker session object
    // under the test

    bdlmt::SignalerConnection d_onChannelCloseHandler;
    // signaler handler associated with
    // onChannelClose slot

    TestClock* d_testClock_p;
    // pointer to struct to initialize system time

    int d_startCounter;
    int d_stopCounter;
    // register state changes
  private:
    TestSession(const TestSession& other) BSLS_CPP11_DELETED;
    TestSession& operator=(const TestSession& other) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a `TestSession` object using the specified `sessionOptions`,
    /// `scheduler`, and `allocator` to supply memory.
    explicit TestSession(
        const bmqt::SessionOptions& sessionOptions,
        bdlmt::EventScheduler&      scheduler,
        bool                        useEventHandler = true,
        bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator());

    /// Create a `TestSession` object using the specified `sessionOptions`,
    /// `clockPtr`, and `allocator` to supply memory.
    explicit TestSession(
        const bmqt::SessionOptions& sessionOptions,
        TestClock&                  testClock,
        bool                        useEventHandler = true,
        bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator());

    ~TestSession();

    // ACCESSORS
    bmqimp::BrokerSession&    session();
    bmqio::TestChannel&       channel();
    bslma::Allocator*               allocator();
    bmqp::BlobPoolUtil::BlobSpPool& blobSpPool();

    // MANIPULATORS

    /// Start the broker session object under the test and sets the test
    /// channel.  Internally this method waits for
    /// SessionEventType::e_CONNECTED event.
    /// Assert in case of any error or internal timeout.
    void startAndConnect(bool expectHostUnhealthy = false);

    /// Stop synchronously.  Verify immediate presence of DISCONNECTED event
    /// and the state.
    bool stop();

    /// Open the queue by sending and processing corresponding requests.
    /// Inject open and configure queue responses and wait for
    /// SessionEventType::e_QUEUE_OPEN_RESULT event.
    /// Assert in case of any error or expired timeout
    void openQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                   const bsls::TimeInterval& timeout = bsls::TimeInterval(5),
                   const bool                skipReaderCfgs = false);

    /// Execute open queue procedure and reproduce requested error type
    /// at corresponding stage (open queue or open configure queue).
    /// Assert in case of any error or expired timeout
    void openQueueWithError(bsl::shared_ptr<bmqimp::Queue> queue,
                            const RequestType              requestType,
                            const ErrorResult              errorResult,
                            const bsls::TimeInterval&      timeout);

    /// Process the specified `queue` reopening by injecting open and
    /// configure queue responses and wait for
    /// `SessionEventType::e_QUEUE_REOPEN_RESULT` event.  Assert in case of
    /// any error or expired timeout.
    void
    reopenQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                const bsls::TimeInterval& timeout = bsls::TimeInterval(5));

    /// Closes the specified `queue` by injecting close and configure queue
    /// responses and wait for `SessionEventType::e_QUEUE_CLOSE_RESULT`
    /// event.  Assert in case of any error or expired timeout.
    void closeQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                    const bsls::TimeInterval& timeout = bsls::TimeInterval(5),
                    bool                      isFinal = true);

    /// Execute close queue procedure and reproduce requested error type
    /// at corresponding stage (close configure queue and close queue).
    /// Assert in case of any error or expired timeout
    void closeQueueWithError(bsl::shared_ptr<bmqimp::Queue> queue,
                             const RequestType              requestType,
                             const ErrorResult              errorResult,
                             bool                           isFinal,
                             const bsls::TimeInterval&      timeout);

    /// Close the specified `queue` by injecting close queueresponses and
    /// wait for `SessionEventType::e_QUEUE_CLOSE_RESULT` event.  Assert in
    /// case of any error or expired timeout.
    void closeDeconfiguredQueue(
        bsl::shared_ptr<bmqimp::Queue> queue,
        const bsls::TimeInterval&      timeout = bsls::TimeInterval(5));

    void configureQueueSync(
        bsl::shared_ptr<bmqimp::Queue>& queue,
        const bmqt::QueueOptions&       options,
        const bsls::TimeInterval&       timeout = bsls::TimeInterval(5));

    /// For the specified `queue` call async opening and verify open queue
    /// request is sent to the channel.  Return control message sent to the
    /// channel.
    bmqp_ctrlmsg::ControlMessage
    openQueueFirstStep(bsl::shared_ptr<bmqimp::Queue> queue,
                       const bsls::TimeInterval&      timeout);

    /// For the specified `queue` in the OPENED state emulate channel drop
    /// so that the queue state is changed to PENDING and then reopening
    /// procedure is initiated.  Return control message with open queue
    /// request sent to the channel.
    bmqp_ctrlmsg::ControlMessage
    reopenQueueFirstStep(bsl::shared_ptr<bmqimp::Queue> queue);

    /// For the specified `queue` call async opening and verify open queue
    /// request is sent to the channel.  Wait until request timeout and send
    /// late open queue response.  Return the last control message sent to
    /// the channel.
    bmqp_ctrlmsg::ControlMessage
    openQueueFirstStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                              const bsls::TimeInterval&      timeout);

    /// For the specified `queue` in the OPENED state emulate channel drop
    /// so that the queue state is changed to PENDING and then reopening
    /// procedure is initiated.  Wait until reopen request is expired and
    /// send back late open response.  Return control message with close
    /// queue request sent to the channel.
    bmqp_ctrlmsg::ControlMessage
    reopenQueueFirstStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                                const bsls::TimeInterval&      timeout);

    /// For the specified `queue` send the close queue response generated on
    /// the specified `closeRequest`.  Verify the queue gets closed waiting
    /// for the close queue event depending on the specified
    /// `waitCloseEvent` flag.
    void closeQueueSecondStep(bsl::shared_ptr<bmqimp::Queue>      queue,
                              const bmqp_ctrlmsg::ControlMessage& closeRequest,
                              bool waitCloseEvent);

    /// For the specified `queue` call async closing and verify close queue
    /// request is sent to the channel.  Wait until request timeout and send
    /// late close queue response.  Return the last control message sent to
    /// the channel.
    bmqp_ctrlmsg::ControlMessage
    closeQueueSecondStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                                const bsls::TimeInterval&      timeout);

    bmqp_ctrlmsg::ControlMessage
    arriveAtStepWithCfgs(bsl::shared_ptr<bmqimp::Queue> queue,
                         const QueueTestStep            step,
                         const bsls::TimeInterval&      timeout);

    bmqp_ctrlmsg::ControlMessage
    arriveAtStepWithoutCfgs(bsl::shared_ptr<bmqimp::Queue> queue,
                            const QueueTestStep            step,
                            const bsls::TimeInterval&      timeout);

    /// Bring the specified `queue` to the specified test `step` by sending
    /// and processing corresponding requests and responses and waiting for
    /// related session events.  The steps are always reached one-by-one
    /// starting from the `e_OPEN_OPENING`.  Return last sent request.
    /// Assert in case of any error or expired timeout.  Two implementations
    /// for reader and writer queues (writers skip queue configuring steps).
    bmqp_ctrlmsg::ControlMessage
    arriveAtStep(bsl::shared_ptr<bmqimp::Queue> queue,
                 const QueueTestStep            step,
                 const bsls::TimeInterval&      timeout,
                 const bool                     skipReaderCfgs = false);

    void arriveAtLateResponseStep(bsl::shared_ptr<bmqimp::Queue> queue,
                                  const LateResponseTestStep     step,
                                  const bsls::TimeInterval&      timeout);
    void arriveAtLateResponseStepReader(bsl::shared_ptr<bmqimp::Queue> queue,
                                        const LateResponseTestStep     step,
                                        const bsls::TimeInterval& timeout);

    /// Bring the specified `queue` to the specified late response test
    /// `step` by sending and processing corresponding requests and
    /// responses and waiting for related session events.  Two versions for
    /// reader and writer queues (writers skip queue configuring steps).
    void arriveAtLateResponseStepWriter(bsl::shared_ptr<bmqimp::Queue> queue,
                                        const LateResponseTestStep     step,
                                        const bsls::TimeInterval& timeout);

    /// Return a new queue object
    bsl::shared_ptr<bmqimp::Queue>
    createQueue(const char*               name,
                bsls::Types::Uint64       flags,
                const bmqt::QueueOptions& options = bmqt::QueueOptions());

    /// Return a new queue object with the specified `queueFlags` which has
    /// already been put into the specified test `step` by sending and
    /// processing corresponsing requests and responses and waiting for
    /// related session events.  Assert in case of any error or expired
    /// timeout.
    bsl::shared_ptr<bmqimp::Queue> createQueueOnStep(
        const QueueTestStep       step,
        bsls::Types::Uint64       queueFlags,
        const bsls::TimeInterval& timeout = bsls::TimeInterval(5));
    bsl::shared_ptr<bmqimp::Queue> createQueueOnStep(
        const LateResponseTestStep step,
        bsls::Types::Uint64        queueFlags,
        const bsls::TimeInterval&  timeout = bsls::TimeInterval(5));

    /// Wait up to the specified `timeout` for a call to close the channel.
    /// If such a call happens, invoke the closeSignaler and return true.
    bool
    waitForChannelClose(const bsls::TimeInterval& timeout = k_EVENT_TIMEOUT);

    /// Wait up to the specified `timeout` for the specified `queue` to
    /// reach the specified queue `state`.  If this happens return true.
    bool
    waitForQueueState(const bsl::shared_ptr<bmqimp::Queue>& queue,
                      const bmqimp::QueueState::Enum        state,
                      const bsls::TimeInterval& timeout = k_EVENT_TIMEOUT);

    /// Wait up to the specified `timeout` for the specified `queue` to
    /// be closed and removed from the queue list.  If this happens return
    /// true.
    bool
    waitForQueueRemoved(const bsl::shared_ptr<bmqimp::Queue>& queue,
                        const bsls::TimeInterval& timeout = k_EVENT_TIMEOUT);

    /// Stop the broker session object under the test.  Checks that
    /// the object sends disconnect request.  Inject disconnect response
    /// and wait for e_DISCONNECTED event if the specified
    /// `waitForDisconnected` flag is true (default).
    /// Assert in case of any error or internal timeout.
    void stopGracefully(bool waitForDisconnected = true);

    /// Set a mocked network channel to the broker session object under the
    /// test.
    void setChannel();

    bool waitConnectedEvent();

    bool waitReconnectedEvent();
    bool waitStateRestoredEvent();
    bool waitDisconnectedEvent();

    /// Wait for the specific session event.  If there is no incoming event
    /// the method will block for the specified timeout (5s default) and
    /// then fire assert if there is still no event.
    bool waitConnectionLostEvent();

    /// Wait for the event with ACK or NACK messages.  Return a shared
    /// pointer to the event object or empty pointer in case of any error.
    bsl::shared_ptr<bmqimp::Event> waitAckEvent();

    /// Wait for an e_HOST_UNHEALTHY session event.  If there is no incoming
    /// event the method will block for the specified timeout (5s default)
    /// and then fire assert if there is still no event.
    bool waitHostUnhealthyEvent();

    /// Wait for an e_QUEUE_SUSPENDED event. If there is no incoming event
    /// the method will block and then fire assert if there is still no
    /// event.
    bool waitQueueSuspendedEvent(
        bmqt::GenericResult::Enum status = bmqt::GenericResult::e_SUCCESS);

    /// Wait for an e_QUEUE_RESUMED event. If there is no incoming event
    /// the method will block and then fire assert if there is still no
    /// event.
    bool waitQueueResumedEvent(
        bmqt::GenericResult::Enum status = bmqt::GenericResult::e_SUCCESS);

    /// Wait for an e_HOST_HEALTH_RESTORED session event.  If there is no
    /// incoming event the method will block for the specified timeout (5s
    /// default) and then fire assert if there is still no event.
    bool waitHostHealthRestoredEvent();

    /// Ensure there is no incoming event.  Return false if there is an
    /// event.
    bool checkNoEvent();

    /// Return a BlazingMQ event that has been passed by the broker session
    /// object under the test to the session event handler.  If there is no
    /// incoming event this method will block forever.
    bsl::shared_ptr<bmqimp::Event> getInboundEvent();

    /// Return true if there is no data that has been sent to the test
    /// network channel.
    bool isChannelEmpty();

    /// Load the specified outMsg object with the data that has been sent to
    /// the test network channel.  Asserts if there is no control message in
    /// the write buffer of the channel.
    void getOutboundControlMessage(bmqp_ctrlmsg::ControlMessage* outMsg);

    /// Load the specified `rawEvent` object with the data that has been
    /// sent to the test network channel.  Asserts if there is no BlazingMQ
    /// Event in the write buffer of the channel.
    void getOutboundEvent(bmqp::Event* rawEvent);

    /// Send the specified control message to the broker session object
    /// under the test as if had received a BlazingMQ event from the broker.
    void sendControlMessage(const bmqp_ctrlmsg::ControlMessage& message);

    /// Send a response to the specified `request` to the broker session
    /// under testing as if it had been received from a broker.  The type of
    /// the response is defined by the type of the specified `request`.
    void sendResponse(const bmqp_ctrlmsg::ControlMessage& request);

    /// Send status message to the specified `request` to the broker session
    /// under testing as if it had been received from a broker.
    void sendStatus(const bmqp_ctrlmsg::ControlMessage& request);

    /// Check that a request of the specified `requestType` is sent to the
    /// channel and return the request object.
    bmqp_ctrlmsg::ControlMessage
    getNextOutboundRequest(const RequestType requestType);

    /// Check that a request of the specified `requestType` is sent to the
    /// channel and return the request ID.
    int verifyRequestSent(const RequestType requestType);

    /// Check that a close queue request is sent to the
    /// channel and return the request object
    bmqp_ctrlmsg::ControlMessage verifyCloseRequestSent(bool isFinal);

    /// Return `true` if a session event of the specified `eventType` and
    /// with the specified `eventStatus` was generated by the session object
    /// under the test, `false` otherwise.
    bool verifyOperationResult(const bmqt::SessionEventType::Enum eventType,
                               int                                eventStatus);

    /// Check that a session event of the `e_QUEUE_OPEN_RESULT` and with the
    /// specified `eventStatus` was generated by the session object under
    /// the test.  Check that there are no more session events.  Check that
    /// the queue comes into the specified `queueState`.
    void verifyOpenQueueErrorResult(
        const bmqp_ctrlmsg::StatusCategory::Value eventStatus,
        const bsl::shared_ptr<bmqimp::Queue>&     queue,
        const bmqimp::QueueState::Enum            queueState);

    /// Check that a session event of the `e_QUEUE_CLOSE_RESULT` and with
    /// the specified `eventStatus` was generated by the session object
    /// under the test.  Also check that there are no more session events
    /// and that the queue is set to specified state.
    void verifyCloseQueueResult(
        const bmqp_ctrlmsg::StatusCategory::Value status,
        const bsl::shared_ptr<bmqimp::Queue>&     queue,
        const bmqimp::QueueState::Enum state = bmqimp::QueueState::e_CLOSED);

    /// Called when the test channel is closed and provides the specified
    /// close `status`.
    void onChannelClose(const bmqio::Status& status);

    /// Wait until stateCb is called informing about reaching the STOPPED
    /// state.  Must be called after `session.stop()` call.  Return false
    /// in case of internal timeout.
    bool verifySessionIsStopped();

    /// A callback provided to the BrokerSession to be called when the
    /// session switches from the specified `oldState` to the specified
    /// `newState` upon the specified `event`.
    int stateCb(bmqimp::BrokerSession::State::Enum    oldState,
                bmqimp::BrokerSession::State::Enum    newState,
                bmqimp::BrokerSession::FsmEvent::Enum event);

    /// Advance the test time source for the specified `step`. Drain the
    /// session FSM event queue before changing the time.
    void advanceTime(const bsls::TimeInterval& step);

    /// Set the specified `writeStatus` to the test network channel and
    /// notify the BrokerSession about channel LWM.
    void setChannelLowWaterMark(bmqio::StatusCategory::Enum writeStatus);
};

// CLASS DATA
const bsls::TimeInterval TestSession::k_EVENT_TIMEOUT(30);

const bsls::TimeInterval TestSession::k_TIME_SOURCE_STEP(0.001);

void TestSession::advanceTime(const bsls::TimeInterval& step)
{
    // Before changing the time drain the FSM queue to verify that any ongoing
    // request is completed.  To achieve that call 'session._synchronize()'.
    // By return of this call the FSM queue is expected to be empty and there
    // should be no in progress requests.
    int rc = session()._synchronize();
    BMQTST_ASSERT(rc);

    if (!d_testClock_p) {
        // System time source is used.  Cannot advance time manually.
        return;  // RETURN
    }

    d_testClock_p->d_timeSource.advanceTime(step);

    bslmt::Semaphore sem;

    typedef void (bslmt::Semaphore::*PostFn)();

    d_scheduler.scheduleEvent(
        d_testClock_p->d_timeSource.now(),
        bdlf::BindUtil::bind(static_cast<PostFn>(&bslmt::Semaphore::post),
                             &sem));
    sem.wait();
}

bsl::shared_ptr<bmqimp::Event> TestSession::getInboundEvent()
{
    bsl::shared_ptr<bmqimp::Event> event;

    if (d_brokerSession.isUsingSessionEventHandler()) {
        d_eventQueue.popFront(&event);
    }
    else {
        event = d_brokerSession.nextEvent(bsls::TimeInterval(5));
    }

    return event;
}

TestSession::TestSession(const bmqt::SessionOptions& sessionOptions,
                         bdlmt::EventScheduler&      scheduler,
                         bool                        useEventHandler,
                         bslma::Allocator*           allocator)
: d_allocator_p(allocator)
, d_blobBufferFactory(1024, d_allocator_p)
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory,
                                         bmqtst::TestHelperUtil::allocator()))
, d_scheduler(scheduler)
, d_testChannel(d_allocator_p)
, d_eventQueue(bsls::SystemClockType::e_MONOTONIC, d_allocator_p)
, d_brokerSession(&d_scheduler,
                  &d_blobBufferFactory,
                  &d_blobSpPool,
                  sessionOptions,
                  useEventHandler
                      ? bdlf::BindUtil::bind(&sessionEventHandler,
                                             &d_eventQueue,
                                             bdlf::PlaceHolders::_1)  // event
                      : bmqimp::EventQueue::EventHandlerCallback(),
                  bdlf::MemFnUtil::memFn(&TestSession::stateCb, this),
                  d_allocator_p)
, d_onChannelCloseHandler(d_testChannel.onClose(
      bdlf::MemFnUtil::memFn(&TestSession::onChannelClose, this)))
, d_testClock_p(0)
, d_startCounter(0)
, d_stopCounter(0)
{
    // Set peer uri for the sake of better logging
    d_testChannel.setPeerUri("tcp://testHost:1234");

    int rc = d_scheduler.start();
    BMQTST_ASSERT_EQ(rc, 0);
}

TestSession::TestSession(const bmqt::SessionOptions& sessionOptions,
                         TestClock&                  testClock,
                         bool                        useEventHandler,
                         bslma::Allocator*           allocator)
: d_allocator_p(allocator)
, d_blobBufferFactory(1024, d_allocator_p)
, d_blobSpPool(
      bmqp::BlobPoolUtil::createBlobPool(&d_blobBufferFactory,
                                         bmqtst::TestHelperUtil::allocator()))
, d_scheduler(testClock.d_scheduler)
, d_testChannel(d_allocator_p)
, d_eventQueue(bsls::SystemClockType::e_MONOTONIC, d_allocator_p)
, d_brokerSession(&d_scheduler,
                  &d_blobBufferFactory,
                  &d_blobSpPool,
                  sessionOptions,
                  useEventHandler
                      ? bdlf::BindUtil::bind(&sessionEventHandler,
                                             &d_eventQueue,
                                             bdlf::PlaceHolders::_1)  // event
                      : bmqimp::EventQueue::EventHandlerCallback(),
                  bdlf::MemFnUtil::memFn(&TestSession::stateCb, this),
                  d_allocator_p)
, d_onChannelCloseHandler(d_testChannel.onClose(
      bdlf::MemFnUtil::memFn(&TestSession::onChannelClose, this)))
, d_testClock_p(&testClock)
, d_startCounter(0)
, d_stopCounter(0)
{
    // Set peer uri for the sake of better logging
    d_testChannel.setPeerUri("tcp://testHost:1234");

    bmqsys::Time::shutdown();
    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, d_testClock_p),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, d_testClock_p),
        bdlf::BindUtil::bind(&TestClock::highResTimer, d_testClock_p),
        d_allocator_p);

    int rc = d_scheduler.start();
    BMQTST_ASSERT_EQ(rc, 0);
}

TestSession::~TestSession()
{
    d_scheduler.cancelAllEventsAndWait();
    d_scheduler.stop();
    d_onChannelCloseHandler.disconnect();
}

// ACCESSORS
bmqimp::BrokerSession& TestSession::session()
{
    return d_brokerSession;
}

bmqio::TestChannel& TestSession::channel()
{
    return d_testChannel;
}

bslma::Allocator* TestSession::allocator()
{
    return d_allocator_p;
}

bmqp::BlobPoolUtil::BlobSpPool& TestSession::blobSpPool()
{
    return d_blobSpPool;
}

// MANIPULATORS
void TestSession::startAndConnect(bool expectHostUnhealthy)
{
    PVVV_SAFE("Starting session...");
    d_startCounter = 0;
    d_stopCounter  = 0;

    int rc = session().startAsync();

    BMQTST_ASSERT_EQ(rc, 0);
    BMQTST_ASSERT_EQ(d_startCounter, 1);
    BMQTST_ASSERT_EQ(session().state(),
                     bmqimp::BrokerSession::State::e_STARTING);

    PVV_SAFE("Session started...");

    // Enable test channel
    PV_SAFE("Set channel");
    setChannel();

    if (expectHostUnhealthy) {
        BMQTST_ASSERT(waitHostUnhealthyEvent());
    }

    BMQTST_ASSERT(waitConnectedEvent());

    PVVV_SAFE("Channel connected");
}

bool TestSession::stop()
{
    session().stop();

    bsl::shared_ptr<bmqimp::Event> event;

    d_eventQueue.popFront(&event);

    if (!event) {
        PV_SAFE("No e_DISCONNECTED event");
        return false;  // RETURN
    }
    if (event->sessionEventType() != bmqt::SessionEventType::e_DISCONNECTED) {
        PV_SAFE("Unexpected DISCONNECTED type: " << event->sessionEventType());
        return false;  // RETURN
    }
    if (event->statusCode() != bmqp_ctrlmsg::StatusCategory::E_SUCCESS) {
        PV_SAFE("Unexpected DISCONNECTED status: " << event->statusCode());
        return false;  // RETURN
    }
    if (!verifySessionIsStopped()) {
        PV_SAFE("Unexpected state: " << d_brokerSession.state());
        return false;  // RETURN
    }
    if (!checkNoEvent()) {
        return false;  // RETURN
    }
    if (d_stopCounter != 1) {
        PV_SAFE("Unexpected stopCounter: " << d_stopCounter);
        return false;  // RETURN
    }

    return true;
}

void TestSession::openQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                            const bsls::TimeInterval&      timeout,
                            const bool                     skipReaderCfgs)
{
    arriveAtStep(queue, e_OPEN_OPENED, timeout, skipReaderCfgs);
}

void TestSession::openQueueWithError(bsl::shared_ptr<bmqimp::Queue> queue,
                                     const RequestType         requestType,
                                     const ErrorResult         errorResult,
                                     const bsls::TimeInterval& timeout)
{
    BMQTST_ASSERT(queue);
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    // open queue request is not sent
    if (requestType == e_REQ_OPEN_QUEUE && errorResult == e_ERR_NOT_SENT) {
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR);
        PVVV_SAFE("Set channel write status to "
                  << status << " to reject open queue request");
        d_testChannel.setWriteStatus(status);
    }

    PVVV_SAFE("Open the queue async");
    int rc = d_brokerSession.openQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    // Verify open request (TODO: check handle queue parameters)
    PVVV_SAFE("Ensure open queue request has been sent");
    currentRequest = getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);

    // open queue request should fail to send but should be buffered
    if (requestType == e_REQ_OPEN_QUEUE && errorResult == e_ERR_NOT_SENT) {
        PVVV_SAFE("Open queue request should fail to send but should be "
                  "buffered");

        // Emulate request timeout.
        advanceTime(timeout);

        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                                   queue,
                                   bmqimp::QueueState::e_OPENING_OPN_EXPIRED);

        // reset channel write status
        PVVV_SAFE("Reset channel write status");
        d_testChannel.setWriteStatus(bmqio::Status());

        return;  // RETURN
    }

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    // bad open queue response
    if (requestType == e_REQ_OPEN_QUEUE && errorResult == e_ERR_BAD_RESPONSE) {
        PVVV_SAFE("Send back bad open queue response");
        sendStatus(currentRequest);

        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                   queue,
                                   bmqimp::QueueState::e_CLOSED);

        return;  // RETURN
    }

    // late open queue response
    if (requestType == e_REQ_OPEN_QUEUE &&
        errorResult == e_ERR_LATE_RESPONSE) {
        // Emulate request timeout.
        advanceTime(timeout);

        PVVV_SAFE("Open queue request should expire");
        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                                   queue,
                                   bmqimp::QueueState::e_OPENING_OPN_EXPIRED);

        PVVV_SAFE("Send back late open queue response");
        sendResponse(currentRequest);

        return;  // RETURN
    }

    // configure request is not sent
    if (requestType == e_REQ_CONFIG_QUEUE && errorResult == e_ERR_NOT_SENT) {
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR);
        PVVV_SAFE("Set channel write status to "
                  << status << " to reject open configure request");
        d_testChannel.setWriteStatus(status);
    }

    // valid open queue response
    PVVV_SAFE("Send back open queue response");
    sendResponse(currentRequest);

    // Verify configure request (todo: check stream parameters and qid)
    PVVV_SAFE("Ensure configure request has been sent");
    currentRequest = getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // configure request should fail to send but should be buffered
    if (requestType == e_REQ_CONFIG_QUEUE && errorResult == e_ERR_NOT_SENT) {
        PVVV_SAFE("Configure request should fail to send but should be "
                  "buffered");

        // Emulate request timeout.
        advanceTime(timeout);
        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                                   queue,
                                   bmqimp::QueueState::e_OPENING_CFG_EXPIRED);

        // reset channel write status
        PVVV_SAFE("Reset channel write status");
        d_testChannel.setWriteStatus(bmqio::Status());

        return;  // RETURN
    }

    // bad configure response
    if (requestType == e_REQ_CONFIG_QUEUE &&
        errorResult == e_ERR_BAD_RESPONSE) {
        PVVV_SAFE("Send back bad configure response");
        sendStatus(currentRequest);

        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                   queue,
                                   bmqimp::QueueState::e_CLOSING_CLS);

        return;  // RETURN
    }

    // late configure response
    if (requestType == e_REQ_CONFIG_QUEUE &&
        errorResult == e_ERR_LATE_RESPONSE) {
        // Emulate request timeout.
        advanceTime(timeout);

        PVVV_SAFE("Configure request should expire");
        verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                                   queue,
                                   bmqimp::QueueState::e_OPENING_CFG_EXPIRED);

        PVVV_SAFE("Send back late configure response");
        sendResponse(currentRequest);

        PVVV_SAFE("Ensure configure request has been sent");
        currentRequest = getNextOutboundRequest(
            TestSession::e_REQ_CONFIG_QUEUE);

        PVVV_SAFE("Send back configure response");
        sendResponse(currentRequest);

        return;  // RETURN
    }
}

void TestSession::reopenQueue(
    bsl::shared_ptr<bmqimp::Queue> queue,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    BMQTST_ASSERT(queue);

    PVVV_SAFE("Reopening: verify first part of open request is sent");
    bmqp_ctrlmsg::ControlMessage request = getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_REOPENING_OPN);
    BMQTST_ASSERT(queue->isValid());

    PVVV_SAFE("Reopening: send open queue response");
    sendResponse(request);

    if (bmqt::QueueFlagsUtil::isReader(queue->flags())) {
        PVVV_SAFE("Reopening: verify second part of open request is sent");
        request = getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

        PVVV_SAFE("Reopening: send configure queue response");
        sendResponse(request);
    }

    PVVV_SAFE("Reopening: wait for reopen queue result");
    BMQTST_ASSERT(
        verifyOperationResult(bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                              bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(queue->isValid());
}

void TestSession::closeQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                             const bsls::TimeInterval&      timeout,
                             bool                           isFinal)
{
    BMQTST_ASSERT(queue);
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(queue->isValid());

    bmqp_ctrlmsg::ControlMessage request(d_allocator_p);

    PVVV_SAFE("Closing: Close queue async");
    int rc = session().closeQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    // Deconfigure only for reader
    if (bmqt::QueueFlagsUtil::isReader(queue->flags())) {
        PVVV_SAFE("Closing: Check configure request was sent");
        request = getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

        BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSING_CFG);
        BMQTST_ASSERT_EQ(queue->isValid(), false);

        PVVV_SAFE("Closing: Send valid configure queue response message");
        sendResponse(request);
    }

    PVVV_SAFE("Closing: Check close request was sent");
    request = verifyCloseRequestSent(isFinal);

    PVVV_SAFE("Closing: Send valid close queue response message");
    sendResponse(request);

    PVVV_SAFE("Closing: Wait close queue result");
    verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, queue);
}

void TestSession::closeQueueWithError(bsl::shared_ptr<bmqimp::Queue> queue,
                                      const RequestType         requestType,
                                      const ErrorResult         errorResult,
                                      bool                      isFinal,
                                      const bsls::TimeInterval& timeout)
{
    BMQTST_ASSERT(queue);
    BMQTST_ASSERT(queue->isValid());
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);

    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    // configure queue request is not sent
    if (requestType == e_REQ_CONFIG_QUEUE && errorResult == e_ERR_NOT_SENT) {
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR);
        PVVV_SAFE(L_ << " Set channel write status to " << status
                     << " to reject configure queue request");
        d_testChannel.setWriteStatus(status);
    }

    PVVV_SAFE(L_ << " Close the queue async");
    int rc = d_brokerSession.closeQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_SUCCESS);

    // Verify configure request (todo: check stream parameters and qid)
    PVVV_SAFE(L_ << " Ensure configure queue request has been sent");
    currentRequest = getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // configure queue request should fail to send
    if (requestType == e_REQ_CONFIG_QUEUE && errorResult == e_ERR_NOT_SENT) {
        PVVV_SAFE(L_ << " Configure queue request should fail to send");
        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED,
                               queue,
                               bmqimp::QueueState::e_CLOSING_CFG_EXPIRED);

        // reset channel write status
        PVVV_SAFE(L_ << " Reset channel write status");
        d_testChannel.setWriteStatus(bmqio::Status());

        PVVV_SAFE(L_ << " Verify there is no close queue request");
        BMQTST_ASSERT(isChannelEmpty());

        return;  // RETURN
    }

    // bad configure response
    if (requestType == e_REQ_CONFIG_QUEUE &&
        errorResult == e_ERR_BAD_RESPONSE) {
        PVVV_SAFE(L_ << " Send back bad configure response");
        sendStatus(currentRequest);

        // Bad configure response triggers close request which fails due to
        // timeout.
        PVVV_SAFE(L_ << " Verify close queue request");
        verifyCloseRequestSent(isFinal);

        // Emulate request timeout.
        advanceTime(timeout);

        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                               queue,
                               bmqimp::QueueState::e_CLOSING_CLS_EXPIRED);

        return;  // RETURN
    }

    // late configure response
    if (requestType == e_REQ_CONFIG_QUEUE &&
        errorResult == e_ERR_LATE_RESPONSE) {
        // Emulate request timeout.
        advanceTime(timeout);

        PVVV_SAFE(L_ << " Configure request should expire");
        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                               queue,
                               bmqimp::QueueState::e_CLOSING_CFG_EXPIRED);

        PVVV_SAFE(L_ << " Verify close queue request is not sent");
        BMQTST_ASSERT(isChannelEmpty());

        return;  // RETURN
    }

    // close request is not sent
    if (requestType == e_REQ_CLOSE_QUEUE && errorResult == e_ERR_NOT_SENT) {
        bmqio::Status status(bmqio::StatusCategory::e_GENERIC_ERROR);
        PVVV_SAFE(L_ << " Set channel write status to " << status
                     << " to reject close request");
        d_testChannel.setWriteStatus(status);
    }

    // valid configure queue response
    PVVV_SAFE(L_ << " Send back configure queue response");
    sendResponse(currentRequest);

    // Verify close request
    PVVV_SAFE(L_ << " Verify close request");
    currentRequest = verifyCloseRequestSent(isFinal);

    // close request should fail to send
    if (requestType == e_REQ_CLOSE_QUEUE && errorResult == e_ERR_NOT_SENT) {
        PVVV_SAFE(L_ << " Close request should fail to send");
        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED,
                               queue,
                               bmqimp::QueueState::e_CLOSING_CLS_EXPIRED);

        // reset channel write status
        PVVV_SAFE(L_ << " Reset channel write status");
        d_testChannel.setWriteStatus(bmqio::Status());

        return;  // RETURN
    }

    // bad close queue response
    if (requestType == e_REQ_CLOSE_QUEUE &&
        errorResult == e_ERR_BAD_RESPONSE) {
        PVVV_SAFE(L_ << " Send back bad close queue response");
        sendStatus(currentRequest);

        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                               queue,
                               bmqimp::QueueState::e_CLOSED);

        return;  // RETURN
    }

    // late close response
    if (requestType == e_REQ_CLOSE_QUEUE &&
        errorResult == e_ERR_LATE_RESPONSE) {
        // Emulate request timeout.
        advanceTime(timeout);

        PVVV_SAFE(L_ << " Close request should expire");
        verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_TIMEOUT,
                               queue,
                               bmqimp::QueueState::e_CLOSING_CLS_EXPIRED);

        PVVV_SAFE(L_ << " Send back late close queue response");
        sendResponse(currentRequest);

        return;  // RETURN
    }
}

void TestSession::closeDeconfiguredQueue(bsl::shared_ptr<bmqimp::Queue> queue,
                                         const bsls::TimeInterval& timeout)
{
    BMQTST_ASSERT(queue);
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSING_CLS);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVVV_SAFE("ClosingDcfg: Close queue async");
    int rc = session().closeQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVVV_SAFE("ClosingDcfg: Check close request was sent");
    bmqp_ctrlmsg::ControlMessage request = getNextOutboundRequest(
        TestSession::e_REQ_CLOSE_QUEUE);

    PVVV_SAFE("ClosingDcfg: Send valid close queue response message");
    sendResponse(request);

    PVVV_SAFE("ClosingDcfg: Wait close queue result");
    verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, queue);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(queue->isValid(), false);
}

void TestSession::configureQueueSync(bsl::shared_ptr<bmqimp::Queue>& queue,
                                     const bmqt::QueueOptions&       options,
                                     const bsls::TimeInterval&       timeout)
{
    BMQTST_ASSERT(queue);
    session().configureQueueSync(
        queue,
        options,
        timeout,
        bdlf::BindUtil::bind(&sessionEventHandler,
                             &d_eventQueue,
                             bdlf::PlaceHolders::_1));  // event
    // This uses 'd_eventQueue' for 'configureQueueSync' result
}

bmqp_ctrlmsg::ControlMessage
TestSession::openQueueFirstStep(bsl::shared_ptr<bmqimp::Queue> queue,
                                const bsls::TimeInterval&      timeout)
{
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    PVVV_SAFE(L_ << " Open the queue async");
    int rc = d_brokerSession.openQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    // Verify open request (TODO: check handle queue parameters)
    PVVV_SAFE(L_ << "Ensure open queue request has been sent");
    currentRequest = getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::openQueueFirstStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                                       const bsls::TimeInterval&      timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    // Start with a queue in OPENING state and pending open request
    currentRequest = arriveAtStep(queue, e_OPEN_OPENING, timeout);

    BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());

    // Emulate request timeout.
    advanceTime(timeout);

    PVVV_SAFE(L_ << " Verify open request timed out");

    BMQTST_ASSERT(
        verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                              bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    PVVV_SAFE(L_ << " Send back late open queue response");
    sendResponse(currentRequest);

    PVVV_SAFE(L_ << " Ensure close queue request is sent");
    currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSING_CLS);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::reopenQueueFirstStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                                         const bsls::TimeInterval& timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    // Go to the state with pending reopen request
    currentRequest = arriveAtStep(queue, e_REOPEN_OPENING, timeout);

    // Emulate request timeout.
    advanceTime(timeout);

    PVVV_SAFE(L_ << " Verify reopen request timed out");
    BMQTST_ASSERT(
        verifyOperationResult(bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                              bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT_EQ(queue->state(),
                     bmqimp::QueueState::e_OPENING_OPN_EXPIRED);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    // We assume there is only one queue under the test and it has failed to
    // reopen.  That means the session should emit STATE_RESTORED event because
    // there are no more queues for reopening.
    BMQTST_ASSERT(waitStateRestoredEvent());

    PVVV_SAFE(L_ << " Send back late reopen queue response");
    sendResponse(currentRequest);

    PVVV_SAFE(L_ << " Ensure close queue request is sent");
    currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSING_CLS);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::reopenQueueFirstStep(bsl::shared_ptr<bmqimp::Queue> queue)
{
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(queue->isValid());

    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    PVVV_SAFE(L_ << " Trigger channel drop");
    session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(waitConnectionLostEvent());

    BMQTST_ASSERT(waitForQueueState(queue, bmqimp::QueueState::e_PENDING));
    BMQTST_ASSERT(queue->isValid());

    PVVV_SAFE(L_ << " Restore the connection");
    setChannel();

    BMQTST_ASSERT(waitReconnectedEvent());

    PVVV_SAFE(L_ << " Check open part of open request is sent");
    currentRequest = getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_REOPENING_OPN);
    BMQTST_ASSERT(queue->isValid());

    return currentRequest;
}

void TestSession::closeQueueSecondStep(
    bsl::shared_ptr<bmqimp::Queue>      queue,
    const bmqp_ctrlmsg::ControlMessage& closeRequest,
    bool                                waitCloseEvent)
{
    BMQTST_ASSERT(closeRequest.choice().isCloseQueueValue());
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_CLOSING_CLS);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVVV_SAFE(L_ << " Send valid close queue response");
    sendResponse(closeRequest);

    if (waitCloseEvent) {
        PVVV_SAFE(L_ << " Wait for close queue result");
        BMQTST_ASSERT(
            verifyOperationResult(bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    }
    BMQTST_ASSERT(waitForQueueState(queue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(queue->isValid(), false);
}

bmqp_ctrlmsg::ControlMessage
TestSession::closeQueueSecondStepExpired(bsl::shared_ptr<bmqimp::Queue> queue,
                                         const bsls::TimeInterval& timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());

    // Go to the step with pending 2nd phase close request
    currentRequest = arriveAtStep(queue, e_CLOSE_CLOSING, timeout);

    BMQTST_ASSERT(currentRequest.choice().isCloseQueueValue());

    // Emulate request timeout.
    advanceTime(timeout);

    PVVV_SAFE(L_ << " Verify close request timed out");

    BMQTST_ASSERT(
        verifyOperationResult(bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                              bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT_EQ(queue->state(),
                     bmqimp::QueueState::e_CLOSING_CLS_EXPIRED);
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVVV_SAFE(L_ << " Send back late close queue response");
    sendResponse(currentRequest);

    BMQTST_ASSERT(waitForQueueState(queue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    // Late close response has arrived, and there should be no more close
    // requests
    PVVV_SAFE(L_ << " Ensure no more requests are sent");
    BMQTST_ASSERT(isChannelEmpty());

    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::arriveAtStepWithCfgs(bsl::shared_ptr<bmqimp::Queue> queue,
                                  const QueueTestStep            step,
                                  const bsls::TimeInterval&      timeout)
{
    BSLS_ASSERT_OPT(bmqt::QueueFlagsUtil::isReader(queue->flags()));

    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    int                          currentStep = e_OPEN_OPENING;

    while (currentStep <= step) {
        switch (currentStep) {
        case e_OPEN_OPENING: {
            currentRequest = openQueueFirstStep(queue, timeout);
        } break;
        case e_OPEN_CONFIGURING: {
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());

            PVVV_SAFE("arriveAtStep: Send back open queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Ensure configure queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_OPENING_CFG);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_OPEN_OPENED: {
            PVVV_SAFE("arriveAtStep: Send back configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Waiting QUEUE_OPEN_RESULT event...");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_REOPEN_OPENING: {
            currentRequest = reopenQueueFirstStep(queue);
        } break;
        case e_REOPEN_CONFIGURING: {
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());

            PVVV_SAFE("arriveAtStep: Send back open queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Ensure configure queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_REOPENING_CFG);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_REOPEN_REOPENED: {
            PVVV_SAFE("arriveAtStep: Send back configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Waiting QUEUE_REOPEN_RESULT event...");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

            BMQTST_ASSERT(waitStateRestoredEvent());

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURING: {
            PVVV_SAFE("arriveAtStep: Standalone configure queue async");
            int rc = session().configureQueueAsync(queue,
                                                   queue->options(),
                                                   timeout);
            BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

            PVVV_SAFE("arriveAtStep: Check configure request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURING_RECONFIGURING: {
            PVVV_SAFE("arriveAtStep: Verify configure request timed out");

            // Emulate request timeout.
            advanceTime(timeout);

            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

            PVVV_SAFE("arriveAtStep: Send back late configure response");

            bmqp_ctrlmsg::ConsumerInfo ci;
            getConsumerInfo(&ci, currentRequest);

            // Modify queue options to trigger reconfigure request
            bmqt::QueueOptions options = queue->options();

            options.setMaxUnconfirmedBytes(options.maxUnconfirmedBytes() + 1);
            queue->setOptions(options);

            sendResponse(currentRequest);

            PVV_SAFE("arriveAtStep: Check reconfigure request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURED: {
            PVVV_SAFE("arriveAtStep: Send back configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: expect no queue event for reconfigure");
            BMQTST_ASSERT(checkNoEvent());

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CLOSE_CONFIGURING: {
            PVVV_SAFE("arriveAtStep: Close queue async");
            int rc = session().closeQueueAsync(queue, timeout);
            BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

            PVVV_SAFE("arriveAtStep: Check configure request was sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CFG);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_CLOSE_CLOSING: {
            PVVV_SAFE("arriveAtStep: Send valid configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Check close request was sent");
            currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_CLOSE_CLOSED: {
            // waitCloseEvent = true
            closeQueueSecondStep(queue, currentRequest, true);
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unreachable by design");
        } break;
        }

        ++currentStep;
    }
    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::arriveAtStepWithoutCfgs(bsl::shared_ptr<bmqimp::Queue> queue,
                                     const QueueTestStep            step,
                                     const bsls::TimeInterval&      timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    int                          currentStep = e_OPEN_OPENING;

    while (currentStep <= step) {
        switch (currentStep) {
        case e_OPEN_OPENING: {
            currentRequest = openQueueFirstStep(queue, timeout);
        } break;
        case e_OPEN_CONFIGURING: {
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());

            PVVV_SAFE("arriveAtStep: Send back open queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Waiting QUEUE_OPEN_RESULT event...");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
        } break;
        case e_OPEN_OPENED: {
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_REOPEN_OPENING: {
            currentRequest = reopenQueueFirstStep(queue);
        } break;
        case e_REOPEN_CONFIGURING: {
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());

            PVVV_SAFE("arriveAtStep: Send back open queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("arriveAtStep: Waiting QUEUE_REOPEN_RESULT event...");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_REOPEN_REOPENED: {
            BMQTST_ASSERT(waitStateRestoredEvent());

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURING: {
            PVVV_SAFE("arriveAtStep: Standalone configure queue async");
            int rc = session().configureQueueAsync(queue,
                                                   queue->options(),
                                                   timeout);
            BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

            // Configure request is skipped for the writer
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURING_RECONFIGURING: {
            // No reconfigure for writer
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CONFIGURED: {
            // No reconfigure for writer
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_CLOSE_CONFIGURING: {
            PVVV_SAFE("arriveAtStep: Close queue async");
            int rc = session().closeQueueAsync(queue, timeout);
            BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

            // Deconfiguring is for reader only, writer sends close request
            PVVV_SAFE("arriveAtStep: Check close request was sent");
            currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_CLOSE_CLOSING: {
            // Nothing to do for the writer
            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_CLOSE_CLOSED: {
            // waitCloseEvent = true
            closeQueueSecondStep(queue, currentRequest, true);
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unreachable by design");
        } break;
        }

        ++currentStep;
    }

    return currentRequest;
}

bmqp_ctrlmsg::ControlMessage
TestSession::arriveAtStep(bsl::shared_ptr<bmqimp::Queue> queue,
                          const QueueTestStep            step,
                          const bsls::TimeInterval&      timeout,
                          const bool                     skipReaderCfgs)
{
    if (queue->state() == bmqimp::QueueState::e_OPENED) {
        closeQueue(queue);
    }

    if (bmqt::QueueFlagsUtil::isReader(queue->flags()) && !skipReaderCfgs) {
        return arriveAtStepWithCfgs(queue, step, timeout);  // RETURN
    }

    return arriveAtStepWithoutCfgs(queue, step, timeout);
}

void TestSession::arriveAtLateResponseStep(
    bsl::shared_ptr<bmqimp::Queue> queue,
    const LateResponseTestStep     step,
    const bsls::TimeInterval&      timeout)
{
    if (bmqt::QueueFlagsUtil::isReader(queue->flags())) {
        return arriveAtLateResponseStepReader(queue, step, timeout);  // RETURN
    }

    return arriveAtLateResponseStepWriter(queue, step, timeout);
}

void TestSession::arriveAtLateResponseStepReader(
    bsl::shared_ptr<bmqimp::Queue> queue,
    const LateResponseTestStep     step,
    const bsls::TimeInterval&      timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    int                          currentStep = e_LATE_OPEN_OPENING;

    while (currentStep <= step) {
        switch (currentStep) {
        case e_LATE_OPEN_OPENING: {
            currentRequest = openQueueFirstStepExpired(queue, timeout);
        } break;
        case e_LATE_OPEN_CONFIGURING_CFG: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Move the queue to opening state and pending configure request
            currentRequest = arriveAtStep(queue, e_OPEN_CONFIGURING, timeout);

            BMQTST_ASSERT(isConfigure(currentRequest));

            // Emulate request timeout.
            advanceTime(timeout);

            PVVV_SAFE("Step: " << currentStep
                               << " Verify open request timed out");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

            PVVV_SAFE("Step: " << currentStep
                               << " Send back late configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("Step: " << currentStep
                               << " Ensure deconfigure queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            PVVV_SAFE("Deconfigure request sent: " << currentRequest);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CFG);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_LATE_OPEN_CONFIGURING_CLS: {
            // Handle pending request from the previous step
            BMQTST_ASSERT(isConfigure(currentRequest));

            PVVV_SAFE("Step: " << currentStep
                               << " Send back deconfigure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("Step: " << currentStep
                               << " Ensure close queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_LATE_REOPEN_OPENING: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            currentRequest = reopenQueueFirstStepExpired(queue, timeout);
        } break;
        case e_LATE_REOPEN_CONFIGURING_CFG: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Go to the state with pending reopen-configure request
            currentRequest = arriveAtStep(queue,
                                          e_REOPEN_CONFIGURING,
                                          timeout);

            // Emulate request timeout.
            advanceTime(timeout);

            PVVV_SAFE("Step: " << currentStep
                               << " Verify reopen request timed out");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_OPENING_CFG_EXPIRED);
            BMQTST_ASSERT_EQ(queue->isValid(), false);

            // We assume there is only one queue under the test and it has
            // failed to reopen.  That means the session should emit
            // STATE_RESTORED event because there are no more queues for
            // reopening.
            BMQTST_ASSERT(waitStateRestoredEvent());

            PVVV_SAFE("Step: " << currentStep
                               << " Send back late configure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("Step: " << currentStep
                               << " Ensure deconfigure queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CFG);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_LATE_REOPEN_CONFIGURING_CLS: {
            // Handle pending request from the previous step
            BMQTST_ASSERT(isConfigure(currentRequest));

            PVVV_SAFE("Step: " << currentStep
                               << " Send back deconfigure queue response");
            sendResponse(currentRequest);

            // Now check pending close request
            PVVV_SAFE("Step: " << currentStep
                               << " Ensure close queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_LATE_RECONFIGURING: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Go to the step with pending reconfigure request
            currentRequest = arriveAtStep(queue,
                                          e_CONFIGURING_RECONFIGURING,
                                          timeout);

            BMQTST_ASSERT(isConfigure(currentRequest));
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_LATE_CLOSE_CONFIGURING: {
            // Cleanup pending reconfigure request from the previous step
            BMQTST_ASSERT(isConfigure(currentRequest));

            PVVV_SAFE("Step: " << currentStep
                               << " Send back reconfigure queue response");
            sendResponse(currentRequest);

            // Go to state with pending deconfigure request
            currentRequest = arriveAtStep(queue, e_CLOSE_CONFIGURING, timeout);

            BMQTST_ASSERT(isConfigure(currentRequest));

            // Emulate request timeout.
            advanceTime(timeout);

            PVVV_SAFE("Step: " << currentStep
                               << " Verify deconfigure request timed out");
            BMQTST_ASSERT(verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CFG_EXPIRED);
            BMQTST_ASSERT_EQ(queue->isValid(), false);

            // Send late deconfigure response
            PVVV_SAFE("Step: "
                      << currentStep
                      << " Send back late deconfigure queue response");
            sendResponse(currentRequest);

            PVVV_SAFE("Step: " << currentStep
                               << " Ensure close queue request is sent");
            currentRequest = getNextOutboundRequest(e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT_EQ(queue->state(),
                             bmqimp::QueueState::e_CLOSING_CLS);
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        } break;
        case e_LATE_CLOSE_CLOSING: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Go to close second step expired
            currentRequest = closeQueueSecondStepExpired(queue, timeout);
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unreachable by design");
        } break;
        }

        ++currentStep;
    }
}

void TestSession::arriveAtLateResponseStepWriter(
    bsl::shared_ptr<bmqimp::Queue> queue,
    const LateResponseTestStep     step,
    const bsls::TimeInterval&      timeout)
{
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    int                          currentStep = e_LATE_OPEN_OPENING;

    while (currentStep <= step) {
        switch (currentStep) {
        case e_LATE_OPEN_OPENING: {
            currentRequest = openQueueFirstStepExpired(queue, timeout);
        } break;
        case e_LATE_OPEN_CONFIGURING_CFG: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Move the queue to opening state and pending configure request
            currentRequest = arriveAtStep(queue, e_OPEN_CONFIGURING, timeout);

            // For writer there should be no pending configure request
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());
        } break;
        case e_LATE_OPEN_CONFIGURING_CLS: {
            // Skip the step for the writer
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_LATE_REOPEN_OPENING: {
            currentRequest = reopenQueueFirstStepExpired(queue, timeout);
        } break;
        case e_LATE_REOPEN_CONFIGURING_CFG: {
            // Handle pending request from the previous step
            // waitCloseEvent = false
            closeQueueSecondStep(queue, currentRequest, false);

            // Go to the state with pending reopen-configure request
            currentRequest = arriveAtStep(queue,
                                          e_REOPEN_CONFIGURING,
                                          timeout);

            // No reopen-configure for the writer
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());
            BMQTST_ASSERT(waitStateRestoredEvent());
        } break;
        case e_LATE_REOPEN_CONFIGURING_CLS: {
            // No reopen-configure for the writer
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_LATE_RECONFIGURING: {
            // No reconfigure for the writer
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_LATE_CLOSE_CONFIGURING: {
            // No configuring for the writer
            BMQTST_ASSERT(currentRequest.choice().isOpenQueueValue());
            BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
            BMQTST_ASSERT(queue->isValid());
        } break;
        case e_LATE_CLOSE_CLOSING: {
            // Go to the step with pending 2nd phase close request
            currentRequest = closeQueueSecondStepExpired(queue, timeout);
        } break;
        default: {
            BSLS_ASSERT_OPT(false && "Unreachable by design");
        } break;
        }
        ++currentStep;
    }
}

bsl::shared_ptr<bmqimp::Queue>
TestSession::createQueue(const char*               name,
                         bsls::Types::Uint64       flags,
                         const bmqt::QueueOptions& options)
{
    BMQTST_ASSERT(name);

    bmqt::Uri uri(name, bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqimp::Queue> pQueue;

    pQueue.createInplace(bmqtst::TestHelperUtil::allocator(),
                         bmqtst::TestHelperUtil::allocator());
    pQueue->setUri(uri);
    pQueue->setFlags(flags);
    pQueue->setOptions(options);
    pQueue->setCorrelationId(bmqt::CorrelationId::autoValue());

    return pQueue;
}

bsl::shared_ptr<bmqimp::Queue>
TestSession::createQueueOnStep(const QueueTestStep       step,
                               bsls::Types::Uint64       queueFlags,
                               const bsls::TimeInterval& timeout)
{
    bmqt::QueueOptions queueOptions;

    bsl::shared_ptr<bmqimp::Queue> pQueue = createQueue(k_URI,
                                                        queueFlags,
                                                        queueOptions);
    arriveAtStep(pQueue, step, timeout);

    return pQueue;
}

bsl::shared_ptr<bmqimp::Queue>
TestSession::createQueueOnStep(const LateResponseTestStep step,
                               bsls::Types::Uint64        queueFlags,
                               const bsls::TimeInterval&  timeout)
{
    bmqt::QueueOptions queueOptions;

    bsl::shared_ptr<bmqimp::Queue> pQueue = createQueue(k_URI,
                                                        queueFlags,
                                                        queueOptions);
    arriveAtLateResponseStep(pQueue, step, timeout);

    return pQueue;
}

bool TestSession::waitForChannelClose(const bsls::TimeInterval& timeout)
{
    PVVV_SAFE("Waiting for channel close");

    // Wait for the close to be called on the base channel
    const bsls::TimeInterval expireAfter =
        bsls::SystemTime::nowRealtimeClock() + timeout;
    while (d_testChannel.closeCallsEmpty() &&
           bsls::SystemTime::nowRealtimeClock() < expireAfter) {
        bslmt::ThreadUtil::microSleep(k_TIME_SOURCE_STEP.totalMicroseconds());
    }

    if (d_testChannel.closeCallsEmpty()) {
        return false;  // RETURN
    }

    d_testChannel.popCloseCall();

    // Invoke the close signaler
    bmqio::Status status;
    onChannelClose(status);

    return true;
}

bool TestSession::waitForQueueState(
    const bsl::shared_ptr<bmqimp::Queue>& queue,
    const bmqimp::QueueState::Enum        state,
    const bsls::TimeInterval&             timeout)

{
    PVVV_SAFE("Waiting the queue in state " << state);

    const bsls::TimeInterval expireAfter =
        bsls::SystemTime::nowRealtimeClock() + timeout;
    while (queue->state() != state &&
           bsls::SystemTime::nowRealtimeClock() < expireAfter) {
        bslmt::ThreadUtil::microSleep(k_TIME_SOURCE_STEP.totalMicroseconds());
    }

    return queue->state() == state;
}

bool TestSession::waitForQueueRemoved(
    const bsl::shared_ptr<bmqimp::Queue>& queue,
    const bsls::TimeInterval&             timeout)

{
    PVVV_SAFE("Waiting the queue is closed and removed");

    const bsls::TimeInterval expireAfter =
        bsls::SystemTime::nowRealtimeClock() + timeout;
    while (session().lookupQueue(queue->uri()) != 0 &&
           bsls::SystemTime::nowRealtimeClock() < expireAfter) {
        bslmt::ThreadUtil::microSleep(k_TIME_SOURCE_STEP.totalMicroseconds());
    }

    return (queue->state() == bmqimp::QueueState::e_CLOSED) &&
           (session().lookupQueue(queue->uri()) == 0);
}

void TestSession::stopGracefully(bool waitForDisconnected)
{
    PVVV_SAFE("Stopping session...");
    session().stopAsync();

    PVVV_SAFE("Stopping: Verify disconnect request is sent");
    bmqp_ctrlmsg::ControlMessage disconnectMessage(
        bmqtst::TestHelperUtil::allocator());
    getOutboundControlMessage(&disconnectMessage);

    BMQTST_ASSERT(!disconnectMessage.rId().isNull());
    BMQTST_ASSERT(disconnectMessage.choice().isDisconnectValue());

    PVVV_SAFE("Stopping: Prepare and send disconnect response message");
    bmqp_ctrlmsg::ControlMessage disconnectResponseMessage(
        bmqtst::TestHelperUtil::allocator());
    disconnectResponseMessage.rId().makeValue(disconnectMessage.rId().value());
    disconnectResponseMessage.choice().makeDisconnectResponse();

    sendControlMessage(disconnectResponseMessage);

    if (waitForDisconnected) {
        BMQTST_ASSERT(waitForChannelClose());

        PVVV_SAFE("Stopping: Waiting DISCONNECTED event");
        BMQTST_ASSERT(waitDisconnectedEvent());
        PVVV_SAFE("Stopping: Waiting session stop CB");
        BMQTST_ASSERT(verifySessionIsStopped());
    }

    PVVV_SAFE("Session stopped...");
}

void TestSession::setChannel()
{
    d_brokerSession.setChannel(
        bsl::shared_ptr<bmqio::Channel>(&d_testChannel,
                                        bslstl::SharedPtrNilDeleter()));
}

bool TestSession::waitConnectedEvent()
{
    PVVV_SAFE("Waiting CONNECTED event...");
    return verifyOperationResult(bmqt::SessionEventType::e_CONNECTED,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitReconnectedEvent()
{
    PVVV_SAFE("Waiting RECONNECTED event...");
    return verifyOperationResult(bmqt::SessionEventType::e_RECONNECTED,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitStateRestoredEvent()
{
    PVVV_SAFE("Waiting STATE_RESTORED event...");
    return verifyOperationResult(bmqt::SessionEventType::e_STATE_RESTORED,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitDisconnectedEvent()
{
    PVVV_SAFE("Waiting DISCONNECTED event...");

    return verifyOperationResult(bmqt::SessionEventType::e_DISCONNECTED,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitConnectionLostEvent()
{
    PVVV_SAFE("Waiting CONNECTION_LOST event...");
    return verifyOperationResult(bmqt::SessionEventType::e_CONNECTION_LOST,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitHostUnhealthyEvent()
{
    PVVV_SAFE("Waiting HOST_UNHEALTHY event...");
    return verifyOperationResult(bmqt::SessionEventType::e_HOST_UNHEALTHY,
                                 bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bool TestSession::waitQueueSuspendedEvent(bmqt::GenericResult::Enum status)
{
    PVVV_SAFE("Waiting QUEUE_SUSPENDED event...");
    return verifyOperationResult(bmqt::SessionEventType::e_QUEUE_SUSPENDED,
                                 status);
}

bool TestSession::waitQueueResumedEvent(bmqt::GenericResult::Enum status)
{
    PVVV_SAFE("Waiting QUEUE_RESUMED event...");
    return verifyOperationResult(bmqt::SessionEventType::e_QUEUE_RESUMED,
                                 status);
}

bool TestSession::waitHostHealthRestoredEvent()
{
    PVVV_SAFE("Waiting HOST_HEALTH_RESTORED event...");
    return verifyOperationResult(
        bmqt::SessionEventType::e_HOST_HEALTH_RESTORED,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
}

bsl::shared_ptr<bmqimp::Event> TestSession::waitAckEvent()
{
    PVVV_SAFE("Waiting ACK event...");
    bsl::shared_ptr<bmqimp::Event> event = getInboundEvent();
    if (!event) {
        PV_SAFE("No ACK event");
        return event;  // RETURN
    }
    if (!event->rawEvent().isAckEvent()) {
        PV_SAFE("Unexpected event: " << *event);
        event.reset();
    }

    return event;
}

bool TestSession::checkNoEvent()
{
    bsl::shared_ptr<bmqimp::Event> event;
    if (d_eventQueue.tryPopFront(&event) == 0) {
        PV_SAFE("Unexpected event: " << *event);
    }

    return event == 0;
}

bool TestSession::isChannelEmpty()
{
    return !d_testChannel.waitFor(1, true, k_TIME_SOURCE_STEP);
}

void TestSession::getOutboundEvent(bmqp::Event* rawEvent)
{
    BMQTST_ASSERT(d_testChannel.waitFor(1, true, bsls::TimeInterval(1)));

    bmqio::TestChannel::WriteCall wc = d_testChannel.popWriteCall();
    bmqp::Event ev(&wc.d_blob, bmqtst::TestHelperUtil::allocator(), true);

    *rawEvent = ev;
}

void TestSession::getOutboundControlMessage(
    bmqp_ctrlmsg::ControlMessage* outMsg)
{
    BMQTST_ASSERT(d_testChannel.waitFor(1, true, k_EVENT_TIMEOUT));

    bmqio::TestChannel::WriteCall wc = d_testChannel.popWriteCall();
    bmqp::Event ev(&wc.d_blob, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(ev.isControlEvent());

    int rc = ev.loadControlEvent(outMsg);
    BMQTST_ASSERT_EQ(0, rc);
}

void TestSession::sendControlMessage(
    const bmqp_ctrlmsg::ControlMessage& message)
{
    bmqp::SchemaEventBuilder builder(&d_blobSpPool,
                                     bmqp::EncodingType::e_BER,
                                     d_allocator_p);
    int rc = builder.setMessage(message, bmqp::EventType::e_CONTROL);
    BMQTST_ASSERT_EQ(0, rc);

    const bdlbb::Blob& packet = *builder.blob();

    PVVV_SAFE("Send control message");
    d_brokerSession.processPacket(packet);
}

void TestSession::sendResponse(const bmqp_ctrlmsg::ControlMessage& request)
{
    bmqp_ctrlmsg::ControlMessage responseMessage(
        bmqtst::TestHelperUtil::allocator());
    responseMessage.rId() = request.rId();

    if (request.choice().isOpenQueueValue()) {
        responseMessage.choice().makeOpenQueueResponse().originalRequest() =
            request.choice().openQueue();
    }
    else if (isConfigure(request)) {
        makeResponse(&responseMessage, request);
    }
    else if (request.choice().isCloseQueueValue()) {
        responseMessage.choice().makeCloseQueueResponse();
    }

    sendControlMessage(responseMessage);
}

void TestSession::sendStatus(const bmqp_ctrlmsg::ControlMessage& request)
{
    bmqp_ctrlmsg::ControlMessage statusMessage(
        bmqtst::TestHelperUtil::allocator());
    statusMessage.rId() = request.rId();
    statusMessage.choice().makeStatus();
    statusMessage.choice().status().category() =
        bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    statusMessage.choice().status().code()    = -1;
    statusMessage.choice().status().message() = "unknown status";

    sendControlMessage(statusMessage);
}

bmqp_ctrlmsg::ControlMessage
TestSession::getNextOutboundRequest(const RequestType requestType)
{
    bmqp_ctrlmsg::ControlMessage controlMessage(
        bmqtst::TestHelperUtil::allocator());
    getOutboundControlMessage(&controlMessage);

    PVVV_SAFE("Outbound request: " << controlMessage);

    BMQTST_ASSERT(!controlMessage.rId().isNull());

    switch (requestType) {
    case e_REQ_OPEN_QUEUE: {
        BMQTST_ASSERT(controlMessage.choice().isOpenQueueValue());
    } break;
    case e_REQ_CONFIG_QUEUE: {
        BMQTST_ASSERT(isConfigure(controlMessage));
    } break;
    case e_REQ_CLOSE_QUEUE: {
        BMQTST_ASSERT(controlMessage.choice().isCloseQueueValue());
    } break;
    case e_REQ_DISCONNECT: {
        BMQTST_ASSERT(controlMessage.choice().isDisconnectValue());
    } break;
    case e_REQ_UNDEFINED:
    default: {
        BSLS_ASSERT_OPT(false && "Unreachable by design");
    }
    }
    return controlMessage;
}

int TestSession::verifyRequestSent(const RequestType requestType)
{
    return getNextOutboundRequest(requestType).rId().value();
}

bmqp_ctrlmsg::ControlMessage TestSession::verifyCloseRequestSent(bool isFinal)
{
    bmqp_ctrlmsg::ControlMessage request = getNextOutboundRequest(
        e_REQ_CLOSE_QUEUE);

    BMQTST_ASSERT_EQ(request.choice().closeQueue().isFinal(), isFinal);

    return request;
}

bool TestSession::verifyOperationResult(
    const bmqt::SessionEventType::Enum eventType,
    int                                eventStatus)
{
    bsl::shared_ptr<bmqimp::Event> event = getInboundEvent();

    if (!event) {
        PV_SAFE("No session event");
        return false;  // RETURN
    }
    else if (event->sessionEventType() != eventType) {
        PV_SAFE("Unexpected event type: " << event->sessionEventType());
        return false;  // RETURN
    }
    else if (event->statusCode() != eventStatus) {
        PV_SAFE("Unexpected event status: " << event->statusCode()
                                            << " "
                                               "Expected: "
                                            << eventStatus);
        return false;  // RETURN
    }

    return true;
}

void TestSession::verifyOpenQueueErrorResult(
    const bmqp_ctrlmsg::StatusCategory::Value eventStatus,
    const bsl::shared_ptr<bmqimp::Queue>&     queue,
    const bmqimp::QueueState::Enum            queueState)
{
    BMQTST_ASSERT(queue);

    BMQTST_ASSERT_EQ(queue->isValid(), false);

    BMQTST_ASSERT(
        verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                              eventStatus));

    BMQTST_ASSERT(waitForQueueState(queue, queueState));

    if (queueState == bmqimp::QueueState::e_CLOSED) {
        // Verify that the queue is removed from the active queue list
        BMQTST_ASSERT(waitForQueueRemoved(queue));
    }
}

void TestSession::onChannelClose(const bmqio::Status& status)
{
    PVVV_SAFE("onChannelClose: [" << status << "] Resetting the channel");
    session().setChannel(bsl::shared_ptr<bmqio::Channel>());
}

bool TestSession::verifySessionIsStopped()
{
    PVVV_SAFE("Verifying that the session is stopped");

    return d_brokerSession.state() == bmqimp::BrokerSession::State::e_STOPPED;
}

int TestSession::stateCb(bmqimp::BrokerSession::State::Enum    oldState,
                         bmqimp::BrokerSession::State::Enum    newState,
                         bmqimp::BrokerSession::FsmEvent::Enum event)
{
    PVVV_SAFE("stateCb: " << oldState << " -> " << newState << " on "
                          << event);

    if (newState == bmqimp::BrokerSession::State::e_STARTING) {
        ++d_startCounter;
    }
    else if (newState == bmqimp::BrokerSession::State::e_STOPPED) {
        ++d_stopCounter;
    }

    return 0;
}

void TestSession::verifyCloseQueueResult(
    const bmqp_ctrlmsg::StatusCategory::Value status,
    const bsl::shared_ptr<bmqimp::Queue>&     queue,
    const bmqimp::QueueState::Enum            state)
{
    bsl::shared_ptr<bmqimp::Event> queueCloseEvent = getInboundEvent();

    BMQTST_ASSERT_EQ(queueCloseEvent->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT);

    BMQTST_ASSERT_EQ(queueCloseEvent->statusCode(), status);

    BMQTST_ASSERT(waitForQueueState(queue, state));

    if (state == bmqimp::QueueState::e_CLOSED) {
        // Verify that the queue is removed from the active queue list
        BMQTST_ASSERT(waitForQueueRemoved(queue));
    }
    BMQTST_ASSERT_EQ(queue->isValid(), false);
}

void TestSession::setChannelLowWaterMark(
    bmqio::StatusCategory::Enum writeStatus)
{
    channel().setWriteStatus(writeStatus);

    // Notify BrokerSession
    session().handleChannelWatermark(
        bmqio::ChannelWatermarkType::e_LOW_WATERMARK);
}

static void test_disconnectRequestErr(bmqio::StatusCategory::Enum category)
{
    bmqt::SessionOptions  sessionOptions;
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    sessionOptions.setNumProcessingThreads(1).setDisconnectTimeout(
        bsls::TimeInterval(0.01));

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    obj.startAndConnect();

    // set channel write status
    bmqio::Status status(category);
    PVV_SAFE("Set channel write status to " << status);
    obj.channel().setWriteStatus(status);

    PVV_SAFE("Stopping session...");
    obj.session().stopAsync();

    // verify disconnect message
    obj.verifyRequestSent(TestSession::e_REQ_DISCONNECT);

    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitDisconnectedEvent());

    BMQTST_ASSERT(obj.verifySessionIsStopped());
}

struct AsyncCancelTestData {
    int                          d_line;
    TestSession::QueueTestStep   d_queueTestStep;
    bsls::Types::Uint64          d_queueFlags;
    bmqt::SessionEventType::Enum d_eventType;
    TestSession::RequestType     d_retransmittedRequest;
    bool                         d_waitOperationResultOnChannelDown;
    bool                         d_waitOperationResultOnDisconnect;
    bool                         d_waitStateRestoredOnChannelDown;
    bool                         d_waitStateRestoredOnDisconnect;
    bmqimp::QueueState::Enum     d_queueStateAfterDisconnect;
};

// ============================================================================
//                        DISTRIBUTED TRACE UTILITIES
// ----------------------------------------------------------------------------

class DTTestContext : public bmqpi::DTContext {
    mutable bslmt::Mutex           d_mutex;
    bsl::shared_ptr<bmqpi::DTSpan> d_currentSpan_sp;
    bslma::Allocator*              d_allocator_p;

    class ScopeToken {
        DTTestContext&               d_context;
        bsl::weak_ptr<bmqpi::DTSpan> d_previousSpan_wp;

      public:
        ScopeToken(DTTestContext&                 context,
                   bsl::shared_ptr<bmqpi::DTSpan> previousSpan,
                   bsl::shared_ptr<bmqpi::DTSpan> newSpan)
        : d_context(context)
        , d_previousSpan_wp(previousSpan)
        {
            d_context.d_currentSpan_sp = newSpan;
        }

        ~ScopeToken()
        {
            bslmt::LockGuard<bslmt::Mutex> guard(&d_context.d_mutex);
            d_context.d_currentSpan_sp = d_previousSpan_wp.lock();
        }
    };

  public:
    DTTestContext(bslma::Allocator* allocator_p)
    : DTContext()
    , d_mutex()
    , d_currentSpan_sp(NULL)
    , d_allocator_p(bslma::Default::allocator(allocator_p))
    {
        // NOTHING
    }

    bsl::shared_ptr<bmqpi::DTSpan> span() const BSLS_KEYWORD_OVERRIDE
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
        return d_currentSpan_sp;
    }

    bslma::ManagedPtr<void>
    scope(const bsl::shared_ptr<bmqpi::DTSpan>& newSpan) BSLS_KEYWORD_OVERRIDE
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);
        bslma::ManagedPtr<void>        result(
            new (*d_allocator_p) ScopeToken(*this, d_currentSpan_sp, newSpan),
            d_allocator_p);
        return result;
    }
};

class DTTestSpan : public bmqpi::DTSpan {
    bsl::string                d_operation;
    bdlcc::Deque<bsl::string>* d_eventsQueue_p;
    bslma::Allocator*          d_allocator_p;

  public:
    DTTestSpan(bsl::string_view              operation,
               const bmqpi::DTSpan::Baggage& baggage,
               bdlcc::Deque<bsl::string>*    eventsQueue_p,
               bslma::Allocator*             allocator_p)
    : DTSpan()
    , d_operation(operation, allocator_p)
    , d_eventsQueue_p(eventsQueue_p)
    , d_allocator_p(bslma::Default::allocator(allocator_p))
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(d_eventsQueue_p);

        bmqu::MemOutStream event(d_allocator_p);
        event << "START " << d_operation;

        bmqpi::DTSpan::Baggage::const_iterator it = baggage.begin();
        for (; it != baggage.end(); ++it) {
            event << "; " << it->first << "=" << it->second;
        }

        d_eventsQueue_p->pushBack(event.str());
    }

    ~DTTestSpan() BSLS_KEYWORD_OVERRIDE
    {
        bmqu::MemOutStream event(d_allocator_p);
        event << "END " << d_operation;

        d_eventsQueue_p->pushBack(event.str());
    }

    bsl::string_view operation() const BSLS_KEYWORD_OVERRIDE
    {
        return d_operation;
    }
};

class DTTestTracer : public bmqpi::DTTracer {
    bdlcc::Deque<bsl::string>* d_eventsQueue_p;
    bslma::Allocator*          d_allocator_p;

  public:
    DTTestTracer(bdlcc::Deque<bsl::string>* eventsQueue_p,
                 bslma::Allocator*          allocator_p)
    : DTTracer()
    , d_eventsQueue_p(eventsQueue_p)
    , d_allocator_p(bslma::Default::allocator(allocator_p))
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(d_eventsQueue_p);
    }

    bsl::shared_ptr<bmqpi::DTSpan> createChildSpan(
        const bsl::shared_ptr<bmqpi::DTSpan>& parent,
        const bsl::string_view&               operation,
        const bmqpi::DTSpan::Baggage& baggage) const BSLS_KEYWORD_OVERRIDE
    {
        bmqu::MemOutStream operationName(d_allocator_p);
        operationName << operation;
        if (parent) {
            operationName << " < " << parent->operation();
        }

        bsl::shared_ptr<bmqpi::DTSpan> result(
            new (*d_allocator_p) DTTestSpan(operationName.str(),
                                            baggage,
                                            d_eventsQueue_p,
                                            d_allocator_p),
            d_allocator_p);
        return result;
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    scheduler.start();

    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    sessionOptions.setNumProcessingThreads(1);

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bsls::AtomicInt                          stopCounter(0);
    bsls::AtomicInt                          startCounter(0);

    bmqimp::BrokerSession session(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        emptyEventHandler,  // emptyEventHandler,
        bdlf::BindUtil::bind(&stateCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &startCounter,
                             &stopCounter),
        bmqtst::TestHelperUtil::allocator());
    PVV_SAFE("Starting session...");
    int rc = session.startAsync();
    BMQTST_ASSERT_EQ(rc, 0);
    // Session changes its state synchronously.
    BMQTST_ASSERT_EQ(session.state(),
                     bmqimp::BrokerSession::State::e_STARTING);

    PVV_SAFE("Session starting...: " << rc);

    PVV_SAFE("Stopping session...");
    session.stop();

    // 'stop' is synchronous and there is no IO connection.
    PVV_SAFE("Session stopped...");

    BMQTST_ASSERT_EQ(session.state(), bmqimp::BrokerSession::State::e_STOPPED);
    BMQTST_ASSERT_EQ(startCounter, 1);
    BMQTST_ASSERT_EQ(stopCounter, 1);

    scheduler.cancelAllEventsAndWait();
    scheduler.stop();
}

static void test2_basicAccessorsTest()
// ------------------------------------------------------------------------
// BASIC ACCESSORS TEST
//
// Concerns:
//   1. Check bmqimp::BrokerSession initial state.
//
// Plan:
//   1. Create bmqimp::BrokerSession object.
//   2. Check accessors values.
//
// Testing accessors:
//   - isUsingSessionEventHandler
//   - lookupQueue
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BASIC ACCESSORS");

    bmqt::Uri uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bmqp::QueueId            k_QUEUE_ID(0, 0);
    const bmqt::CorrelationId      corrId(bmqt::CorrelationId::autoValue());
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bmqimp::BrokerSession::StateFunctor      emptyStateCb;

    bmqimp::BrokerSession obj(&scheduler,
                              &blobBufferFactory,
                              &blobSpPool,
                              sessionOptions,
                              emptyEventHandler,
                              emptyStateCb,
                              bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(!obj.isUsingSessionEventHandler());

    BMQTST_ASSERT(obj.lookupQueue(uri) == 0);
    BMQTST_ASSERT(obj.lookupQueue(k_QUEUE_ID) == 0);
    BMQTST_ASSERT(obj.lookupQueue(corrId) == 0);
}

static void test3_nullChannelTest()
// ------------------------------------------------------------------------
// NULL CHANNEL TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when null
//      bmqio::Channel is provided as a connection with the broker.
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set null communication channel.
//   3. Verify that no session events are generated.
//
//
// Testing manipulators:
//   - setChannel
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("NULL CHANNEL");

    const int                      k_NUM_EVENTS = 3;
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    bsls::AtomicInt                eventCounter(0);
    bsls::AtomicInt                startCounter(0);
    bsls::AtomicInt                stopCounter(0);
    bslmt::TimedSemaphore          eventSemaphore;

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    bmqimp::BrokerSession obj(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        bdlf::BindUtil::bind(&channelEventHandler,
                             bdlf::PlaceHolders::_1,
                             &eventSemaphore),
        bdlf::BindUtil::bind(&stateCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &startCounter,
                             &stopCounter),
        bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(obj.isUsingSessionEventHandler(), true);

    // Should not emit CONNECTION_LOST
    obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

    // Stop without previous start is ok.  DISCONNECTED will not be delivered
    // to the user but still stateCb will be called
    obj.stop();

    // 'stop' is synchronous and there is no IO connection.
    BMQTST_ASSERT_EQ(obj.state(), bmqimp::BrokerSession::State::e_STOPPED);
    BMQTST_ASSERT_EQ(stopCounter, 1);

    PVV_SAFE("Starting session...");
    int rc = obj.startAsync();
    BMQTST_ASSERT_EQ(rc, 0);

    // Session changes its state synchronously.
    BMQTST_ASSERT_EQ(obj.state(), bmqimp::BrokerSession::State::e_STARTING);
    BMQTST_ASSERT_EQ(startCounter, 1);

    PVV_SAFE("Resetting channel...");
    // Expect no CONNECTION_LOST events if a valid channel is never being set
    for (int i = 0; i < k_NUM_EVENTS; i++) {
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    // Expect one stop callback due to STARTING->STOPPED transition resulting
    // in stopCounter == 2 which we will check at the end.

    rc = eventSemaphore.timedWait(bmqsys::Time::nowRealtimeClock() +
                                  bsls::TimeInterval(0.1));

    // Timeout since there are no events
    BMQTST_ASSERT(rc != 0);

    PVV_SAFE("Stopping session...");
    obj.stop();

    // 'stop' is synchronous and there is no IO connection.

    BMQTST_ASSERT_EQ(startCounter, 1);
    BMQTST_ASSERT_EQ(stopCounter, 3);
}

static void test4_createEventTest()
// ------------------------------------------------------------------------
// CREATE EVENT TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession createEvent
//   method.
//
// Plan:
//   1. Create bmqimp::BrokerSession object.
//   2. Create bmqimp::Event object via createEvent method.
//   3. Verify that created object has "type" (returned by type method)
//   bmqimp::Event::EventType::e_UNINITIALIZED.
//
// Testing manipulators:
//   - createEvent
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CREATE EVENT TEST");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bmqimp::BrokerSession::StateFunctor      emptyStateCb;

    bmqimp::BrokerSession obj(&scheduler,
                              &blobBufferFactory,
                              &blobSpPool,
                              sessionOptions,
                              emptyEventHandler,
                              emptyStateCb,
                              bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Event> ev = obj.createEvent();
    BMQTST_ASSERT_EQ(bmqimp::Event::EventType::e_UNINITIALIZED, ev->type());
}

static void queueErrorsTest(bsls::Types::Uint64 queueFlags)
{
    bmqt::Uri uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bsls::TimeInterval&      timeout = bsls::TimeInterval();
    bsl::shared_ptr<bmqimp::Queue> pQueue;
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    bmqt::QueueOptions             queueOptions;
    bsl::shared_ptr<bmqimp::Event> eventSp;
    bsls::AtomicInt                startCounter(0);
    bsls::AtomicInt                stopCounter(0);
    bslmt::TimedSemaphore          semaphore;

    pQueue.createInplace(bmqtst::TestHelperUtil::allocator(),
                         bmqtst::TestHelperUtil::allocator());
    pQueue->setUri(uri);
    pQueue->setFlags(queueFlags);
    pQueue->setOptions(queueOptions);

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    scheduler.start();

    sessionOptions.setNumProcessingThreads(1);

    const bmqimp::EventQueue::EventHandlerCallback& eventCallback =
        bdlf::BindUtil::bind(&eventHandler,
                             &eventSp,
                             bsl::ref(semaphore),
                             bdlf::PlaceHolders::_1);  // event

    bmqimp::BrokerSession session(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        eventCallback,
        bdlf::BindUtil::bind(&stateCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &startCounter,
                             &stopCounter),
        bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Step 1. Starting session...");
    int rc = session.startAsync();
    BMQTST_ASSERT_EQ(rc, 0);

    // Session changes its state synchronously.
    BMQTST_ASSERT_EQ(session.state(),
                     bmqimp::BrokerSession::State::e_STARTING);

    BMQTST_ASSERT_EQ(startCounter, 1);
    PVV_SAFE("Session starting...: " << rc);  // Session did not start.

    PVV_SAFE("Step 2a. Checking deprecated sync open queue...");
    rc = session.openQueue(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_REFUSED);
    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);

    PVV_SAFE("Step 2b. Checking sync open queue...");
    session.openQueueSync(pQueue, timeout, eventCallback);
    // use the same callback to capture event
    BMQTST_ASSERT(waitRealTime(&semaphore));
    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);

    BMQTST_ASSERT(eventSp);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
    BMQTST_ASSERT_EQ(eventSp->statusCode(), bmqt::OpenQueueResult::e_REFUSED);

    eventSp.reset();

    PVV_SAFE("Step 3. Checking async open...");
    rc = session.openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_REFUSED);

    BMQTST_ASSERT(session.lookupQueue(uri) == 0);

    PVV_SAFE("Step 4a. Checking deprecated sync configure...");
    queueOptions.setMaxUnconfirmedMessages(0).setMaxUnconfirmedBytes(0);

    rc = session.configureQueue(pQueue, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Step 4b. Checking sync configure...");
    session.configureQueueSync(pQueue, queueOptions, timeout, eventCallback);

    BMQTST_ASSERT(waitRealTime(&semaphore));

    BMQTST_ASSERT(eventSp);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);
    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqt::ConfigureQueueResult::e_REFUSED);
    eventSp.reset();

    PVV_SAFE("Step 5. Checking async configure...");
    BMQTST_ASSERT_EQ(
        session.configureQueueAsync(pQueue, queueOptions, timeout),
        bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Step 6a. Checking deprecated sync configure...");
    rc = session.configureQueue(pQueue, queueOptions, timeout);

    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Step 6b. Checking sync configure...");
    session.configureQueueSync(pQueue, queueOptions, timeout, eventCallback);

    BMQTST_ASSERT(waitRealTime(&semaphore));

    BMQTST_ASSERT(eventSp);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);

    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqt::ConfigureQueueResult::e_REFUSED);

    eventSp.reset();

    PVV_SAFE("Step 7. Checking async configure...");
    rc = session.configureQueueAsync(pQueue, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Step 8a. Checking deprecated sync close...");
    rc = session.closeQueue(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_REFUSED);

    PVV_SAFE("Step 8b. Checking sync close...");

    session.closeQueueSync(pQueue, timeout, eventCallback);

    BMQTST_ASSERT(waitRealTime(&semaphore));

    BMQTST_ASSERT(eventSp != 0);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT);
    BMQTST_ASSERT_EQ(eventSp->statusCode(), bmqt::CloseQueueResult::e_REFUSED);

    eventSp.reset();

    PVV_SAFE("Step 9. Checking async close...");
    rc = session.closeQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_REFUSED);

    PVV_SAFE("Step 10. Stopping session...");
    session.stop();

    // 'stop' is synchronous and there is no IO connection.
    PVV_SAFE("Session stopped...");

    BMQTST_ASSERT_EQ(startCounter, 1);
    BMQTST_ASSERT_EQ(stopCounter, 1);

    // need to reset eventSp before the event pool destructs
    eventSp.clear();

    scheduler.cancelAllEventsAndWait();
    scheduler.stop();
}

static void test5_queueErrorsTest()
// ------------------------------------------------------------------------
// QUEUE ERRORS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has
//      no connection with the broker and queue manipulations are
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Check that all queue related operations return expected
//      error codes.
//
// Testing manipulators:
//   - openQueue
//   - openQueueAsync
//   - configureQueue
//   - configureQueueAsync
//   - closeQueue
//   - closeQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE ERRORS TEST");

    PVV_SAFE("Check READER");
    queueErrorsTest(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueErrorsTest(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueErrorsTest(flags);
}

static void test6_setChannelTest()
// ------------------------------------------------------------------------
// SET CHANNEL TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//   2. Check the behavior of the bmqimp::BrokerSession when a null
//      bmqio::Channel is provided as a connection with the broker.
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel several times.
//   3. Verify that special session events are generated:
//      - e_CONNECTED for the first setChannel call
//      - e_RECONNECTED and e_STATE_RESTORED for each subsequent call
//   4. Verify that 'e_CONNECTION_LOST' event is generated when setting a
//      null channel.
//   5. Verify that 'e_DISCONNECTED' event is generated after session is
//      stopped.
//
// Testing manipulators:
//   - setChannel
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SET CHANNEL");

    // We are going to start the session and in cycle set and reset the channel
    // for k_NUM_CALLS times. We expect two events on the first iteration:
    // - CONNECTED
    // - CONNECTION_LOST
    // For the rest (k_NUM_CALLS - 1) iterations three events are expected:
    // - RECONNECTED
    // - STATE_RESTORED
    // - CONNECTION_LOST
    const int                      k_NUM_CALLS  = 4;
    const int                      k_NUM_EVENTS = 2 + 3 * (k_NUM_CALLS - 1);
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    bsls::AtomicInt                eventCounter(0);
    bsls::AtomicInt                startCounter(0);
    bsls::AtomicInt                stopCounter(0);
    bmqio::TestChannel testChannel(bmqtst::TestHelperUtil::allocator());
    bslmt::TimedSemaphore          eventSemaphore;

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    bmqimp::BrokerSession obj(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        bdlf::BindUtil::bind(&channelSetEventHandler,
                             bdlf::PlaceHolders::_1,
                             bsl::ref(eventCounter),
                             bsl::ref(eventSemaphore),
                             k_NUM_EVENTS),
        bdlf::BindUtil::bind(&stateCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &startCounter,
                             &stopCounter),
        bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Starting session...");
    int rc = obj.startAsync();
    BMQTST_ASSERT_EQ(rc, 0);

    // Session changes its state synchronously.
    BMQTST_ASSERT_EQ(startCounter, 1);
    BMQTST_ASSERT_EQ(obj.state(), bmqimp::BrokerSession::State::e_STARTING);

    for (int i = 0; i < k_NUM_CALLS; i++) {
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    PVV_SAFE("Stopping session...");
    obj.stop();
    // 'stop' is synchronous and there is no IO connection.
    // 'stop' should unblock _after_ e_DISCONNECTED event
    BMQTST_ASSERT_EQ(stopCounter, 1);

    // Expect one more event - DISCONNECTED
    BMQTST_ASSERT_EQ(eventCounter, k_NUM_EVENTS + 1);
}

static void queueOpenTimeoutTest(bsls::Types::Uint64 queueFlags)
{
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI,
                                                            queueFlags,
                                                            queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 2. Check async open first part timeout");
    {
        int rc = obj.session().openQueueAsync(pQueue, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

        PVV_SAFE("Step 3. Verify open request was sent");
        obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);

        BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
        BMQTST_ASSERT_EQ(pQueue->isValid(), false);

        // Emulate request timeout.
        obj.advanceTime(timeout);

        PVV_SAFE("Step 4. Waiting QUEUE_OPEN_RESULT event...");
        BMQTST_ASSERT(obj.verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

        BMQTST_ASSERT_EQ(pQueue->state(),
                         bmqimp::QueueState::e_OPENING_OPN_EXPIRED);
        BMQTST_ASSERT_EQ(pQueue->isValid(), false);

        PVV_SAFE("Step 5. Reset the channel to make the queue CLOSED");
        // Reset channel to force closing of the expired queue
        obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());
        BMQTST_ASSERT(obj.waitConnectionLostEvent());

        BMQTST_ASSERT(
            obj.waitForQueueState(pQueue, bmqimp::QueueState::e_CLOSED));
        BMQTST_ASSERT_EQ(pQueue->isValid(), false);

        // Restore the channel, the queue is closed and can be reused
        obj.setChannel();
        BMQTST_ASSERT(obj.waitReconnectedEvent());
        BMQTST_ASSERT(obj.waitStateRestoredEvent());
    }

    PVV_SAFE("Step 6. Check async open second part timeout");
    {
        int rc = obj.session().openQueueAsync(pQueue, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

        PVV_SAFE("Step 8. Verify open request was sent");
        bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
            TestSession::e_REQ_OPEN_QUEUE);

        BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
        BMQTST_ASSERT_EQ(pQueue->isValid(), false);

        PVV_SAFE("Step 9. Send open queue response message");
        obj.sendResponse(request);

        // Writer queue will be opened without configuring
        if (!bmqt::QueueFlagsUtil::isReader(queueFlags)) {
            PVV_SAFE("Step 10. Waiting QUEUE_OPEN_RESULT event...");
            BMQTST_ASSERT(obj.verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

            BMQTST_ASSERT(
                obj.waitForQueueState(pQueue, bmqimp::QueueState::e_OPENED));
            BMQTST_ASSERT(pQueue->isValid());
        }
        else {
            BMQTST_ASSERT(
                obj.waitForQueueState(pQueue,
                                      bmqimp::QueueState::e_OPENING_CFG));
            BMQTST_ASSERT_EQ(pQueue->isValid(), false);

            PVV_SAFE("Step 10. Verify config request was sent");
            obj.verifyRequestSent(TestSession::e_REQ_CONFIG_QUEUE);

            // Emulate request timeout.
            obj.advanceTime(timeout);

            PVV_SAFE("Step 11. Waiting QUEUE_OPEN_RESULT event...");
            BMQTST_ASSERT(obj.verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));
            BMQTST_ASSERT_EQ(pQueue->state(),
                             bmqimp::QueueState::e_OPENING_CFG_EXPIRED);
            BMQTST_ASSERT_EQ(pQueue->isValid(), false);
        }
    }

    PVV_SAFE("Step 12. Stop the session");
    obj.stopGracefully();
}

static void test7_queueOpenTimeoutTest()
// ------------------------------------------------------------------------
// QUEUE OPEN TIMEOUT TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and open queue operations are
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session.
//   2. Set a test network channel.
//   2. Check that open queue operations return expected
//      error codes and generate expected events.
//
// Testing manipulators:
//   - openQueue
//   - openQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE OPEN TIMEOUT TEST");

    PVV_SAFE("Check READER");
    queueOpenTimeoutTest(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueOpenTimeoutTest(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueOpenTimeoutTest(flags);
}

static void test8_queueWriterConfigureTest()
// ------------------------------------------------------------------------
// QUEUE WRITER CONFIGURE TEST
//
// Concerns:
//   1. Check that calling configure for a writer only queue is a no-op and
//      doesn't send any request to the broker.
//   2. Check that configureSync does not dead-lock after stop
//
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE WRITER CONFIGURE TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE);

    (*pQueue).setState(bmqimp::QueueState::e_OPENED);
    (*pQueue).setRequestGroupId(1);
    // Fake the state of the queue as calling configure on a closed queue
    // is forbidden

    int rc = 0;

    PVV_SAFE("Starting session...");
    rc = obj.session().startAsync();
    BMQTST_ASSERT_EQ(rc, 0);

    PVV_SAFE("Calling configure queue when no channel");
    rc = obj.session().configureQueue(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Setting channel");
    obj.setChannel();

    // Consume the connected event (so that stop gracefully will be able to
    // work and pop the 'disconnected' event)
    BMQTST_ASSERT(obj.waitConnectedEvent());

    // We empty any outbound message now, so that we can then ensure no
    // messages was sent as part of the following configures).
    obj.channel().writeCalls().clear();

    PVV_SAFE("Calling configure queue when channel");
    rc = obj.session().configureQueue(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);
    BMQTST_ASSERT(obj.channel().writeCalls().empty());

    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);
    BMQTST_ASSERT(obj.channel().writeCalls().empty());

    // Async configure should have enqueued a configure result of value success
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    PVV_SAFE("Stop the session");
    obj.session().stopAsync();
    // This is to set 'd_disconnectRequested == true'

    // Check configure _after_ stop ('d_disconnectRequested == true')

    PVV_SAFE("Checking deprecated sync configure...");

    rc = obj.session().configureQueue(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Checking sync configure...");
    obj.configureQueueSync(pQueue, pQueue->options(), timeout);

    obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
                              bmqt::ConfigureQueueResult::e_REFUSED);

    PVV_SAFE("Checking async configure...");
    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_REFUSED);

    // The non-callback 'configureQueueAsync' versions _must_ _not_ generate
    /// an event on failure
    BMQTST_ASSERT(obj.checkNoEvent());

    // Now really stop
    obj.stopGracefully();
}

static void queueOpenErrorTest(bsls::Types::Uint64 queueFlags)
{
    bmqtst::TestHelper::printTestName("QUEUE OPEN ERROR TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI,
                                                            queueFlags,
                                                            queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 2. Check async open first part error");
    int rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 3. Check open request was sent");
    const int openReqId1 = obj.verifyRequestSent(
        TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 4. Send open queue response with error");
    bmqp_ctrlmsg::ControlMessage responseMessage(
        bmqtst::TestHelperUtil::allocator());
    responseMessage.rId().makeValue(openReqId1);
    responseMessage.choice().makeStatus();
    responseMessage.choice().status().category() =
        bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;

    obj.sendControlMessage(responseMessage);

    PVV_SAFE("Step 5. Waiting QUEUE_OPEN_RESULT event...");
    obj.verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                   pQueue,
                                   bmqimp::QueueState::e_CLOSED);

    PVV_SAFE("Step 6. Check async open second part error");
    rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 7. Check open request was sent");
    bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 8. Send open queue response message");
    obj.sendResponse(request);

    // Writer queue is already opened, reader queue should be configured
    if (!bmqt::QueueFlagsUtil::isReader(queueFlags)) {
        PVV_SAFE("Step 9. Waiting QUEUE_OPEN_RESULT event...");
        BMQTST_ASSERT(obj.verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

        BMQTST_ASSERT(
            obj.waitForQueueState(pQueue, bmqimp::QueueState::e_OPENED));
        BMQTST_ASSERT(pQueue->isValid());
    }
    else {
        PVV_SAFE("Step 9. Check config request was sent");
        const int confReqId = obj.verifyRequestSent(
            TestSession::e_REQ_CONFIG_QUEUE);

        PVV_SAFE("Step 10. Send config response with error");
        responseMessage.rId().makeValue(confReqId);
        obj.sendControlMessage(responseMessage);

        PVV_SAFE("Step 11. Waiting QUEUE_OPEN_RESULT event...");
        obj.verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                       pQueue,
                                       bmqimp::QueueState::e_CLOSING_CLS);

        PVV_SAFE("Step 12. Check close request was sent");
        request = obj.verifyCloseRequestSent(true);  // ifFinal

        PVV_SAFE("Step 13. Send close response");
        obj.sendResponse(request);

        PVV_SAFE("Step 14. Check the queue gets closed");
        BMQTST_ASSERT(
            obj.waitForQueueState(pQueue, bmqimp::QueueState::e_CLOSED));
    }

    PVV_SAFE("Step 15. Stop the session");
    obj.stopGracefully();
}

static void test9_queueOpenErrorTest()
// ------------------------------------------------------------------------
// QUEUE OPEN ERROR TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and async open queue operation is
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session.
//   2. Set a test network channel.
//   3. Check that async open queue operations return expected
//      error codes when upstream responses indicate error statuses.
//
// Testing manipulators:
//   - openQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE OPEN ERROR TEST");

    PVV_SAFE("Check READER");
    queueOpenErrorTest(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueOpenErrorTest(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueOpenErrorTest(flags);
}

static void queueOpenCloseAsync(bsls::Types::Uint64 queueFlags)
{
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI,
                                                            queueFlags,
                                                            queueOptions);

    PVV_SAFE("Start the session");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Close unopened queue async");
    int rc = obj.session().closeQueueAsync(pQueue, timeout);

    // Verify the result
    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_SUCCESS);

    PVV_SAFE("Waiting QUEUE_CLOSE_RESULT event...");
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT,
                                  bmqt::CloseQueueResult::e_UNKNOWN_QUEUE));

    PVV_SAFE("Open the queue async");
    rc = obj.session().openQueueAsync(pQueue, timeout);

    // Verify the result
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    // Verify open request (todo: check handle queue parameters)
    PVV_SAFE("Ensure open queue request has been sent");
    const int openId = obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    // Prepare and process open queue response
    PVV_SAFE("Prepare and send back open queue response");
    bmqp_ctrlmsg::ControlMessage openQueueResponse(
        bmqtst::TestHelperUtil::allocator());
    openQueueResponse.rId().makeValue(openId);
    openQueueResponse.choice().makeOpenQueueResponse();

    obj.sendControlMessage(openQueueResponse);

    bmqp_ctrlmsg::ControlMessage configureQueueMessage(
        bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::ControlMessage configureQueueResponse(
        bmqtst::TestHelperUtil::allocator());

    // For reader queue there is a configuring phase
    if (bmqt::QueueFlagsUtil::isReader(queueFlags)) {
        // Verify configure request (todo: check stream parameters and qid)
        PVV_SAFE("Ensure configure queue request has been sent");
        obj.getOutboundControlMessage(&configureQueueMessage);

        BMQTST_ASSERT(!configureQueueMessage.rId().isNull());
        BMQTST_ASSERT(isConfigure(configureQueueMessage));

        // Prepare and process configure queue response
        PVV_SAFE("Prepare and send back configure queue response");

        makeResponse(&configureQueueResponse, configureQueueMessage);

        obj.sendControlMessage(configureQueueResponse);
    }

    PVV_SAFE("Waiting QUEUE_OPEN_RESULT event...");
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(pQueue->isValid());

    // Close the queue async
    PVV_SAFE("Close the queue async");
    rc = obj.session().closeQueueAsync(pQueue, timeout);

    // Verify the result
    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_SUCCESS);

    // For reader queue there is a deconfiguring phase
    if (bmqt::QueueFlagsUtil::isReader(queueFlags)) {
        // Verify configure request (todo: check stream parameters and qid)
        PVV_SAFE("Ensure configure queue request has been sent");
        configureQueueMessage.reset();
        obj.getOutboundControlMessage(&configureQueueMessage);

        BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSING_CFG);
        BMQTST_ASSERT_EQ(pQueue->isValid(), false);

        BMQTST_ASSERT(!configureQueueMessage.rId().isNull());
        BMQTST_ASSERT(isConfigure(configureQueueMessage));

        // Prepare and process configure queue response
        PVV_SAFE("Prepare and send back configure queue response");
        configureQueueResponse.reset();

        makeResponse(&configureQueueResponse, configureQueueMessage);

        obj.sendControlMessage(configureQueueResponse);
    }

    // Verify close request (todo: check handle queue parameters)
    PVV_SAFE("Ensure close queue request has been sent");
    bmqp_ctrlmsg::ControlMessage closeQueueMessage(
        bmqtst::TestHelperUtil::allocator());
    obj.getOutboundControlMessage(&closeQueueMessage);

    BMQTST_ASSERT(!closeQueueMessage.rId().isNull());
    BMQTST_ASSERT(closeQueueMessage.choice().isCloseQueueValue());
    BMQTST_ASSERT(closeQueueMessage.choice().closeQueue().isFinal());

    // Prepare and process close queue response
    PVV_SAFE("Prepare and send back close queue response");
    bmqp_ctrlmsg::ControlMessage closeQueueResponse(
        bmqtst::TestHelperUtil::allocator());
    closeQueueResponse.rId().makeValue(closeQueueMessage.rId().value());
    closeQueueResponse.choice().makeCloseQueueResponse();

    obj.sendControlMessage(closeQueueResponse);

    PVV_SAFE("Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                               pQueue);

    obj.stopGracefully();
}

static void test10_queueOpenCloseAsync()
// ------------------------------------------------------------------------
// ASYNC OPEN CLOSE QUEUE TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker, async open (+configure) and
//      close (+configure) queue operations are successfully performed.
//
// Plan:
//   1.  Create bmqimp::BrokerSession test wrapper object
//       and start the session.
//   2.  Set a test network channel.
//   3.  Open queue async for reading
//   4.  Ensure open queue request has been sent
//   5.  Prepare and send back successful open queue response
//   6.  Ensure configure queue request has been sent
//   7.  Prepare and send back successful configure queue response
//   8.  Wait QUEUE_OPEN_RESULT event
//   9.  Close queue async
//   10. Ensure configure queue request has been sent
//   11. Prepare and send back successful configure queue response
//   12. Ensure close queue request has been sent
//   13. Prepare and send back successful close queue response
//   14. Wait QUEUE_CLOSE_RESULT event
//   15. Stop the session
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueueAsync
//   - closeQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ASYNC OPEN/CLOSE QUEUE TEST");

    PVV_SAFE("Check READER");
    queueOpenCloseAsync(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueOpenCloseAsync(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueOpenCloseAsync(flags);
}

static void test11_disconnect()
// ------------------------------------------------------------------------
// DISCONNECT TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.  When
//      is being stopped it should properly send disconnect message to the
//      broker, receive a response and generate DISCONNECTED event.  If
//      user is using event handler, a single DISCONNECTED event should be
//      emitted (regardless of the number of processing threads), when not
//      using event handler, 'bmqimp::BrokerSession.nextEvent' should
//      'infinitely' emit DISCONNECTED events.
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Stop the session
//   4. Generate disconnect response message
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bdlcc::Deque<bsl::shared_ptr<bmqimp::Event> > eventQueue(
        bsls::SystemClockType::e_MONOTONIC,
        bmqtst::TestHelperUtil::allocator());
    bmqio::TestChannel testChannel(bmqtst::TestHelperUtil::allocator());
    bsls::AtomicInt    startCounter(0);
    bsls::AtomicInt    stopCounter(0);

    int rc = scheduler.start();
    BMQTST_ASSERT_EQ(rc, 0);

    testChannel.setPeerUri("tcp://testHost:1234");  // for better logging

    {
        PV_SAFE("Using EventHandler");
        PV_SAFE("------------------");

        bmqt::SessionOptions options;
        options.setNumProcessingThreads(5);

        bmqimp::BrokerSession obj(
            &scheduler,
            &blobBufferFactory,
            &blobSpPool,
            options,
            bdlf::BindUtil::bind(&sessionEventHandler,
                                 &eventQueue,
                                 bdlf::PlaceHolders::_1),
            bdlf::BindUtil::bind(&stateCb,
                                 bdlf::PlaceHolders::_1,  // old state
                                 bdlf::PlaceHolders::_2,  // new state
                                 &startCounter,
                                 &stopCounter),
            bmqtst::TestHelperUtil::allocator());

        // Start the session
        PVV_SAFE("Starting session");
        rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Session changes its state synchronously.
        BMQTST_ASSERT_EQ(startCounter, 1);
        BMQTST_ASSERT_EQ(obj.state(),
                         bmqimp::BrokerSession::State::e_STARTING);

        // Set channel
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        PVV_SAFE("Waiting CONNECTED event...");
        bsl::shared_ptr<bmqimp::Event> event;
        rc = eventQueue.timedPopFront(&event,
                                      bmqsys::Time::nowMonotonicClock() +
                                          bsls::TimeInterval(5));
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(event->statusCode(), 0);
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTED);
        PVV_SAFE("Channel connected!");

        // Stop the session
        PVV_SAFE("Stopping session...");
        obj.stopAsync();

        // Ensure no event is yet emitted to the user
        event.reset();
        eventQueue.timedPopFront(&event,
                                 bmqsys::Time::nowMonotonicClock() +
                                     bsls::TimeInterval(0.1));
        BMQTST_ASSERT(!event);

        PVV_SAFE("Ensure a disconnectMessage was sent to the broker");
        bmqp_ctrlmsg::ControlMessage disconnectMessage(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT(testChannel.waitFor(1, true, bsls::TimeInterval(5)));
        bmqio::TestChannel::WriteCall wc = testChannel.popWriteCall();
        bmqp::Event ev(&wc.d_blob, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT(ev.isControlEvent());

        rc = ev.loadControlEvent(&disconnectMessage);
        BMQTST_ASSERT_EQ(rc, 0);

        PVV_SAFE("Send the disconnect response");
        bmqp_ctrlmsg::ControlMessage disconnectResponseMessage(
            bmqtst::TestHelperUtil::allocator());
        disconnectResponseMessage.rId().makeValue(
            disconnectMessage.rId().value());
        disconnectResponseMessage.choice().makeDisconnectResponse();

        bmqp::SchemaEventBuilder builder(&blobSpPool,
                                         bmqp::EncodingType::e_BER,
                                         bmqtst::TestHelperUtil::allocator());
        rc = builder.setMessage(disconnectResponseMessage,
                                bmqp::EventType::e_CONTROL);
        BMQTST_ASSERT_EQ(rc, 0);

        obj.processPacket(*builder.blob());

        // Reset the channel
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

        PVV_SAFE("Verify the user receives the DISCONNECTED event");
        rc = eventQueue.timedPopFront(&event,
                                      bmqsys::Time::nowMonotonicClock() +
                                          bsls::TimeInterval(5));
        BMQTST_ASSERT_EQ(rc, 0);
        BMQTST_ASSERT_EQ(event->statusCode(), 0);
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_DISCONNECTED);

        // Ensure no more  events are delivered to the user
        event.reset();
        eventQueue.timedPopFront(&event,
                                 bmqsys::Time::nowMonotonicClock() +
                                     bsls::TimeInterval(0.1));
        BMQTST_ASSERT(!event);

        BMQTST_ASSERT_EQ(startCounter, 1);
        // Ensure stateCb was called
        BMQTST_ASSERT_EQ(stopCounter, 1);
    }

    {
        PV_SAFE("Not using EventHandler");
        PV_SAFE("----------------------");

        bmqt::SessionOptions  options;
        bmqimp::BrokerSession obj(
            &scheduler,
            &blobBufferFactory,
            &blobSpPool,
            options,
            bmqimp::EventQueue::EventHandlerCallback(),
            bdlf::BindUtil::bind(&stateCb,
                                 bdlf::PlaceHolders::_1,  // old state
                                 bdlf::PlaceHolders::_2,  // new state
                                 &startCounter,
                                 &stopCounter),
            bmqtst::TestHelperUtil::allocator());

        // Start the session
        PVV_SAFE("Starting session");
        rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Session changes its state synchronously.
        BMQTST_ASSERT_EQ(obj.state(),
                         bmqimp::BrokerSession::State::e_STARTING);
        BMQTST_ASSERT_EQ(startCounter, 2);

        // Set channel
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        PVV_SAFE("Waiting CONNECTED event...");
        bsl::shared_ptr<bmqimp::Event> event;
        event = obj.nextEvent(bsls::TimeInterval(5));
        BMQTST_ASSERT(event);
        BMQTST_ASSERT_EQ(event->statusCode(), 0);
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_CONNECTED);
        PV_SAFE("Channel connected!");

        // Stop the session
        PVV_SAFE("Stopping session...");
        obj.stopAsync();

        // Ensure no event is yet emitted to the user
        event.reset();
        event = obj.nextEvent(bsls::TimeInterval(0.1));
        BMQTST_ASSERT_EQ(event->sessionEventType(),
                         bmqt::SessionEventType::e_TIMEOUT);

        PVV_SAFE("Ensure a disconnectMessage was sent to the broker");
        bmqp_ctrlmsg::ControlMessage disconnectMessage(
            bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT(testChannel.waitFor(1, true, bsls::TimeInterval(5)));
        bmqio::TestChannel::WriteCall wc = testChannel.popWriteCall();
        bmqp::Event ev(&wc.d_blob, bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT(ev.isControlEvent());

        rc = ev.loadControlEvent(&disconnectMessage);
        BMQTST_ASSERT_EQ(rc, 0);

        PVV_SAFE("Send the disconnect response");
        bmqp_ctrlmsg::ControlMessage disconnectResponseMessage(
            bmqtst::TestHelperUtil::allocator());
        disconnectResponseMessage.rId().makeValue(
            disconnectMessage.rId().value());
        disconnectResponseMessage.choice().makeDisconnectResponse();

        bmqp::SchemaEventBuilder builder(&blobSpPool,
                                         bmqp::EncodingType::e_BER,
                                         bmqtst::TestHelperUtil::allocator());
        rc = builder.setMessage(disconnectResponseMessage,
                                bmqp::EventType::e_CONTROL);
        BMQTST_ASSERT_EQ(rc, 0);

        obj.processPacket(*builder.blob());

        // Reset the channel
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

        PVV_SAFE("Verify the user receives a DISCONNECTED event");
        // Since we are using next event, we should 'infinitely' pop out
        // 'disconnected' events now.
        for (int i = 0; i < 5; ++i) {
            event = obj.nextEvent(bsls::TimeInterval(5));
            BMQTST_ASSERT(event);
            BMQTST_ASSERT_EQ(event->statusCode(), 0);
            BMQTST_ASSERT_EQ(event->sessionEventType(),
                             bmqt::SessionEventType::e_DISCONNECTED);
        }

        BMQTST_ASSERT_EQ(startCounter, 2);
        // Ensure stateCb was called
        BMQTST_ASSERT_EQ(stopCounter, 2);
    }

    scheduler.cancelAllEventsAndWait();
    scheduler.stop();
}

static void test12_disconnectStatus()
// ------------------------------------------------------------------------
// DISCONNECT STATUS MESSAGE RESPONSE TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should properly send disconnect message
//      to the broker, but there will be an unknown status message response
//      which should not break the process
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Stop the session
//   4. Generate status message
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT STATUS MESSAGE");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    obj.startAndConnect();

    PVV_SAFE("Stopping session...");
    obj.session().stopAsync();

    bmqp_ctrlmsg::ControlMessage disconnectMessage(
        bmqtst::TestHelperUtil::allocator());
    obj.getOutboundControlMessage(&disconnectMessage);

    BMQTST_ASSERT(!disconnectMessage.rId().isNull());
    BMQTST_ASSERT(disconnectMessage.choice().isDisconnectValue());

    // prepare status message
    bmqp_ctrlmsg::ControlMessage statusMessage(
        bmqtst::TestHelperUtil::allocator());
    statusMessage.rId().makeValue(disconnectMessage.rId().value());
    statusMessage.choice().makeStatus();
    statusMessage.choice().status().category() =
        bmqp_ctrlmsg::StatusCategory::E_UNKNOWN;
    statusMessage.choice().status().code()    = -1;
    statusMessage.choice().status().message() = "unknown status";

    // process status message
    obj.sendControlMessage(statusMessage);

    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitDisconnectedEvent());

    BMQTST_ASSERT(obj.verifySessionIsStopped());
}

static void test13_disconnectTimeout()
// ------------------------------------------------------------------------
// DISCONNECT TIMEOUT TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should properly send disconnect message
//      to the broker, but there will be no response, timeout handler
//      should be called and the session should be stopped.
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Stop the session
//   4. Do not generate disconnect response message, wait for timeout
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT TIMEOUT");

    const bsls::TimeInterval timeout = bsls::TimeInterval(5);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);
    sessionOptions.setNumProcessingThreads(1).setDisconnectTimeout(timeout);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    obj.startAndConnect();

    PVV_SAFE("Stopping session...");
    obj.session().stopAsync();

    bmqp_ctrlmsg::ControlMessage disconnectMessage(
        bmqtst::TestHelperUtil::allocator());
    obj.getOutboundControlMessage(&disconnectMessage);

    BMQTST_ASSERT(!disconnectMessage.rId().isNull());
    BMQTST_ASSERT(disconnectMessage.choice().isDisconnectValue());

    // Emulate disconnect request timeout.
    obj.advanceTime(timeout);

    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitDisconnectedEvent());

    BMQTST_ASSERT(obj.verifySessionIsStopped());
}

static void test14_disconnectRequestGenericErr()
// ------------------------------------------------------------------------
// DISCONNECT REQUEST GENERIC ERROR STATUS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should fail to send disconnect message
//      to the broker due to generic error, but this should not break
//      the process and the session should be successfully stopped
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Set the test channel write status to e_GENERIC_ERROR
//   4. Stop the session
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT FAILED REQUEST (GENERIC)");
    test_disconnectRequestErr(bmqio::StatusCategory::e_GENERIC_ERROR);
}

static void test15_disconnectRequestConnection()
// ------------------------------------------------------------------------
// DISCONNECT REQUEST CONNECTION STATUS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should fail to send disconnect message
//      to the broker due to connection problems, but this should not break
//      the process and the session should be successfully stopped
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Set the test channel write status to e_CONNECTION
//   4. Stop the session
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "DISCONNECT FAILED REQUEST (CONNECTION)");
    test_disconnectRequestErr(bmqio::StatusCategory::e_CONNECTION);
}

static void test16_disconnectRequestTimeout()
// ------------------------------------------------------------------------
// DISCONNECT REQUEST TIMEOUT STATUS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should fail to send disconnect message
//      to the broker due to timeout, but this should not break
//      the process and the session should be successfully stopped
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Set the test channel write status to e_TIMEOUT
//   4. Stop the session
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT FAILED REQUEST (TIMEOUT)");
    test_disconnectRequestErr(bmqio::StatusCategory::e_TIMEOUT);
}

static void test17_disconnectRequestCanceled()
// ------------------------------------------------------------------------
// DISCONNECT REQUEST CANCELED STATUS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should fail to send disconnect message
//      to the broker due to canceled request, but this should not break
//      the process and the session should be successfully stopped
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Set the test channel write status to e_CANCELED
//   4. Stop the session
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT FAILED REQUEST (CANCELED)");
    test_disconnectRequestErr(bmqio::StatusCategory::e_CANCELED);
}

static void test18_disconnectRequestLimit()
// ------------------------------------------------------------------------
// DISCONNECT REQUEST LIMIT STATUS TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when a valid
//      bmqio::Channel is provided as a connection with the broker.
//      When is being stopped it should fail to send disconnect message
//      to the broker due to busy channel, but this should not break
//      the process and the session should be successfully stopped
//
// Plan:
//   1. Create bmqimp::BrokerSession object and start the session.
//   2. Set a valid test communication channel.
//   3. Set the test channel write status to e_LIMIT
//   4. Stop the session
//   5. Wait for e_DISCONNECTED session event
//
// Testing manipulators:
//   - start
//   - setChannel
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("DISCONNECT FAILED REQUEST (LIMIT)");
    test_disconnectRequestErr(bmqio::StatusCategory::e_LIMIT);
}

static void lateOpenQueueResponse(bsls::Types::Uint64 queueFlags)
{
    bmqtst::TestHelper::printTestName("QUEUE LATE OPEN RESPONSE TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI, queueFlags);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 2. Check async open error");
    int rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 3. Check open request was sent");
    bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    // Emulate request timeout.
    obj.advanceTime(timeout);

    PVV_SAFE("Step 4. Waiting QUEUE_OPEN_RESULT event...");
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT_EQ(pQueue->state(),
                     bmqimp::QueueState::e_OPENING_OPN_EXPIRED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 5. Send late open queue response message");
    obj.sendResponse(request);

    PVV_SAFE("Step 6. Check close request was sent");
    request = obj.verifyCloseRequestSent(true);  // ifFinal

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSING_CLS);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    BMQTST_ASSERT(obj.session().lookupQueue(pQueue->uri()) != 0);

    PVV_SAFE("Step 7. Send close response");
    obj.sendResponse(request);

    PVV_SAFE("Step 8. Check the queue gets closed");
    BMQTST_ASSERT(obj.waitForQueueRemoved(pQueue));

    PVV_SAFE("Step 9. Stop the session");
    obj.stopGracefully();
}

static void test19_queueOpen_LateOpenQueueResponse()
// ------------------------------------------------------------------------
// QUEUE OPEN ERROR TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and async open queue operation is
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session.
//   2. Set a test network channel.
//   3. Check that async open queue operation returns expected
//      error code when upstream open queue response comes late (after
//      local timeout).
//
// Testing manipulators:
//   - openQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE LATE OPEN RESPONSE TEST");

    PVV_SAFE("Check READER");
    lateOpenQueueResponse(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    lateOpenQueueResponse(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    lateOpenQueueResponse(flags);
}

static void test20_queueOpen_LateConfigureQueueResponse()
// ------------------------------------------------------------------------
// QUEUE OPEN ERROR TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and async open queue operation is
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session.
//   2. Set a test network channel.
//   3. Check that async open queue operation returns expected
//      error code when upstream configure queue response comes late (after
//      local timeout).
//
// Testing manipulators:
//   - openQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE LATE CONFIGURE RESPONSE TEST");

    bmqt::Uri uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bsls::TimeInterval     timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions         sessionOptions;
    bmqp_ctrlmsg::ControlMessage responseMessage(
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler        scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                    testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    // The test case is applicable only for the reader queue
    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 2. Check async open first part error");
    int rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 3. Check open request was sent");
    bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_OPENING_OPN);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 4. Send valid open queue response message");
    obj.sendResponse(request);

    PVV_SAFE("Step 5. Check configure request was sent");
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // Emulate request timeout.
    obj.advanceTime(timeout);

    PVV_SAFE("Step 6. Waiting QUEUE_OPEN_RESULT event...");
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT_EQ(pQueue->state(),
                     bmqimp::QueueState::e_OPENING_CFG_EXPIRED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 7. Send late configure queue response message");
    obj.sendResponse(request);

    PVV_SAFE("Step 8. Check close request (config part) was sent");
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSING_CFG);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    BMQTST_ASSERT(obj.session().lookupQueue(uri) != 0);

    PVV_SAFE("Step 9. Send close response (config part)");
    obj.sendResponse(request);

    PVV_SAFE("Step 10. Check close request (final part) was sent");
    request = obj.verifyCloseRequestSent(true);  // ifFinal

    PVV_SAFE("Step 11. Send close response (final part)");
    obj.sendResponse(request);

    PVV_SAFE("Step 12. Check the queue gets closed");
    BMQTST_ASSERT(obj.waitForQueueRemoved(pQueue));

    PVV_SAFE("Step 13. Stop the session");
    obj.stopGracefully();
}

static void test21_post_Limit()
// ------------------------------------------------------------------------
// POST CHANNEL LIMIT ERROR
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and post operation is performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object and start the
//      session.
//   2. Open queue async
//   3. Set the channel to return e_LIMIT status
//   4. Create and post PUT message
//   5. Ensure e_BW_LIMIT is returned
//   6. Schedule channel LWM event and repeat posting PUT message
//   7. Ensure e_SUCCESS is returned
//   8. Set the channel to return e_GENERIC_ERROR status
//   9. Ensure posting PUT message returns e_SUCCESS
//   10. Ensure the channel is not closed
//   11. Stop the session
//
// Testing manipulators:
//   - openQueueAsync
//   - post
//-------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("POST CHANNEL LIMIT TEST");

    const bsls::TimeInterval timeout     = bsls::TimeInterval(15);
    const bsls::TimeInterval postTimeout = bsls::TimeInterval(0.1);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    // Create test session with the system time source
    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Set the channel to return e_LIMIT on write");
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_LIMIT);

    PVV_SAFE("Step 4. Create and post PUT message");
    bmqp::Crc32c::initialize();
    bmqp::PutEventBuilder builder(&obj.blobSpPool(), obj.allocator());

    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    builder.startMessage();
    builder.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setMessageGUID(bmqp::MessageGUIDGenerator::testGUID());
    BMQTST_ASSERT_EQ(bmqt::EventBuilderResult::e_SUCCESS,
                     builder.packMessage(pQueue->id()));

    int rc = obj.session().post(*builder.blob(), postTimeout);

    PVV_SAFE("Step 5. Ensure the PUT message is accepted");
    BMQTST_ASSERT_EQ(rc, bmqt::PostResult::e_SUCCESS);

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    // Check PUT event is sent
    bmqp::Event rawEvent(bmqtst::TestHelperUtil::allocator());
    obj.getOutboundEvent(&rawEvent);

    BMQTST_ASSERT(rawEvent.isPutEvent());

    PVV_SAFE("Step 6. Ensure e_BW_LIMIT is returned for the second post");
    rc = obj.session().post(*builder.blob(), postTimeout);

    BMQTST_ASSERT_EQ(rc, bmqt::PostResult::e_BW_LIMIT);
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 0u);

    PVV_SAFE("Step 7. Schedule LWM and post PUT again");
    scheduler.scheduleEvent(
        bmqsys::Time::nowMonotonicClock() + postTimeout,
        bdlf::BindUtil::bind(&TestSession::setChannelLowWaterMark,
                             &obj,
                             bmqio::StatusCategory::e_SUCCESS));

    // Call post with a bigger timeout so that LWM event arrives before it
    // expires.
    rc = obj.session().post(*builder.blob(), timeout);

    PVV_SAFE("Step 8. Ensure e_SUCCESS is returned");
    BMQTST_ASSERT_EQ(rc, bmqt::PostResult::e_SUCCESS);

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    // Expect two PUT events in the test channel - one from the attempt that
    // returned E_LIMIT and the second one after LWM event.
    for (int i = 0; i < 2; i++) {
        rawEvent.clear();
        obj.getOutboundEvent(&rawEvent);
        BMQTST_ASSERT(rawEvent.isPutEvent());
    }
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 0u);

    PVV_SAFE("Step 9. Set the channel to return e_GENERIC_ERROR on write");
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_GENERIC_ERROR);

    rc = obj.session().post(*builder.blob(), postTimeout);

    PVV_SAFE("Step 10. Ensure e_SUCCESS is returned");
    BMQTST_ASSERT_EQ(rc, bmqt::PostResult::e_SUCCESS);

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    // Verify the channel is closed after write error
    rawEvent.clear();
    obj.getOutboundEvent(&rawEvent);
    BMQTST_ASSERT(rawEvent.isPutEvent());
    BMQTST_ASSERT_EQ(obj.channel().closeCalls().size(), 1u);

    // Set write status back to e_SUCCESS
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_SUCCESS);

    PV_SAFE("Step 11. Stop the session");
    obj.stopGracefully();
}

static void test22_confirm_Limit()
// ------------------------------------------------------------------------
// CONFIRM MESSAGES CHANNEL LIMIT ERROR
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and confirm operation is
//      performed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object and start the
//      session.
//   2. Open queue async
//   3. Set the channel to return e_LIMIT status
//   4. Create the blob and confirm messages
//   5. Ensure e_NOT_READY is returned
//   6. Schedule channel LWM event and repeat sending confirm message
//   7. Ensure e_SUCCESS is returned
//   8. Set the channel to return e_GENERIC_ERROR status
//   9. Ensure sending confirm message returns e_UNKNOWN
//   10. Ensure the channel is closed
//   11. Stop the session
//
// Testing manipulators:
//   - openQueueAsync
//   - confirmMessages
//-------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONFIRM MESSAGES CHANNEL LIMIT TEST");

    const bsls::TimeInterval timeout        = bsls::TimeInterval(15);
    const bsls::TimeInterval confirmTimeout = bsls::TimeInterval(0.1);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Set the channel to return e_LIMIT on write");
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_LIMIT);

    PVV_SAFE("Step 4. Create the blob and confirm messages");
    bmqp::ConfirmEventBuilder builder(&obj.blobSpPool(), obj.allocator());

    int rc = builder.appendMessage(pQueue->id(),
                                   pQueue->subQueueId(),
                                   bmqt::MessageGUID());
    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    rc = obj.session().confirmMessages(*builder.blob(), confirmTimeout);

    PVV_SAFE("Step 5. Ensure e_SUCCESS is returned");
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 1u);

    // Clear test channel write queue
    obj.channel().writeCalls().clear();

    PVV_SAFE("Step 6. Schedule LWM and send confirm again");

    // LWM handler will set write status to e_SUCCESS
    scheduler.scheduleEvent(
        bmqsys::Time::nowMonotonicClock() + confirmTimeout,
        bdlf::BindUtil::bind(&TestSession::setChannelLowWaterMark,
                             &obj,
                             bmqio::StatusCategory::e_SUCCESS));

    // Call confirm with a bigger timeout so that LWM event arrives before it
    // expires.
    rc = obj.session().confirmMessages(*builder.blob(), timeout);

    PVV_SAFE("Step 7. Ensure e_SUCCESS is returned");
    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    // Expect two write calls in the test channel - one from the first attempt
    // that returned E_LIMIT and the second one after LWM event.
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 2u);

    // Clear test channel write queue
    obj.channel().writeCalls().clear();

    PVV_SAFE("Step 8. Set the channel to return e_GENERIC_ERROR on write");
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_GENERIC_ERROR);

    rc = obj.session().confirmMessages(*builder.blob(), confirmTimeout);

    PVV_SAFE("Step 9. Ensure e_SUCCESS");

    // Drain the FSM queue
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_SUCCESS);
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 1u);

    // Clear test channel write queue
    obj.channel().writeCalls().clear();

    // Set write status back to e_SUCCESS
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_SUCCESS);

    PV_SAFE("Step 10. Stop the session");
    obj.stopGracefully();
}

static void queueCloseSync(bsls::Types::Uint64 queueFlags)
{
    bmqtst::TestHelper::printTestName("SYNC CLOSE QUEUE TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(500);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI,
                                                            queueFlags,
                                                            queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Trigger channel drop");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    BMQTST_ASSERT(
        obj.waitForQueueState(pQueue, bmqimp::QueueState::e_PENDING));
    BMQTST_ASSERT(pQueue->isValid());

    PVV_SAFE("Step 4. Close the queue");
    int rc = obj.session().closeQueue(pQueue, timeout);

    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_SUCCESS);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 5. Try to close the queue again");
    rc = obj.session().closeQueue(pQueue, timeout);

    BMQTST_ASSERT_EQ(rc, bmqt::CloseQueueResult::e_UNKNOWN_QUEUE);

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);

    PVV_SAFE("Step 6. Stop the session");
    BMQTST_ASSERT(obj.stop());

    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
}

static void test23_queueCloseSync()
// ------------------------------------------------------------------------
// SYNC CLOSE QUEUE TEST
//
// Concerns:
//   1. Check the queue closing preceeded with CONNECTION_LOST condition.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open queue for reading
//   3. Drop the test channel
//   4. Close queue and check error code
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueue
//   - closeQueue
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SYNC CLOSE QUEUE TEST");

    PVV_SAFE("Check READER");
    queueCloseSync(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueCloseSync(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueCloseSync(flags);
}

static void
queueAsyncCanceled(int                          lineNum,
                   TestSession::QueueTestStep   queueTestStep,
                   bsls::Types::Uint64          queueFlags,
                   bmqt::SessionEventType::Enum eventType,
                   TestSession::RequestType     retransmittedRequest,
                   bool                     waitOperationResultOnChannelDown,
                   bool                     waitOperationResultOnDisconnect,
                   bool                     waitStateRestoredOnChannelDown,
                   bool                     waitStateRestoredOnDisconnect,
                   bmqimp::QueueState::Enum queueStateAfterDisconnect)
{
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("==================\n"
             << "Starting test " << lineNum << "\n"
             << "==================");

    PVV_SAFE(lineNum << ": Step 1. Starting session...");
    obj.startAndConnect();

    PVV_SAFE(lineNum << ": Step 2. Create the queue");
    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueueOnStep(queueTestStep, queueFlags, timeout);

    PVV_SAFE(lineNum << ": Step 3. Trigger channel drop");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    PVV_SAFE(lineNum << ": Step 4. Waiting queue operation result"
                     << " STATE_RESTORED and CONNECTION_LOST events");

    BMQTST_ASSERT(obj.waitForQueueState(pQueue, queueStateAfterDisconnect));

    if (waitOperationResultOnChannelDown) {
        PVV_SAFE(lineNum << ": Step 4.1. Waiting queue operation result");
        BMQTST_ASSERT(obj.verifyOperationResult(
            eventType,
            bmqp_ctrlmsg::StatusCategory::E_CANCELED));
    }

    if (waitStateRestoredOnChannelDown) {
        PVV_SAFE(lineNum << ": Step 4.2. Waiting STATE_RESTORED event");
        BMQTST_ASSERT(obj.waitStateRestoredEvent());
    }
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    PVV_SAFE(lineNum << ": Step 5. Restore the connection");
    obj.setChannel();

    PVV_SAFE(lineNum << ": Step 6. Waiting RECONNECTED and STATE_RESTORED "
                        "events");
    BMQTST_ASSERT(obj.waitReconnectedEvent());

    if (queueStateAfterDisconnect == bmqimp::QueueState::e_PENDING) {
        PVV_SAFE(lineNum << ": Step 6.1. process queue reopening");
        obj.reopenQueue(pQueue);
    }

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    BMQTST_ASSERT(obj.checkNoEvent());

    if (retransmittedRequest != TestSession::e_REQ_UNDEFINED) {
        PVV_SAFE(lineNum << ": Step 7. Check retransmitted request");
        obj.verifyRequestSent(retransmittedRequest);
    }
    else {
        PVV_SAFE(lineNum << ": Step 7. Setup the queue again");
        obj.arriveAtStep(pQueue, queueTestStep, timeout);
    }

    PVV_SAFE(lineNum << ": Step 8. Stop the session");
    obj.stopGracefully(false);  // don't wait for DISCONNECTED event

    PVV_SAFE(lineNum << ": Step 9. Waiting queue operation result"
                     << " and DISCONNECTED event");

    if (waitOperationResultOnDisconnect) {
        PVV_SAFE(lineNum << ": Step 9.1. Waiting queue operation result");
        BMQTST_ASSERT(obj.verifyOperationResult(
            eventType,
            bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED));
    }

    if (waitStateRestoredOnDisconnect) {
        PVV_SAFE(lineNum << ": Step 9.2. Waiting STATE_RESTORED event");
        BMQTST_ASSERT(obj.waitStateRestoredEvent());
    }

    PVV_SAFE(lineNum << ": Step 10. Waiting DISCONNECTED event");

    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitDisconnectedEvent());
    BMQTST_ASSERT(obj.verifySessionIsStopped());

    BMQTST_ASSERT(obj.waitForQueueState(pQueue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
}

static void queueAsyncCanceled_OPEN_OPENING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_OPEN_OPENING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_OPEN_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                 // pending request type
            bmqt::QueueFlags::e_READ,       // queue type
            k_QUEUE_EVENT,                  // queue operation
            TestSession::e_REQ_OPEN_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_OPENING_OPN,  // queue state after channel
                                                // down
        },
        {
            L_,
            k_REQUEST_TYPE,                 // pending request type
            bmqt::QueueFlags::e_WRITE,      // queue type
            k_QUEUE_EVENT,                  // queue operation
            TestSession::e_REQ_OPEN_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_OPENING_OPN,  // queue state after channel
                                                // down
        },
        {
            L_,
            k_REQUEST_TYPE,                 // pending request type
            k_READ_WRITE,                   // queue type
            k_QUEUE_EVENT,                  // queue operation
            TestSession::e_REQ_OPEN_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_OPENING_OPN,  // queue state after channel
                                                // down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_OPEN_CONFIGURING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_OPEN_CONFIGURING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_OPEN_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                   // pending request type
            bmqt::QueueFlags::e_READ,         // queue type
            k_QUEUE_EVENT,                    // queue operation
            TestSession::e_REQ_CONFIG_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_OPENING_CFG,  // queue state after channel
                                                // down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                   // pending request type
            k_READ_WRITE,                     // queue type
            k_QUEUE_EVENT,                    // queue operation
            TestSession::e_REQ_CONFIG_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_OPENING_CFG,  // queue state after channel
                                                // down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_REOPEN_OPENING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_REOPEN_OPENING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_READ,      // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            k_READ_WRITE,                  // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_REOPEN_CONFIGURING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_REOPEN_CONFIGURING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_READ,      // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            true,   // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            k_READ_WRITE,                  // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            true,   // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_CONFIGURING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_CONFIGURING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                   // pending request type
            bmqt::QueueFlags::e_READ,         // queue type
            k_QUEUE_EVENT,                    // queue operation
            TestSession::e_REQ_CONFIG_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                   // pending request type
            k_READ_WRITE,                     // queue type
            k_QUEUE_EVENT,                    // queue operation
            TestSession::e_REQ_CONFIG_QUEUE,  // retransmitted request
            false,  // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_CONFIGURING_RECONFIGURING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_CONFIGURING_RECONFIGURING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_READ,      // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            k_READ_WRITE,                  // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            false,  // expect queue operation event on channel down
            false,  // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_PENDING,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_CLOSE_CONFIGURING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_CLOSE_CONFIGURING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_READ,      // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            k_READ_WRITE,                  // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void queueAsyncCanceled_CLOSE_CLOSING()
{
    const TestSession::QueueTestStep k_REQUEST_TYPE =
        TestSession::e_CLOSE_CLOSING;
    const bmqt::SessionEventType::Enum k_QUEUE_EVENT =
        bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT;
    bsls::Types::Uint64 k_READ_WRITE = 0;
    bmqt::QueueFlagsUtil::setReader(&k_READ_WRITE);
    bmqt::QueueFlagsUtil::setWriter(&k_READ_WRITE);

    AsyncCancelTestData k_DATA[] = {
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_READ,      // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            bmqt::QueueFlags::e_WRITE,     // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
        {
            L_,
            k_REQUEST_TYPE,                // pending request type
            k_READ_WRITE,                  // queue type
            k_QUEUE_EVENT,                 // queue operation
            TestSession::e_REQ_UNDEFINED,  // retransmitted request
            true,   // expect queue operation event on channel down
            true,   // expect queue operation event on disconnect
            false,  // STATE_RESTORED event after channel down
            false,  // STATE_RESTORED event after disconnect
            bmqimp::QueueState::e_CLOSED,  // queue state after channel down
        },
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const AsyncCancelTestData& test = k_DATA[idx];

        queueAsyncCanceled(test.d_line,
                           test.d_queueTestStep,
                           test.d_queueFlags,
                           test.d_eventType,
                           test.d_retransmittedRequest,
                           test.d_waitOperationResultOnChannelDown,
                           test.d_waitOperationResultOnDisconnect,
                           test.d_waitStateRestoredOnChannelDown,
                           test.d_waitStateRestoredOnDisconnect,
                           test.d_queueStateAfterDisconnect);
    }
}

static void test24_queueAsyncCanceled1()
// ------------------------------------------------------------------------
// QUEUE ASYNC CANCELED TEST
//
// Concerns:
//   1. Check the behavior of the async queue operation when it is
//      interrupted by channel drop and session stop events.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object.
//   2. For each queue state where there is a pending async request:
//   3. Start the session with a test network channel.
//   4. Open a queue in a READ mode.
//   5. Check that ongoing async queue operation returns expected error
//      code when the channel drop happens and when the session is stopped.
//
// Testing manipulators:
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE ASYNC CANCELED TEST 1");

    queueAsyncCanceled_OPEN_OPENING();

    queueAsyncCanceled_OPEN_CONFIGURING();
}

static void test25_sessionFsmTable()
// ------------------------------------------------------------------------
// SESSION FSM TRANSITION TABLE TEST
//
// Concerns:
//   1. Check that session FSM transition table is fully covered by
//      handlers
//
// Plan:
//   1. Create a session object and using its manipulators verify that all
//      session state transitions are reachable
//
// Testing manipulators:
//   - getSessionFsmTransitionTable
//   - start
//   - stop
//   - setChannel
//   - onStartTimeout
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SESSION FSM TRANSITION TABLE TEST");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    scheduler.start();

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    sessionOptions.setNumProcessingThreads(1).setHostHealthMonitor(monitor_sp);

    bmqimp::EventQueue::EventHandlerCallback            emptyEventHandler;
    bsl::vector<bmqimp::BrokerSession::StateTransition> table;
    bslmt::TimedSemaphore                               doneSemaphore;
    bsls::AtomicInt                                     tcpRc(0);

    bmqio::TestChannel testChannel(bmqtst::TestHelperUtil::allocator());
    testChannel.setPeerUri("tcp://testHost:1234");  // for better logging

    bmqimp::BrokerSession obj(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        emptyEventHandler,  // emptyEventHandler,
        bdlf::BindUtil::bind(&transitionCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &table,
                             &tcpRc,
                             &doneSemaphore),
        bmqtst::TestHelperUtil::allocator());

    table = obj.getSessionFsmTransitionTable();

    {
        // STOPPED -> STOPPED
        obj.stop();

        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, bmqt::GenericResult::e_NOT_SUPPORTED);

        // STOPPED -> STOPPED
        obj.stop();
    }

    {
        // STOPPED  -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Reset the channel
        // STARTING -> STOPPED
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    {
        // STOPPED  -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Start timeout
        // STARTING     -> STOPPED
        obj.onStartTimeout();
    }

    {
        // STOPPED  -> STARTING
        tcpRc = 11;

        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, tcpRc);

        // Start failure
        // STARTING -> STOPPED
    }

    tcpRc = 0;

    {
        // STOPPED  -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Set valid channel
        // STARTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // STARTED -> STARTED
        rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Reset the channel
        // STARTING -> RECONNECTING
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

        // Set valid channel
        // RECONNECTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // Generate error on disconnect request
        testChannel.setWriteStatus(bmqio::StatusCategory::e_CANCELED);
        // STARTED         -> CLOSING_SESSION
        // CLOSING_SESSION -> CLOSING_CHANNEL
        obj.stopAsync();

        // Reset the channel
        // CLOSING_CHANNEL -> STOPPED
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    {
        // Make channel writeable
        testChannel.setWriteStatus(bmqio::StatusCategory::e_SUCCESS);

        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Set valid channel
        // STARTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // Reset the channel
        // STARTING -> RECONNECTING
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

        // RECONNECTING    -> CLOSING_CHANNEL
        // CLOSING_CHANNEL -> STOPPED
        obj.stop();
    }

    {
        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Set valid channel
        // STARTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));
        // STARTED -> CLOSING_SESSION
        obj.stopAsync();

        // Reset the channel
        // CLOSING_SESSION -> CLOSING_CHANNEL
        // CLOSING_CHANNEL -> STOPPED
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    {
        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // STARTING -> STARTING
        monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
        monitor->setState(bmqt::HostHealthState::e_HEALTHY);

        // STARTED         -> CLOSING_SESSION
        // CLOSING_SESSION -> CLOSING_CHANNEL
        obj.stopAsync();
    }

    {
        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Set valid channel
        // STARTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // STARTED -> STARTED
        monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
        monitor->setState(bmqt::HostHealthState::e_HEALTHY);

        // STARTED         -> CLOSING_SESSION
        // CLOSING_SESSION -> CLOSING_CHANNEL
        obj.stopAsync();

        // Reset the channel
        // CLOSING_CHANNEL -> STOPPED
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    {
        // STOPPED -> STARTING
        int rc = obj.startAsync();
        BMQTST_ASSERT_EQ(rc, 0);

        // Set valid channel
        // STARTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // Reset the channel
        // STARTING -> RECONNECTING
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());

        monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
        monitor->setState(bmqt::HostHealthState::e_HEALTHY);

        // Set valid channel
        // RECONNECTING -> STARTED
        obj.setChannel(
            bsl::shared_ptr<bmqio::Channel>(&testChannel,
                                            bslstl::SharedPtrNilDeleter()));

        // Generate error on disconnect request
        testChannel.setWriteStatus(bmqio::StatusCategory::e_CANCELED);
        // STARTED         -> CLOSING_SESSION
        // CLOSING_SESSION -> CLOSING_CHANNEL
        obj.stopAsync();

        // Reset the channel
        // CLOSING_CHANNEL -> STOPPED
        obj.setChannel(bsl::shared_ptr<bmqio::Channel>());
    }

    PVV_SAFE("Waiting for empty transition table");
    BMQTST_ASSERT(waitRealTime(&doneSemaphore));

    scheduler.cancelAllEventsAndWait();
    scheduler.stop();
}

static void test26_openCloseMultipleSubqueues()
// ------------------------------------------------------------------------
// OPEN CLOSE MULTIPLE SUBQUEUES TEST
//
// Concerns:
//   1. Check that close queue request contains proper isFinal flag value
//      when opening and closing subqueues in positive use case
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open the queues
//   3. Close the queues
//   4. Stop the session
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueueAsync
//   - closeQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> queueFoo = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=foo",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBar = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=bar",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBaz = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=baz",
        bmqt::QueueFlags::e_READ);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queues");

    PVVV_SAFE("Open FOO subqueue");
    obj.openQueue(queueFoo);

    PVVV_SAFE("Open BAR subqueue");
    obj.openQueue(queueBar);

    PVVV_SAFE("Open BAZ subqueue");
    obj.openQueue(queueBaz);

    PVV_SAFE("Step 3. Close the queues");

    PVVV_SAFE("Close FOO subqueue");
    obj.closeQueue(queueFoo, timeout, false);

    PVVV_SAFE("Close BAR subqueue");
    obj.closeQueue(queueBar, timeout, false);

    PVVV_SAFE("Close BAZ subqueue (isFinal flag should be true)");
    obj.closeQueue(queueBaz, timeout, true);

    PVV_SAFE("Step 4. Stop the session");
    obj.stopGracefully();
}

static void test27_openCloseMultipleSubqueuesWithErrors()
// ------------------------------------------------------------------------
// OPEN CLOSE MULTIPLE SUBQUEUES TEST WITH ERRORS
//
// Concerns:
//   1. Check that close queue request contains proper isFinal flag value
//      when opening and closing subqueues with errors
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open the queues
//   3. Close the queues
//   4. Stop the session
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueueAsync
//   - closeQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "OPEN CLOSE MULTIPLE SUBQUEUES WITH ERRORS TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> queueValid = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=valid",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueFailedOpen = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=failedopen",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBadOpen = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=badopen",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueLateOpen = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=lateopen",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueFailedOpenConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=failedopenconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBadOpenConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=badopenconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueLateOpenConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=lateopenconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueFailedCloseConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=failedcloseconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBadCloseConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=badcloseconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueLateCloseConfig = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=latecloseconfig",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueFailedClose = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=failedclose",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueBadClose = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=badclose",
        bmqt::QueueFlags::e_READ);

    bsl::shared_ptr<bmqimp::Queue> queueLateClose = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=lateclose",
        bmqt::QueueFlags::e_READ);

    PVV_SAFE(L_ << " Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE(L_ << " Step 2. Open the queues");

    PVVV_SAFE(L_ << " Open valid queue");
    obj.openQueue(queueValid);

    PVVV_SAFE(L_ << " Open the queue with rejected open queue request");
    obj.openQueueWithError(queueFailedOpen,
                           TestSession::e_REQ_OPEN_QUEUE,
                           TestSession::e_ERR_NOT_SENT,
                           timeout);

    PVVV_SAFE(L_ << " Open the queue with bad open queue response");
    obj.openQueueWithError(queueBadOpen,
                           TestSession::e_REQ_OPEN_QUEUE,
                           TestSession::e_ERR_BAD_RESPONSE,
                           timeout);

    PVVV_SAFE(L_ << " Open the queue with late open queue response");
    obj.openQueueWithError(queueLateOpen,
                           TestSession::e_REQ_OPEN_QUEUE,
                           TestSession::e_ERR_LATE_RESPONSE,
                           timeout);
    PVVV_SAFE(
        L_
        << " Verify close queue request has been sent for late open response");
    obj.verifyCloseRequestSent(false);

    PVVV_SAFE(L_ << " Open the queue with rejected open configure request");
    obj.openQueueWithError(queueFailedOpenConfig,
                           TestSession::e_REQ_CONFIG_QUEUE,
                           TestSession::e_ERR_NOT_SENT,
                           timeout);

    PVVV_SAFE(L_ << " Open the queue with bad open configure response");
    obj.openQueueWithError(queueBadOpenConfig,
                           TestSession::e_REQ_CONFIG_QUEUE,
                           TestSession::e_ERR_BAD_RESPONSE,
                           timeout);
    PVVV_SAFE(L_ << " Verify close queue request has been sent for bad "
                    "configure response");
    obj.verifyCloseRequestSent(false);

    PVVV_SAFE(L_ << " Open the queue with late open configure response");
    obj.openQueueWithError(queueLateOpenConfig,
                           TestSession::e_REQ_CONFIG_QUEUE,
                           TestSession::e_ERR_LATE_RESPONSE,
                           timeout);
    PVVV_SAFE(L_ << " Verify close queue request has been sent for late "
                    "configure response");
    obj.verifyCloseRequestSent(false);

    PVVV_SAFE(L_ << " Open the queue with rejected close configure request");
    obj.openQueue(queueFailedCloseConfig, timeout);

    PVVV_SAFE(L_ << " Open the queue with bad close configure request");
    obj.openQueue(queueBadCloseConfig, timeout);

    PVVV_SAFE(L_ << " Open the queue with late close configure request");
    obj.openQueue(queueLateCloseConfig, timeout);

    PVVV_SAFE(L_ << " Open the queue with rejected close request");
    obj.openQueue(queueFailedClose, timeout);

    PVVV_SAFE(L_ << " Open the queue with bad close response");
    obj.openQueue(queueBadClose, timeout);

    PVVV_SAFE(L_ << " Open the queue with late close response");
    obj.openQueue(queueLateClose, timeout);

    PVV_SAFE(L_ << " Step 3. Close the queues");

    PVVV_SAFE(L_ << " Close the queue with rejected close configure request");
    obj.closeQueueWithError(queueFailedCloseConfig,
                            TestSession::e_REQ_CONFIG_QUEUE,
                            TestSession::e_ERR_NOT_SENT,
                            false,
                            timeout);

    PVVV_SAFE(L_ << " Close the queue with bad close configure response");
    obj.closeQueueWithError(queueBadCloseConfig,
                            TestSession::e_REQ_CONFIG_QUEUE,
                            TestSession::e_ERR_BAD_RESPONSE,
                            false,  // Ignored, close request is not sent
                            timeout);

    PVVV_SAFE(L_ << " Close the queue with late close configure response");
    obj.closeQueueWithError(queueLateCloseConfig,
                            TestSession::e_REQ_CONFIG_QUEUE,
                            TestSession::e_ERR_LATE_RESPONSE,
                            false,
                            timeout);

    PVVV_SAFE(L_ << " Close the queue with rejected close request");
    obj.closeQueueWithError(queueFailedClose,
                            TestSession::e_REQ_CLOSE_QUEUE,
                            TestSession::e_ERR_NOT_SENT,
                            false,
                            timeout);

    PVVV_SAFE(L_ << " Close the queue with bad close response");
    obj.closeQueueWithError(queueBadClose,
                            TestSession::e_REQ_CLOSE_QUEUE,
                            TestSession::e_ERR_BAD_RESPONSE,
                            false,
                            timeout);

    PVVV_SAFE(L_ << " Close the queue with late close response");
    obj.closeQueueWithError(queueLateClose,
                            TestSession::e_REQ_CLOSE_QUEUE,
                            TestSession::e_ERR_LATE_RESPONSE,
                            false,
                            timeout);

    PVVV_SAFE(L_ << " Close valid queue");
    // Since the queues above are not in e_CLOSED state,
    // the valid queue close request isFinal flag will not be
    // set to true
    PV_SAFE(L_ << " [BUG] isFinal parameter should be true");
    obj.closeQueue(queueValid, timeout, false);  // TODO: false -> true

    PVV_SAFE(L_ << " Step 4. Stop the session");
    obj.stopGracefully();
}

static void
queueLateAsyncCanceled(int                               testId,
                       bsls::Types::Uint64               queueFlags,
                       TestSession::LateResponseTestStep step,
                       bmqimp::QueueState::Enum queueStateAfterDisconnect)

{
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    // Set timeouts used for implicit reopening, reconfiguring and closing
    sessionOptions.setNumProcessingThreads(1)
        .setOpenQueueTimeout(timeout)
        .setConfigureQueueTimeout(timeout)
        .setCloseQueueTimeout(timeout);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    PVV_SAFE(testId << ": Step 1. Starting session...");
    obj.startAndConnect();

    PVV_SAFE(testId << ": Step 2. Create the queue");
    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueueOnStep(step,
                                                                  queueFlags,
                                                                  timeout);

    PVV_SAFE(testId << ": Step 3. Trigger channel drop");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    PVV_SAFE(testId << ": Step 4. Waiting queue operation result"
                    << " STATE_RESTORED and CONNECTION_LOST events");
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    BMQTST_ASSERT(obj.waitForQueueState(pQueue, queueStateAfterDisconnect));

    PVV_SAFE(testId << ": Step 5. Restore the connection");
    obj.setChannel();

    PVV_SAFE(
        testId << ": Step 6. Waiting RECONNECTED and STATE_RESTORED events");

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    if (queueStateAfterDisconnect == bmqimp::QueueState::e_PENDING) {
        PVV_SAFE(testId << ": Step 6.1. process queue reopening");
        obj.reopenQueue(pQueue);
    }

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE(testId << ": Step 7. Setup the queue again");

    if (queueStateAfterDisconnect == bmqimp::QueueState::e_PENDING) {
        PVV_SAFE(testId << ": Step 7.1. reuse the queue");
        obj.arriveAtLateResponseStep(pQueue, step, timeout);
    }
    else if (queueStateAfterDisconnect == bmqimp::QueueState::e_CLOSING_CLS) {
        PVV_SAFE(testId << ": Step 7.2. close and re-create the queue");
        obj.closeDeconfiguredQueue(pQueue);
        pQueue = obj.createQueueOnStep(step, queueFlags, timeout);
    }
    else {
        PVV_SAFE(testId << ": Step 7.3. re-create the queue");
        pQueue = obj.createQueueOnStep(step, queueFlags, timeout);
    }

    PVV_SAFE(testId << ": Step 8. Stop the session");
    obj.stopGracefully();

    BMQTST_ASSERT(obj.waitForQueueState(pQueue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
}

static void test28_queueLateAsyncCanceledReader1()
// ------------------------------------------------------------------------
// QUEUE LATE ASYNC CANCELED READER TEST 1-5
//
// Concerns:
//   1. Check the behavior of the async queue operation caused by the late
//      broker response and interrupted by channel drop and session stop
//      events.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object.
//   2. For each queue state where there is a pending async request:
//   3. Start the session with a test network channel.
//   4. Open a queue in a READ mode.
//   5. Check that ongoing async queue operation returns expected error
//      code when the channel drop happens and when the session is stopped.
//
// Testing manipulators:
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED READER TEST 1");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_OPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_OPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_CLOSED);
}

static void test29_queueLateAsyncCanceledWriter1()
// ------------------------------------------------------------------------
// QUEUE LATE ASYNC CANCELED WRITER TEST 1-5
//
// Concerns:
//   1. Check the behavior of the async queue operation caused by the late
//      broker response and interrupted by channel drop and session stop
//      events.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object.
//   2. For each queue state where there is a pending async request:
//   3. Start the session with a test network channel.
//   4. Open a queue in a WRITE mode.
//   5. Check that ongoing async queue operation returns expected error
//      code when the channel drop happens and when the session is stopped.
//
// Testing manipulators:
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED WRITER TEST 1");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_OPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_OPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_PENDING);
}

static void test30_queueLateAsyncCanceledHybrid1()
// ------------------------------------------------------------------------
// QUEUE LATE ASYNC CANCELED HYBRID TEST 1-5
//
// Concerns:
//   1. Check the behavior of the async queue operation caused by the late
//      broker response and interrupted by channel drop and session stop
//      events.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object.
//   2. For each queue state where there is a pending async request:
//   3. Start the session with a test network channel.
//   4. Open a queue in a READ and WRITE mode.
//   5. Check that ongoing async queue operation returns expected error
//      code when the channel drop happens and when the session is stopped.
//
// Testing manipulators:
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED HYBRID TEST 1");

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_OPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_OPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_CLOSED);
}

static void queueDoubleOpenUri(bsls::Types::Uint64 queueFlags)
{
    const bsls::TimeInterval timeout = bsls::TimeInterval(500);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue1 = obj.createQueue(k_URI,
                                                             queueFlags,
                                                             queueOptions);
    bsl::shared_ptr<bmqimp::Queue> pQueue2 = obj.createQueue(k_URI,
                                                             queueFlags,
                                                             queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue1->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue1->isValid(), false);
    BMQTST_ASSERT_EQ(pQueue2->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue2->isValid(), false);

    PVV_SAFE("Step 2. Open the first  queue");
    obj.openQueue(pQueue1, timeout);

    PVV_SAFE("Step 3. Open the second queue and check the error");
    int rc = obj.session().openQueue(pQueue2, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_ALREADY_OPENED);

    BMQTST_ASSERT_EQ(pQueue1->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(pQueue1->isValid());

    BMQTST_ASSERT_EQ(pQueue2->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue2->isValid(), false);

    PVV_SAFE("Step 4. Stop the session");
    obj.stopGracefully();
}

static void test31_queueDoubleOpenUri()
// ------------------------------------------------------------------------
// QUEUE DOUBLE OPEN URI TEST
//
// Concerns:
//   1. Check two queues are opened with the same URI.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open queue for reading.
//   3. Open another queue with the same URI for reading and check error
//      code is e_ALREADY_OPENED.
//   4. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueue
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE DOUBLE OPEN URI TEST");

    PVV_SAFE("Check READER");
    queueDoubleOpenUri(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueDoubleOpenUri(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueDoubleOpenUri(flags);
}

static void queueDoubleOpenCorrelationId(bsls::Types::Uint64 queueFlags)
{
    const char k_URI1[] = "bmq://ts.trades.myapp/my.queue1?id=my.app";
    const char k_URI2[] = "bmq://ts.trades.myapp/my.queue2?id=my.app";

    const bsls::TimeInterval  timeout = bsls::TimeInterval(500);
    const bmqt::CorrelationId corId   = bmqt::CorrelationId::autoValue();
    bmqt::SessionOptions      sessionOptions;
    bmqt::QueueOptions        queueOptions;
    bdlmt::EventScheduler     scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue1 = obj.createQueue(k_URI1,
                                                             queueFlags,
                                                             queueOptions);
    bsl::shared_ptr<bmqimp::Queue> pQueue2 = obj.createQueue(k_URI2,
                                                             queueFlags,
                                                             queueOptions);

    pQueue1->setCorrelationId(corId);
    pQueue2->setCorrelationId(corId);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    BMQTST_ASSERT_EQ(pQueue1->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue1->isValid(), false);
    BMQTST_ASSERT_EQ(pQueue2->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue2->isValid(), false);

    PVV_SAFE("Step 2. Open the first  queue");
    obj.openQueue(pQueue1, timeout);

    PVV_SAFE("Step 3. Open the second queue and check the error");
    int rc = obj.session().openQueue(pQueue2, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_CORRELATIONID_NOT_UNIQUE);

    BMQTST_ASSERT_EQ(pQueue1->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(pQueue1->isValid());

    BMQTST_ASSERT_EQ(pQueue2->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue2->isValid(), false);

    PVV_SAFE("Step 4. Stop the session");
    obj.stopGracefully();
}

static void test32_queueDoubleOpenCorrelationId()
// ------------------------------------------------------------------------
// QUEUE DOUBLE OPEN CORRELATION ID TEST
//
// Concerns:
//   1. Check two queues are opened with the same CorrelationId.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open queue for reading.
//   3. Open another queue with the same CorrelationId for reading and
//      check error code is e_CORRELATIONID_NOT_UNIQUE.
//   4. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueue
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE DOUBLE OPEN CORRELATION ID TEST");

    PVV_SAFE("Check READER");
    queueDoubleOpenCorrelationId(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueDoubleOpenCorrelationId(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueDoubleOpenCorrelationId(flags);
}

static void test33_queueNackTest()
// ------------------------------------------------------------------------
// QUEUE NACK TEST
//
// Concerns:
//   1. Check the unACKed PUT message generates NACK on channel down
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open queue for writing and post a PUT message.
//   3. Trigger channel down and verify no NACK message arrives.
//   4. Restore the channel and verify the PUT message is retransmitted
//      once the queue is reopened.
//   5. Trigger channel down again and close the queue.
//   6. Verify that NACK message arrives.
//   7. Stop the session.
//
// Testing manipulators:
//   - start
//   - openQueue
//   - post
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE NACK TEST");

    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    const bsls::TimeInterval       timeout = bsls::TimeInterval(5);
    bmqt::SessionOptions           sessionOptions;
    bmqt::QueueOptions             queueOptions;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder          eventBuilder(&blobSpPool,
                                       bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId      corrId(243);
    bmqt::MessageGUID     guid = bmqp::MessageGUIDGenerator::testGUID();
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Send PUT message");

    // Create Event object
    bsl::shared_ptr<bmqimp::Event> putEvent = obj.session().createEvent();

    // Add CorrelationId
    bmqimp::MessageCorrelationIdContainer* idsContainer =
        putEvent->messageCorrelationIdContainer();
    const bmqp::QueueId qid(pQueue->id(), pQueue->subQueueId());
    idsContainer->add(guid, corrId, qid);

    // Set flags
    int phFlags = 0;
    bmqp::PutHeaderFlagUtil::setFlag(&phFlags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    // Create the event blob
    eventBuilder.startMessage();
    eventBuilder.setMessageGUID(guid)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);

    bmqt::EventBuilderResult::Enum rc = eventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using just event blob
    int res = obj.session().post(*eventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    PVV_SAFE("Step 4. Verify PUT event is sent");
    bmqp::Event rawEvent(bmqtst::TestHelperUtil::allocator());
    obj.getOutboundEvent(&rawEvent);

    BMQTST_ASSERT(rawEvent.isPutEvent());

    PVV_SAFE("Step 5. Trigger channel drop and verify no NACK event is sent");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    // Restore the connection
    obj.setChannel();

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    PVV_SAFE("Step 6. Reopen the queue");
    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 7. Verify PUT event is sent again");
    rawEvent.clear();
    obj.getOutboundEvent(&rawEvent);

    BMQTST_ASSERT(rawEvent.isPutEvent());

    PVV_SAFE("Step 8. Trigger channel drop, close the queue and verify NACK "
             "event");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    obj.session().closeQueueAsync(pQueue, timeout);

    bsl::shared_ptr<bmqimp::Event> nackEvent = obj.waitAckEvent();

    BMQTST_ASSERT(nackEvent);

    const int k_ACK_STATUS_UNKNOWN = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_UNKNOWN);

    bmqp::AckMessageIterator* iter = nackEvent->ackMessageIterator();
    BMQTST_ASSERT_EQ(1, iter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), iter->message().queueId());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_UNKNOWN, iter->message().status());
    BMQTST_ASSERT_EQ(1, nackEvent->numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrId, nackEvent->correlationId(0));
    BMQTST_ASSERT_EQ(0, iter->next());

    PVV_SAFE("Step 9. Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                               pQueue);

    PVV_SAFE("Step 10. Stop the session when not connected");
    BMQTST_ASSERT(obj.stop());

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
    rawEvent.clear();
}

static void reopenError(bsls::Types::Uint64 queueFlags)
{
    bmqtst::TestHelper::printTestName("REOPEN ERROR TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> queue = obj.createQueue(k_URI,
                                                           queueFlags,
                                                           queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Check reopen 1st part error");
    {
        PVV_SAFE("Step 3. Open the queue");
        obj.openQueue(queue);

        PVV_SAFE("Step 4. Reset the channel");
        obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

        BMQTST_ASSERT(obj.waitConnectionLostEvent());

        BMQTST_ASSERT(
            obj.waitForQueueState(queue, bmqimp::QueueState::e_PENDING));
        BMQTST_ASSERT(queue->isValid());

        // Set write error condition
        obj.channel().setWriteStatus(bmqio::StatusCategory::e_GENERIC_ERROR);

        // Restore the connection
        obj.setChannel();

        BMQTST_ASSERT(obj.waitReconnectedEvent());

        PVV_SAFE("Step 5. Verify reopen request was sent");
        obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);

        BMQTST_ASSERT(obj.verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
            bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED));

        BMQTST_ASSERT(obj.waitStateRestoredEvent());

        PVV_SAFE("Step 6. Verify the queue gets closed");
        BMQTST_ASSERT(obj.waitForQueueRemoved(queue, timeout));

        // Set normal write status
        obj.channel().setWriteStatus(bmqio::StatusCategory::e_SUCCESS);
    }

    PVV_SAFE("Step 7. Check reopen 2st part error");
    {
        PVV_SAFE("Step 8. Open the queue");
        obj.openQueue(queue);

        PVV_SAFE("Step 9. Reset the channel");
        obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

        BMQTST_ASSERT(obj.waitConnectionLostEvent());

        BMQTST_ASSERT(
            obj.waitForQueueState(queue, bmqimp::QueueState::e_PENDING));
        BMQTST_ASSERT(queue->isValid());

        // Restore the connection
        obj.setChannel();

        BMQTST_ASSERT(obj.waitReconnectedEvent());

        PVV_SAFE("Step 10. Verify reopen request was sent");
        bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
            TestSession::e_REQ_OPEN_QUEUE);

        // Set write error condition
        obj.channel().setWriteStatus(bmqio::StatusCategory::e_GENERIC_ERROR);

        PVV_SAFE("Step 11. Send reopen response");
        obj.sendResponse(request);

        // Configuring is applicable only for the reader
        if (bmqt::QueueFlagsUtil::isReader(queue->flags())) {
            PVV_SAFE("Step 12. Verify configure request was sent");
            obj.verifyRequestSent(TestSession::e_REQ_CONFIG_QUEUE);

            BMQTST_ASSERT(obj.verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED));

            BMQTST_ASSERT(obj.waitStateRestoredEvent());

            PVV_SAFE("Step 13. Verify the queue is e_CLOSING_CLS");
            obj.verifyRequestSent(TestSession::e_REQ_CLOSE_QUEUE);

            BMQTST_ASSERT(
                obj.waitForQueueState(queue,
                                      bmqimp::QueueState::e_CLOSING_CLS));
            BMQTST_ASSERT_EQ(queue->isValid(), false);
        }
        else {
            BMQTST_ASSERT(obj.verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
            BMQTST_ASSERT(obj.waitStateRestoredEvent());
            BMQTST_ASSERT(
                obj.waitForQueueState(queue, bmqimp::QueueState::e_OPENED));
            BMQTST_ASSERT(queue->isValid());
        }

        // Set normal write status
        obj.channel().setWriteStatus(bmqio::StatusCategory::e_SUCCESS);
    }

    PVV_SAFE("Step 14. Stop the session");
    obj.stopGracefully();
}

static void test34_reopenError()
// ------------------------------------------------------------------------
// REOPEN ERROR TEST
//
// Concerns:
//   1. Check that queue reopen error due to channel failure is handled
//      properly.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open the queue, reset the channel and trigger reopening procedure.
//   3. Set error write status for the channel and check 1st part of the
//      queue reopening.
//   4. Repeat step 2 and 3 for the 2nd part of reopening.
//   5. Close the session.
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REOPEN ERROR TEST");

    PVV_SAFE("Check READER");
    reopenError(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    reopenError(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    reopenError(flags);
}

static void test35_hostHealthMonitoring()
// ------------------------------------------------------------------------
// HOST HEALTH MONITORING TEST
//
// Concerns:
//   1. Check that e_HOST_HEALTH_CHANGE session events are correctly issued
//      in response to changes in host health, as inferred by an installed
//      'HostHealthMonitor' object.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Explicitly set the host health to a new value.
//   4. Verify that a HOST_UNHEALTHY session event is issued, with the
//      correct new 'HostHealthState' provided for the event status.
//   5. Validate closing session removes callback from HostHealthMonitor.
//   6. Validate reopening session re-adds callback.
//   7. Verify that opening queue on unhealthy host initializes the queue
//      into suspended state (without configuring with broker).
//   8. Verify that queue resumes when host again becomes healthy.
//   9. Verify that resumed queue can again reconfigure with broker.
//  10. Verify that resumed queue again suspends on newly unhealthy host.
//  11. Verify that a host-health request is deferred if another configure
//      is in progress, and thereafter executes.
//  12. Ensure that HOST_UNHEALTHY and HOST_HEALTH_RESUMED events do not
//      fire during periods of host-health flapping, unless moving from a
//      fully healthy to unhealthy state, or visa versa.
//  13. Re-close session.
//
// Testing manipulators:
//   - start
//   - setHostHealthMonitor
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName("HOST HEALTH MONITORING TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1).setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. Set up bmqimp::BrokerSession with "
             "bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Set new host health state");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);

    PVV_SAFE("Step 4. Ensure that event arrives");
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    PVV_SAFE(
        "Ensure that flipping to unknown state does not re-publish event");
    monitor->setState(bmqt::HostHealthState::e_UNKNOWN);
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 5. Stop the session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 6. Validate reopening session re-adds callback");
    obj.startAndConnect(true);
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 7. Verify that opening a queue initializes into suspend "
             "state");
    bmqt::QueueOptions queueOptions;
    queueOptions.setSuspendsOnBadHostHealth(true);

    bsl::shared_ptr<bmqimp::Queue> queue = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=foo",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    BMQTST_ASSERT(!queue->isSuspended());

    obj.openQueue(queue, bsls::TimeInterval(5), true);
    BMQTST_ASSERT(queue->isSuspended());

    size_t before = obj.channel().writeCalls().size();
    BMQTST_ASSERT(obj.checkNoEvent());
    queueOptions.setMaxUnconfirmedMessages(321);
    BMQTST_ASSERT_EQ(bmqt::GenericResult::e_SUCCESS,
                     obj.session().configureQueue(queue,
                                                  queueOptions,
                                                  bsls::TimeInterval(5)));
    BMQTST_ASSERT_EQ(queue->options().maxUnconfirmedMessages(), 321);
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), before);

    queueOptions.setMaxUnconfirmedMessages(123);
    BMQTST_ASSERT_EQ(bmqt::GenericResult::e_SUCCESS,
                     obj.session().configureQueueAsync(queue,
                                                       queueOptions,
                                                       bsls::TimeInterval(5)));
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(queue->options().maxUnconfirmedMessages(), 123);
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), before);

    PVV_SAFE("Step 8. "
             "Verify that queue resumes when the host becomes healthy");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);

    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    bmqp_ctrlmsg::ConsumerInfo ci;

    getConsumerInfo(&ci, request);
    BMQTST_ASSERT_EQ(ci.consumerPriority(), 0);
    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 123);

    BMQTST_ASSERT(queue->isSuspended());
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(!queue->isSuspended());
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());

    PVV_SAFE("Step 9. "
             "Verify that resumed queue can again configure with broker");
    BMQTST_ASSERT_EQ(bmqt::GenericResult::e_SUCCESS,
                     obj.session().configureQueueAsync(queue,
                                                       queueOptions,
                                                       bsls::TimeInterval(5)));
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    PVV_SAFE("Step 10. Queue deconfigures on host again becoming unhealthy");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    BMQTST_ASSERT(!queue->isSuspended());
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    BMQTST_ASSERT(queue->isSuspended());

    PVV_SAFE("Step 11. "
             "Queue defers host-health changes if configure in progress");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    getConsumerInfo(&ci, request);
    // Do not care about 'subId'
    BMQTST_ASSERT_EQ(ci.consumerPriority(), 0);
    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 123);
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(!queue->isSuspended());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    getConsumerInfo(&ci, request);
    BMQTST_ASSERT_EQ(ci.consumerPriority(),
                     bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);

    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    BMQTST_ASSERT(obj.checkNoEvent());
    // No HOST_HEALTH_RESUMED or additional HOST_UNHEALTHY, since session
    // never returned to a fully healthy state: All queues were never
    // resumed concurrently with a healthy host state.

    PVV_SAFE("Step 12. Flap the host health; ensure a single session event");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    getConsumerInfo(&ci, request);

    BMQTST_ASSERT_EQ(ci.consumerPriority(), 0);
    // request := resume
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    obj.sendResponse(request);
    // responding to resume
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    getConsumerInfo(&ci, request);
    BMQTST_ASSERT_EQ(ci.consumerPriority(),
                     bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);
    // request := suspend
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    obj.sendResponse(request);
    // responding to suspend
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    getConsumerInfo(&ci, request);
    // request := resume
    obj.sendResponse(request);
    // responding to resume
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());
    // This time, we arrive back into a state in which the host is healthy
    // and all queues have managed to resume: We therefore issue the
    // HOST_HEALTH_RESTORED event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 13. Re-close session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void test36_closingAfterConfigTimeout()
// ------------------------------------------------------------------------
// CLOSING AFTER CONFIGURE TIMEOUT TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it handles
//      late standalone configure response while the queue is being closed.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object and start the
//      session.
//   2. Open queue and initiate standalone configuring.
//   3. Wait for configure timeout and initiate queue closing.
//   4. Send late configure response and verify that it doesn't disturb
//      closing procedure.
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - configureQueueAsync
//   - closeQueueAsync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLOSING AFTER CONFIGURE TIMEOUT TEST");

    const bsls::TimeInterval     timeout = bsls::TimeInterval(15);
    bmqt::QueueOptions           queueOptions;
    bmqt::SessionOptions         sessionOptions;
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler        scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                    testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Test step: Start the session");
    obj.startAndConnect();

    bsl::shared_ptr<bmqimp::Queue> queue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ, queueOptions);

    PVV_SAFE("Test step: Open the queue");
    obj.openQueue(queue);

    PVV_SAFE("Test step: Do standalone configure");
    int rc = obj.session().configureQueueAsync(queue, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

    // Get standalone configure request
    currentRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    PVV_SAFE("Test step: Wait for configure timeout");

    // Emulate request timeout.
    obj.advanceTime(timeout);

    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);
    BMQTST_ASSERT(queue->isValid());

    PVV_SAFE("Test step: Do queue closing");
    rc = obj.session().closeQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    BMQTST_ASSERT(
        obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSING_CFG));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Send late standalone configure response");
    obj.sendResponse(currentRequest);

    PVV_SAFE("Test step: Send deconfigure response");
    currentRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    obj.sendResponse(currentRequest);

    BMQTST_ASSERT(
        obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSING_CLS));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Send close response");
    currentRequest = obj.verifyCloseRequestSent(true);  // isFinal

    obj.sendResponse(currentRequest);

    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, queue);

    PVV_SAFE("Test step: Stop the session");
    obj.stopGracefully();
}

static void test37_closingPendingConfig()
// ------------------------------------------------------------------------
// CLOSING WHILE PENDING CONFIGURE TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it closes a
//      queue while standalone configure request is in progress.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object and start the
//      session.
//   2. Open queue and initiate standalone configuring.
//   3. Initiate queue closing.
//   4. Verify the configure request is rejected and closing procedure can
//      be finished successfully.
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - configureQueueAsync
//   - closeQueueAsync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CLOSING WHILE PENDING CONFIGURE TEST");

    const bsls::TimeInterval     timeout = bsls::TimeInterval(15);
    bmqt::QueueOptions           queueOptions;
    bmqt::SessionOptions         sessionOptions;
    bmqp_ctrlmsg::ControlMessage currentRequest(
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Test step: Start the session");
    obj.startAndConnect();

    bsl::shared_ptr<bmqimp::Queue> queue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ, queueOptions);

    PVV_SAFE("Test step: Open the queue");
    obj.openQueue(queue);

    PVV_SAFE("Test step: Do standalone configure");
    int rc = obj.session().configureQueueAsync(queue, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

    // Get standalone configure request
    currentRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    PVV_SAFE("Test step: Do queue closing");
    rc = obj.session().closeQueueAsync(queue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    BMQTST_ASSERT(
        obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSING_CFG));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Verify configure request is rejected");

    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED));

    PVV_SAFE("Test step: Send standalone configure response");
    // It will be ignored by the request manager
    obj.sendResponse(currentRequest);

    PVV_SAFE("Test step: Send deconfigure response");
    currentRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    obj.sendResponse(currentRequest);

    BMQTST_ASSERT(
        obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSING_CLS));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Send close response");
    currentRequest = obj.verifyCloseRequestSent(true);  // ifFinal

    obj.sendResponse(currentRequest);

    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_CLOSED));
    BMQTST_ASSERT_EQ(queue->isValid(), false);

    PVV_SAFE("Test step: Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS, queue);

    PVV_SAFE("Test step: Stop the session");
    obj.stopGracefully();
}

static void eventCallbackMockup(bsl::shared_ptr<bmqimp::Event>* resultEvent,
                                int*                            count,
                                bmqimp::BrokerSession*          session,
                                const bsl::shared_ptr<bmqimp::Event>& event)
{
    PV_SAFE("Got an event: " << *event);

    BSLS_ASSERT_SAFE(resultEvent);
    BSLS_ASSERT_SAFE(count);

    ++(*count);

    *resultEvent = event;

    if (session) {
        // If we call 'nextEvent', it will block after calling 'cb' waiting for
        // another, non-callback event.  Enqueue dummy event to unblock that.
        // In 'eventHandler' mode, the dummy event serves as synchronization
        // point.
        session->enqueueSessionEvent(bmqt::SessionEventType::e_UNDEFINED);
    }
}

static void responderMockup(TestSession*                   obj,
                            int                            count,
                            const TestSession::RequestType type[])
{
    for (int i = 0; i < count; ++i) {
        bmqp_ctrlmsg::ControlMessage request = obj->getNextOutboundRequest(
            type[i]);
        if (TestSession::e_REQ_DISCONNECT == type[i]) {
            bmqp_ctrlmsg::ControlMessage response(
                bmqtst::TestHelperUtil::allocator());
            response.rId().makeValue(request.rId().value());
            response.choice().makeDisconnectResponse();

            obj->sendControlMessage(response);
            BMQTST_ASSERT(obj->waitForChannelClose(bsls::TimeInterval(1)));

            break;
        }
        else {
            obj->sendResponse(request);
        }
    }
}

static void test_syncOpenConfigureClose(bool withHandler)
{
    // Plan:
    //   1. Create bmqimp::BrokerSession test wrapper object with Event Handler
    //      and start the session.
    //   2. Sync Open queue and verify the operation succeeds only after the
    //      handler picks up the internal event.
    //   3. Repeat for configure.
    //   4. Repeat for close.
    //   5. Stop the session
    //
    // Testing manipulators:
    //   - start
    //   - openQueueSync
    //   - configureQueueSync
    //   - closeQueueSync
    //   - stop
    // ------------------------------------------------------------------------
    const bsls::TimeInterval       timeout = bsls::TimeInterval(1);
    bmqt::QueueOptions             queueOptions;
    bsl::shared_ptr<bmqimp::Event> eventSp;
    bmqt::SessionOptions           sessionOptions;
    bmqp_ctrlmsg::ControlMessage   currentRequest(
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestSession           obj(sessionOptions, scheduler, withHandler);

    sessionOptions.setNumProcessingThreads(1);

    PVV_SAFE("1: Start the session");
    obj.startAndConnect();

    PVV_SAFE("2: Open the queue");

    bsl::shared_ptr<bmqimp::Queue> queueSp =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ, queueOptions);
    int                                calls = 0;
    const bmqimp::Event::EventCallback cb    = bdlf::BindUtil::bind(
        &eventCallbackMockup,
        &eventSp,
        &calls,
        static_cast<bmqimp::BrokerSession*>(0),
        bdlf::PlaceHolders::_1);  // event

    bslmt::ThreadUtil::Handle threadHandle;
    TestSession::RequestType  requestType[] = {TestSession::e_REQ_OPEN_QUEUE,
                                               TestSession::e_REQ_CONFIG_QUEUE,
                                               TestSession::e_REQ_CONFIG_QUEUE,
                                               TestSession::e_REQ_CONFIG_QUEUE,
                                               TestSession::e_REQ_CLOSE_QUEUE,
                                               TestSession::e_REQ_DISCONNECT};
    bslmt::ThreadUtil::createWithAllocator(
        &threadHandle,
        bdlf::BindUtil::bind(&responderMockup, &obj, 6, requestType),
        bmqtst::TestHelperUtil::allocator());

    obj.session().openQueueSync(queueSp, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 1);
    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("3: Configure");
    calls = 0;
    obj.session().configureQueueSync(queueSp, queueOptions, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 1);
    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("4: Close");
    calls = 0;

    obj.session().closeQueueSync(queueSp, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 1);
    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("5: Stop");
    calls = 0;

    if (withHandler) {
        BMQTST_ASSERT(obj.stop());
    }
    else {
        // since this is nextEvent mode, 'stop' unblock _before_ the event
        obj.session().stop();
        BMQTST_ASSERT(obj.waitDisconnectedEvent());
    }

    BMQTST_ASSERT_EQ(queueSp->isValid(), false);
    BMQTST_ASSERT_EQ(calls, 0);
    bslmt::ThreadUtil::join(threadHandle);
    eventSp.clear();
}

static void test_asyncOpenConfigureClose(bool withHandler)
{
    // Plan:
    //   1. Create bmqimp::BrokerSession test wrapper object with Event Handler
    //      and start the session.
    //   2. Async Open queue and verify the operation succeeds only after the
    //      handler picks up the internal event.
    //   3. Verify the Async API callback is called.
    //   4. Repeat for configure.
    //   5. Repeat for close.
    //   6. Stop the session
    //
    // Testing manipulators:
    //   - start
    //   - openQueueAsync
    //   - configureQueueAsync
    //   - closeQueueAsync
    //   - stop
    // ------------------------------------------------------------------------
    const bsls::TimeInterval       timeout = bsls::TimeInterval(1);
    bmqt::QueueOptions             queueOptions;
    bsl::shared_ptr<bmqimp::Event> eventSp;
    bsl::shared_ptr<bmqimp::Event> dummy;
    bmqt::SessionOptions           sessionOptions;
    bmqp_ctrlmsg::ControlMessage   currentRequest(
        bmqtst::TestHelperUtil::allocator());
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestSession           obj(sessionOptions, scheduler, withHandler);
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    PVV_SAFE("1: Start the session");
    obj.startAndConnect();

    PVV_SAFE("2: Open the queue");

    bsl::shared_ptr<bmqimp::Queue> queueSp = obj.createQueue(
        k_URI,
        bmqt::QueueFlags::e_READ | bmqt::QueueFlags::e_WRITE,
        queueOptions);
    int                                calls = 0;
    const bmqimp::Event::EventCallback cb    = bdlf::BindUtil::bind(
        &eventCallbackMockup,
        &eventSp,
        &calls,
        &obj.session(),
        bdlf::PlaceHolders::_1);  // event

    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());

    obj.session().openQueueAsync(queueSp, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 0);
    BMQTST_ASSERT(obj.checkNoEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);

    BMQTST_ASSERT_EQ(calls, 0);
    BMQTST_ASSERT(obj.checkNoEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);

    dummy = obj.getInboundEvent();
    BMQTST_ASSERT_EQ(dummy->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(dummy->sessionEventType(),
                     bmqt::SessionEventType::e_UNDEFINED);
    BMQTST_ASSERT_EQ(calls, 1);

    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_OPEN_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("3: Configure");
    obj.session().configureQueueAsync(queueSp, queueOptions, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 1);
    BMQTST_ASSERT(obj.checkNoEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);

    dummy = obj.getInboundEvent();
    BMQTST_ASSERT_EQ(dummy->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(dummy->sessionEventType(),
                     bmqt::SessionEventType::e_UNDEFINED);
    BMQTST_ASSERT_EQ(calls, 2);

    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("4: Close");
    obj.session().closeQueueAsync(queueSp, timeout, cb);

    BMQTST_ASSERT_EQ(calls, 2);
    BMQTST_ASSERT(obj.checkNoEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);

    BMQTST_ASSERT_EQ(calls, 2);
    BMQTST_ASSERT(obj.checkNoEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CLOSE_QUEUE);
    obj.sendResponse(request);

    dummy = obj.getInboundEvent();
    BMQTST_ASSERT_EQ(dummy->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(dummy->sessionEventType(),
                     bmqt::SessionEventType::e_UNDEFINED);
    BMQTST_ASSERT_EQ(calls, 3);

    BMQTST_ASSERT_EQ(eventSp->statusCode(),
                     bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    BMQTST_ASSERT_EQ(eventSp->type(), bmqimp::Event::EventType::e_SESSION);
    BMQTST_ASSERT_EQ(eventSp->sessionEventType(),
                     bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT);
    // There is no user event.
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("5: Stop");

    obj.stopGracefully();

    BMQTST_ASSERT_EQ(queueSp->isValid(), false);
    BMQTST_ASSERT_EQ(calls, 3);
    eventSp.clear();
    dummy.clear();
}

static void test38_syncOpenConfigureCloseWithHandler()
// ------------------------------------------------------------------------
// SYNC API
//
// Concerns:
//   1. Test Sync
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object with Event Handler
//      and start the session.
//   2. Sync Open queue and verify the operation succeeds only after the
//      handler picks up the internal event.
//   3. Repeat for configure.
//   4. Repeat for close.
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueueSync
//   - configureQueueSync
//   - closeQueueSync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SYNC API WITH HANDLER TEST");

    test_syncOpenConfigureClose(true);
}

static void test39_syncOpenConfigureCloseWithoutHandler()
// ------------------------------------------------------------------------
// SYNC API
//
// Concerns:
//   1. Test Sync
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object without Event
//      Handler and start the session.
//   2. Sync Open queue and verify the operation succeeds as soon as BS
//      gets responses.
//   3. Repeat for configure.
//   4. Repeat for close.
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueueSync
//   - configureQueueSync
//   - closeQueueSync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SYNC API WITHOUT HANDLER TEST");

    test_syncOpenConfigureClose(false);
}

static void test40_syncCalledFromEventHandler()
// ------------------------------------------------------------------------
// SYNC API CALLED FROM EVENT HANDLER TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it calls sync
//      queue API from the EventHandler thread
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object and start the
//      session.
//   2. Call async open queue to generate async event.
//   3. In the main thread wait the eventHandler to receive the
//      OPEN_QUEUE_RESULT event while in the eventHandler call sync queue
//      API
//   4. Verify the sync calls return.
//   5. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueue
//   - configureQueue
//   - closeQueue
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SYNC API CALLED FROM EVENT HANDLER TEST");

    bmqt::Uri uri(k_URI, bmqtst::TestHelperUtil::allocator());
    const bsls::TimeInterval&      timeout = bsls::TimeInterval();
    bsl::shared_ptr<bmqimp::Queue> pQueue;
    bdlbb::PooledBlobBufferFactory blobBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &blobBufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqt::SessionOptions           sessionOptions;
    bmqt::QueueOptions             queueOptions;
    bsl::shared_ptr<bmqimp::Event> eventSp;
    bsls::AtomicInt                startCounter(0);
    bsls::AtomicInt                stopCounter(0);
    bslmt::TimedSemaphore          semaphore;

    pQueue.createInplace(bmqtst::TestHelperUtil::allocator(),
                         bmqtst::TestHelperUtil::allocator());
    pQueue->setUri(uri);
    pQueue->setFlags(bmqt::QueueFlags::e_WRITE);
    pQueue->setOptions(queueOptions);

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    scheduler.start();

    sessionOptions.setNumProcessingThreads(1);

    bmqimp::BrokerSession session(
        &scheduler,
        &blobBufferFactory,
        &blobSpPool,
        sessionOptions,
        bdlf::BindUtil::bind(&eventHandlerSyncCall,
                             &eventSp,
                             &session,
                             pQueue,
                             bsl::ref(semaphore),
                             bdlf::PlaceHolders::_1),
        bdlf::BindUtil::bind(&stateCb,
                             bdlf::PlaceHolders::_1,  // old state
                             bdlf::PlaceHolders::_2,  // new state
                             &startCounter,
                             &stopCounter),
        bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Step 1. Starting session...");
    int rc = session.startAsync();
    BMQTST_ASSERT_EQ(rc, 0);

    // Session changes its state synchronously.
    BMQTST_ASSERT_EQ(session.state(),
                     bmqimp::BrokerSession::State::e_STARTING);

    BMQTST_ASSERT_EQ(startCounter, 1);
    PVV_SAFE("Session starting...: " << rc);

    PVV_SAFE("Step 3. Checking async open...");
    rc = session.openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_REFUSED);

    // Need to clear eventSp before the event pool destructs
    eventSp.clear();

    PVV_SAFE("Stopping session...");
    session.stop();

    // 'stop' is synchronous and there is no IO connection.
    PVV_SAFE("Session stopped...");

    BMQTST_ASSERT_EQ(startCounter, 1);
    BMQTST_ASSERT_EQ(stopCounter, 1);

    scheduler.cancelAllEventsAndWait();
    scheduler.stop();
}

static void test41_asyncOpenConfigureCloseWithHandler()
// ------------------------------------------------------------------------
// ASYNC API
//
// Concerns:
//   1. Test Async callback
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object with Event Handler
//      and start the session.
//   2. Async Open queue and verify the operation succeeds only after the
//      handler picks up the internal event.
//   3. Verify the Async API callback is called.
//   4. Repeat for configure.
//   5. Repeat for close.
//   6. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ASYNC API WITH HANDLER TEST");

    test_asyncOpenConfigureClose(true);
}

static void test42_asyncOpenConfigureCloseWithoutHandler()
// ------------------------------------------------------------------------
// ASYNC API
//
// Concerns:
//   1. Test Async callback
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object with Event Handler
//      and start the session.
//   2. Async Open queue and verify the operation succeeds only after the
//      handler picks up the internal event.
//   3. Verify the Async API callback is called.
//   4. Repeat for configure.
//   5. Repeat for close.
//   6. Stop the session
//
// Testing manipulators:
//   - start
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   - stop
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ASYNC API WITHOUT HANDLER TEST");

    test_asyncOpenConfigureClose(false);
}

static void test43_hostHealthMonitoringErrors()
// ------------------------------------------------------------------------
// HOST HEALTH MONITORING ERRORS TEST
//
// Concerns:
//
//   1. Check that errors related to monitoring of host health, encountered
//      while communicating with the upstream broker, are correctly
//      handled.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open a queue.
//   4. Explicitly provide the host health monitor with an unhealthy value.
//   5. Provide the BrokerSession with an error response "from the broker".
//   6. Verify that the channel is dropped, queue is suspended, etc.
//   7. Ensure that suspended queue is not reconfigured when channel comes
//      back online.
//   8. Restore health, then verify same behavior with a response timeout.
//   9. Provide the host health monitor with a healthy value.
//  10. Provide the BrokerSession with an error response "from the broker".
//  11. Verify that the channel is not dropped, etc.
//  12. Return to unhealthy state, then verify same behavior with a
//      response timeout.
//  13. Close the session.
//
// Testing manipulators:
//   - start
//   - setHostHealthMonitor
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName("HOST HEALTH MONITORING ERRORS TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);
    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    sessionOptions.setNumProcessingThreads(1)
        .setConfigureQueueTimeout(timeout)
        .setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. Set up bmqimp::BrokerSession with "
             "bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Open a queue");
    bmqt::QueueOptions queueOptions;
    queueOptions.setSuspendsOnBadHostHealth(true);

    bsl::shared_ptr<bmqimp::Queue> queue = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=foo",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    obj.openQueue(queue);
    BMQTST_ASSERT(!queue->isSuspended());

    PVV_SAFE("Step 4. Set host to an unhealthy state");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    PVV_SAFE("Step 5. Provide an error response from the broker");
    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendStatus(request);

    PVV_SAFE("Step 6. Verify channel dropped, etc");
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent(bmqt::GenericResult::e_UNKNOWN));
    BMQTST_ASSERT(queue->isSuspended());
    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());
    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_PENDING));

    PVV_SAFE("Step 7. "
             "Ensure queue is not reconfigured when channel comes back");
    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_OPENED));
    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 8. Verify same flow with response timeout");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());

    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // Emulate request timeout.
    obj.advanceTime(timeout);

    BMQTST_ASSERT(obj.waitQueueSuspendedEvent(bmqt::GenericResult::e_TIMEOUT));
    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());
    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_PENDING));

    // Ensure late response is no-op.
    obj.checkNoEvent();
    obj.sendResponse(request);
    obj.checkNoEvent();

    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT(obj.waitForQueueState(queue, bmqimp::QueueState::e_OPENED));
    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 9. Provide monitor with a healthy host health state");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);

    PVV_SAFE("Step 10. Provide an error response from the broker");
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendStatus(request);

    PVV_SAFE("Step 11. Verify that channel is not dropped, etc");
    BMQTST_ASSERT(obj.waitQueueResumedEvent(bmqt::GenericResult::e_UNKNOWN));
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());

    PVV_SAFE("Step 12. Verify same behavior with a timeout response");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    BMQTST_ASSERT(obj.checkNoEvent());

    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // Emulate request timeout.
    obj.advanceTime(timeout);

    BMQTST_ASSERT(obj.waitQueueResumedEvent(bmqt::GenericResult::e_TIMEOUT));
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());
    BMQTST_ASSERT_EQ(queue->state(), bmqimp::QueueState::e_OPENED);

    const int k_INVALID_CONFIGURE_ID = bmqimp::Queue::k_INVALID_CONFIGURE_ID;
    BMQTST_ASSERT_EQ(queue->pendingConfigureId(), k_INVALID_CONFIGURE_ID);

    // Ensure late response is no-op.
    obj.checkNoEvent();
    obj.sendResponse(request);
    obj.checkNoEvent();

    PVV_SAFE("Step 13. Close session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void testHostHealthWithMultipleQueues(
    TestSession*                    testSession,
    ManualHostHealthMonitor*        monitor,
    bsl::shared_ptr<bmqimp::Queue>* sensitiveReaders,
    const size_t                    numSensitiveReaders,
    bsl::shared_ptr<bmqimp::Queue>* naiveReaders,
    const size_t                    numNaiveReaders,
    bsl::shared_ptr<bmqimp::Queue>* sensitiveWriters,
    const size_t                    numSensitiveWriters,
    bsl::shared_ptr<bmqimp::Queue>* naiveWriters,
    const size_t                    numNaiveWriters)
// Implementation invoked multiple times by
// 'test44_hostHealthMonitoringMultipleQueues'.
{
    const size_t numSensitiveQueues = numSensitiveReaders +
                                      numSensitiveWriters;

    PVV_SAFE("Step 1. Set host to an unhealthy state");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);
    BMQTST_ASSERT(testSession->waitHostUnhealthyEvent());

    PVV_SAFE("Step 2. Verify that only two queues are suspended");
    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());
    for (size_t k = 0; k < numSensitiveReaders; k++) {
        request = testSession->getNextOutboundRequest(
            TestSession::e_REQ_CONFIG_QUEUE);
        testSession->sendResponse(request);
    }
    for (size_t k = 0; k < numSensitiveQueues; k++) {
        PVV_SAFE("Waiting on event #" << k);
        BMQTST_ASSERT(testSession->waitQueueSuspendedEvent());
    }
    for (size_t k = 0; k < numSensitiveReaders; k++) {
        BMQTST_ASSERT(sensitiveReaders[k]->isSuspended());
    }
    for (size_t k = 0; k < numSensitiveWriters; k++) {
        BMQTST_ASSERT(sensitiveWriters[k]->isSuspended());
    }
    for (size_t k = 0; k < numNaiveReaders; k++) {
        BMQTST_ASSERT(!naiveReaders[k]->isSuspended());
    }
    for (size_t k = 0; k < numNaiveWriters; k++) {
        BMQTST_ASSERT(!naiveWriters[k]->isSuspended());
    }

    PVV_SAFE("Step 3. "
             "Verify that naive readers can still reconfigure with broker");
    bmqt::QueueOptions naiveOptions;
    naiveOptions.setSuspendsOnBadHostHealth(false);
    for (size_t k = 0; k < numNaiveReaders; k++) {
        BMQTST_ASSERT_EQ(
            bmqt::GenericResult::e_SUCCESS,
            testSession->session().configureQueueAsync(naiveReaders[k],
                                                       naiveOptions,
                                                       bsls::TimeInterval(5)));
        request = testSession->getNextOutboundRequest(
            TestSession::e_REQ_CONFIG_QUEUE);
        testSession->sendResponse(request);
        BMQTST_ASSERT(testSession->verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    }

    PVV_SAFE("Step 4. Recover host health and verify that all queues resume");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);
    for (size_t k = 0; k < numSensitiveReaders; k++) {
        request = testSession->getNextOutboundRequest(
            TestSession::e_REQ_CONFIG_QUEUE);
        testSession->sendResponse(request);
    }
    for (size_t k = 0; k < numSensitiveQueues; k++) {
        BMQTST_ASSERT(testSession->waitQueueResumedEvent());
    }
    BMQTST_ASSERT(testSession->waitHostHealthRestoredEvent());
    BMQTST_ASSERT(testSession->checkNoEvent());

    for (size_t k = 0; k < numSensitiveReaders; k++) {
        BMQTST_ASSERT(!sensitiveReaders[k]->isSuspended());
    }
    for (size_t k = 0; k < numSensitiveWriters; k++) {
        BMQTST_ASSERT(!sensitiveWriters[k]->isSuspended());
    }
    for (size_t k = 0; k < numNaiveReaders; k++) {
        BMQTST_ASSERT(!naiveReaders[k]->isSuspended());
    }
    for (size_t k = 0; k < numNaiveWriters; k++) {
        BMQTST_ASSERT(!naiveWriters[k]->isSuspended());
    }
}

static void test44_hostHealthMonitoringMultipleQueues()
// ------------------------------------------------------------------------
// HOST HEALTH MONITORING MULTIPLE QUEUES TEST
//
// Concerns:
//
//   1. Check that host health monitoring notifications behave correctly in
//      the presence of multiple queues, not all of which are sensitive to
//      the health of the host.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open three queues: Two of which are host-health-sensitive, and one
//      of which is not.
//   4. Explicitly provide the host health monitor with an unhealthy value.
//   5. Verify that only the host-health-sensitive queues are suspended.
//   6. Verify that the non-suspended queue can still reconfigure with the
//      broker.
//   7. Restore the host health, and verify that all queues resume.
//   8. Close the session.
//
// Testing manipulators:
//   - start
//   - setHostHealthMonitor
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName(
        "HOST HEALTH MONITORING MULTIPLE QUEUES TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());
    const bsls::TimeInterval                  timeout = bsls::TimeInterval(15);
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1)
        .setConfigureQueueTimeout(timeout)
        .setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. "
             "Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Open several different kinds of queues");
    bmqt::QueueOptions naiveOptions;
    naiveOptions.setSuspendsOnBadHostHealth(false);

    bmqt::QueueOptions sensitiveOptions;
    sensitiveOptions.setSuspendsOnBadHostHealth(true);

    bsl::shared_ptr<bmqimp::Queue> sensitiveReaders[] = {
        obj.createQueue("bmq://ts.trades.myapp/my.q1?id=foo",
                        bmqt::QueueFlags::e_READ,
                        sensitiveOptions),
        obj.createQueue("bmq://ts.trades.myapp/my.q2?id=foo",
                        bmqt::QueueFlags::e_READ,
                        sensitiveOptions),
    };
    const size_t numSensitiveReaders = sizeof(sensitiveReaders) /
                                       sizeof(sensitiveReaders[0]);

    bsl::shared_ptr<bmqimp::Queue> naiveReaders[] = {
        obj.createQueue("bmq://ts.trades.myapp/my.q5?id=foo",
                        bmqt::QueueFlags::e_READ,
                        naiveOptions),
        obj.createQueue("bmq://ts.trades.myapp/my.q6?id=foo",
                        bmqt::QueueFlags::e_READ,
                        naiveOptions),
    };
    const size_t numNaiveReaders = sizeof(naiveReaders) /
                                   sizeof(naiveReaders[0]);

    bsl::shared_ptr<bmqimp::Queue> sensitiveWriters[] = {
        obj.createQueue("bmq://ts.trades.myapp/my.q3?id=foo",
                        bmqt::QueueFlags::e_WRITE,
                        sensitiveOptions),
        obj.createQueue("bmq://ts.trades.myapp/my.q4?id=foo",
                        bmqt::QueueFlags::e_WRITE,
                        sensitiveOptions),
    };
    const size_t numSensitiveWriters = sizeof(sensitiveWriters) /
                                       sizeof(sensitiveWriters[0]);

    bsl::shared_ptr<bmqimp::Queue> naiveWriters[] = {
        obj.createQueue("bmq://ts.trades.myapp/my.q7?id=foo",
                        bmqt::QueueFlags::e_WRITE,
                        naiveOptions),
        obj.createQueue("bmq://ts.trades.myapp/my.q8?id=foo",
                        bmqt::QueueFlags::e_WRITE,
                        naiveOptions),
    };
    const size_t numNaiveWriters = sizeof(naiveWriters) /
                                   sizeof(naiveWriters[0]);

    PVV_SAFE("Step 4. Test with readers only.");
    for (size_t k = 0; k < numSensitiveReaders; k++) {
        obj.openQueue(sensitiveReaders[k]);
    }
    for (size_t k = 0; k < numNaiveReaders; k++) {
        obj.openQueue(naiveReaders[k]);
    }
    testHostHealthWithMultipleQueues(&obj,
                                     monitor,
                                     sensitiveReaders,
                                     numSensitiveReaders,
                                     naiveReaders,
                                     numNaiveReaders,
                                     NULL,
                                     0,
                                     NULL,
                                     0);

    PVV_SAFE("Step 5. Test with mixed readers and writers.");
    for (size_t k = 0; k < numSensitiveWriters; k++) {
        obj.openQueue(sensitiveWriters[k]);
    }
    for (size_t k = 0; k < numNaiveWriters; k++) {
        obj.openQueue(naiveWriters[k]);
    }
    testHostHealthWithMultipleQueues(&obj,
                                     monitor,
                                     sensitiveReaders,
                                     numSensitiveReaders,
                                     naiveReaders,
                                     numNaiveReaders,
                                     sensitiveWriters,
                                     numSensitiveWriters,
                                     naiveWriters,
                                     numNaiveWriters);

    PVV_SAFE("Step 6. Test with writers only.");
    for (size_t k = 0; k < numSensitiveReaders; k++) {
        obj.closeQueue(sensitiveReaders[k], bsls::TimeInterval(5), true);
    }
    for (size_t k = 0; k < numNaiveReaders; k++) {
        obj.closeQueue(naiveReaders[k], bsls::TimeInterval(5), true);
    }
    testHostHealthWithMultipleQueues(&obj,
                                     monitor,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     sensitiveWriters,
                                     numSensitiveWriters,
                                     naiveWriters,
                                     numNaiveWriters);

    PVV_SAFE("Step 7. Close session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void test45_hostHealthMonitoringPendingStandalone()
// ------------------------------------------------------------------------
// HOST HEALTH MONITORING PENDING STANDALONE CONFIGURE TEST
//
// Concerns:
// 1. Check that reader queue doesn't suspend and resume if there is
//    pending standalone configure request.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open queue (reader).
//   4. Send standalone configure request.
//   5. Set the host health to unhealthy.
//   6. Verify HOST_UNHEALTHY session event is issued.
//   7. Verify the queue defers to suspend.
//   8. Set the host health back to healthy.
//   9. Verify HOST_HEALTH_RESTORED event is issued.
//  10. Verify the queue defers to resume.
//  11. Send standalone configure response.
//  12. Verify the queue has not sent suspend/resume requests.
//  13. Check there should be no more events.
//  14. Close the session.
//
// Testing manipulators:
//   - start
//   - setHostHealthMonitor
//   - openQueue
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName(
        "HOST HEALTH MONITORING PENDING STANDALONE TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1).setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. "
             "Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Open queue (reader)");
    bmqt::QueueOptions queueOptions;
    queueOptions.setSuspendsOnBadHostHealth(true);

    bsls::TimeInterval timeout(5);

    bsl::shared_ptr<bmqimp::Queue> queue = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    BMQTST_ASSERT(!queue->isSuspended());
    obj.openQueue(queue, timeout, false);
    BMQTST_ASSERT(!queue->isSuspended());

    PVV_SAFE("Step 4. Send standalone configure request.");
    int rc = obj.session().configureQueueAsync(queue,
                                               queue->options(),
                                               timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    bmqp_ctrlmsg::ControlMessage configureRequest(
        bmqtst::TestHelperUtil::allocator());
    configureRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    PVV_SAFE("Step 5. Set the host health to unhealthy");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);

    PVV_SAFE("Step 6. Verify HOST_UNHEALTHY session event is issued.");
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    PVV_SAFE("Step 7. Verify the queue defers to suspend.");
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT(!queue->isSuspended());

    PVV_SAFE("Step 8. Set the host health back to healthy.");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);

    PVV_SAFE("Step 9. Verify e_HOST_HEALTH_RESTORED event is issued.");
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());

    PVV_SAFE("Step 10. Verify the queue defers to resume.");
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT(!queue->isSuspended());

    PVV_SAFE("Step 11. Send standalone configure response.");
    obj.sendResponse(configureRequest);

    PVVV_SAFE("the queue should receive standalone configure response.");
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    PVV_SAFE("Step 12. "
             "Verify the queue has not sent suspend/resume requests.");
    BMQTST_ASSERT(obj.isChannelEmpty());
    BMQTST_ASSERT(!queue->isSuspended());

    PVV_SAFE("Step 13. Check there should be no more events.");
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 14. Stop the session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void test46_hostHealthMonitoringDeferredResumeClose()
// ------------------------------------------------------------------------
// HOST HEALTH MONITORING DEFERRED RESUME CLOSE QUEUE TEST
//
// Concerns:
// 1. Check that e_HOST_HEALTH_RESTORED event is issued when queue is being
//    closed during deferred resuming.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open queues (reader).
//   4. Set the host health to unhealthy.
//   5. Verify HOST_UNHEALTHY session event is issued.
//   6. Set the host health back to healthy.
//   7. Verify the queue defers to resume.
//   8. Close the queues.
//   9. Verify HOST_HEALTH_RESTORED event is issued.
//  10. Check there should be no more events.
//  11. Close the session.
//
// Testing manipulators:
//   - start
//   - setHostHealthMonitor
//   - openQueue
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName(
        "HOST HEALTH MONITORING DEFERRED SUSPEND RESUME TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1).setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. "
             "Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Open queue (reader)");
    bmqt::QueueOptions queueOptions;
    queueOptions.setSuspendsOnBadHostHealth(true);

    bsls::TimeInterval timeout(5);

    bsl::shared_ptr<bmqimp::Queue> queue1 = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    bsl::shared_ptr<bmqimp::Queue> queue2 = obj.createQueue(
        "bmq://ts.trades.myapp/other.queue",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    BMQTST_ASSERT(!queue1->isSuspended());
    BMQTST_ASSERT(!queue2->isSuspended());
    obj.openQueue(queue1, timeout, false);
    obj.openQueue(queue2, timeout, false);
    BMQTST_ASSERT(!queue1->isSuspended());
    BMQTST_ASSERT(!queue2->isSuspended());

    PVV_SAFE("Step 4. Set the host health to unhealthy");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);

    PVV_SAFE("Step 5. Verify HOST_UNHEALTHY session event is issued.");
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    PVVV_SAFE("The queue should send suspend request.");
    bmqp_ctrlmsg::ControlMessage suspendRequest(
        bmqtst::TestHelperUtil::allocator());
    suspendRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);
    suspendRequest = obj.getNextOutboundRequest(
        TestSession::e_REQ_CONFIG_QUEUE);

    PVVV_SAFE("There should be no e_QUEUE_SUSPENDED event.");
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT(!queue1->isSuspended());
    BMQTST_ASSERT(!queue2->isSuspended());

    PVV_SAFE("Step 6. Set the host health back to healthy.");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);

    // Drain the FSM queue to verify the health state event is handled.
    BMQTST_ASSERT_EQ(obj.session().start(bsls::TimeInterval(1)), 0);

    PVV_SAFE("Step 7. Verify the queue defers to resume.");
    BMQTST_ASSERT(obj.checkNoEvent());
    BMQTST_ASSERT(obj.isChannelEmpty());

    // Use _SAFE version to avoid std::cout race with logging from 'stateCb()'.
    PVV_SAFE("Step 8. Close the queue.");
    {
        int rc = obj.session().closeQueueAsync(queue1, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
        rc = obj.session().closeQueueAsync(queue2, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

        PVVV_SAFE("Closing: Check configure request was sent");
        bmqp_ctrlmsg::ControlMessage request1(
            bmqtst::TestHelperUtil::allocator());
        bmqp_ctrlmsg::ControlMessage request2(
            bmqtst::TestHelperUtil::allocator());
        request1 = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
        request2 = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

        BMQTST_ASSERT_EQ(queue1->state(), bmqimp::QueueState::e_CLOSING_CFG);
        BMQTST_ASSERT_EQ(queue2->state(), bmqimp::QueueState::e_CLOSING_CFG);
        BMQTST_ASSERT_EQ(queue1->isValid(), false);
        BMQTST_ASSERT_EQ(queue2->isValid(), false);

        BMQTST_ASSERT(!queue1->isSuspended());
        BMQTST_ASSERT(!queue2->isSuspended());
        BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());
        BMQTST_ASSERT(obj.checkNoEvent());

        PVVV_SAFE("Closing: Send valid configure queue response message");
        obj.sendResponse(request1);
        obj.sendResponse(request2);

        PVVV_SAFE("Closing: Check close request was sent");
        request1 = obj.verifyCloseRequestSent(true);
        request2 = obj.verifyCloseRequestSent(true);

        PVVV_SAFE("Closing: Send valid close queue response message");
        obj.sendResponse(request1);
        obj.sendResponse(request2);

        PVVV_SAFE("Closing: Wait close queue result");
        obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                                   queue1);
        obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                                   queue2);
    }

    PVV_SAFE("Step 10. Check there should be no more events.");
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 11. Stop the session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void test47_configureMergesQueueOptions()
// ------------------------------------------------------------------------
// CONFIGURE MERGES QUEUE OPTIONS TEST
//
// Concerns:
// 1. Check that 'configureQueue' only updates fields from
//    'bmqt::QueueOptions' that were explicitly set by the client (rather
//    than overriding previously set fields with default values).
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open a reader queue.
//   4. Configure the queue with only 'maxUnconfirmedBytes' modified.
//   5. Verify that the request sent to the broker, and the queue options,
//      both finish with the merged options.
//   6. Close the session.
//
// Testing manipulators:
//   - configureQueueAsync
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName("CONFIGURE MERGED QUEUE OPTIONS TEST");

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1);

    PVV_SAFE("Step 1. Set up bmqimp::BrokerSession");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 3. Open queue (reader)");
    bmqt::QueueOptions queueOptions;
    queueOptions.setMaxUnconfirmedMessages(300);

    bsls::TimeInterval timeout(5);

    bsl::shared_ptr<bmqimp::Queue> queue = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    obj.openQueue(queue, timeout, false);
    BMQTST_ASSERT(queue->options().hasMaxUnconfirmedMessages());
    BMQTST_ASSERT(!queue->options().hasMaxUnconfirmedBytes());
    BMQTST_ASSERT(!queue->options().hasConsumerPriority());
    BMQTST_ASSERT(!queue->options().hasSuspendsOnBadHostHealth());

    PVV_SAFE("Step 4. "
             "Configure queue with only 'maxUnconfirmedBytes' modified");
    bmqt::QueueOptions updates;
    updates.setMaxUnconfirmedBytes(500);

    int rc = obj.session().configureQueueAsync(queue, updates, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 5. "
             "Verify that request and queue finish with merged options");
    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    bmqp_ctrlmsg::ConsumerInfo ci;
    getConsumerInfo(&ci, request);

    BMQTST_ASSERT_EQ(ci.maxUnconfirmedBytes(), 500);
    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 300);
    BMQTST_ASSERT_EQ(ci.consumerPriority(),
                     bmqt::QueueOptions::k_DEFAULT_CONSUMER_PRIORITY);

    obj.sendResponse(request);
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(queue->options().maxUnconfirmedBytes(), 500);
    BMQTST_ASSERT_EQ(queue->options().maxUnconfirmedMessages(), 300);
    BMQTST_ASSERT(!queue->options().hasConsumerPriority());
    BMQTST_ASSERT(!queue->options().hasSuspendsOnBadHostHealth());

    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 6. Stop the session");
    obj.stopGracefully();
}

static void test48_hostHealthSensitivityReconfiguration()
// ------------------------------------------------------------------------
// HOST HEALTH SENSITIVITY RECONFIGURATION TEST
//
// Concerns:
// 1. Check that queues are correctly suspended and resumed in response to
//    client-driven configure operations that modify the
//    'suspendsOnBadHostHealth' queue option.
//
// Plan:
//   1. Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor.
//   2. Start the session.
//   3. Open two host health sensitive queues (one reader, one writer).
//   4. Set the host health to unhealthy.
//   5. Verify that queues suspend.
//   6. Configure each queue as no longer host health sensitive.
//   7. Verify that each queue resumes, with no HOST_HEALTH_RESTORED event.
//   8. Set the host health back to healthy.
//   9. Verify HOST_HEALTH_RESTORED event is issued.
//  10. Set the host back to unhealthy.
//  11. Verify HOST_UNHEALTHY with no QUEUE_SUSPENDED events.
//  12. Configure each queue back to host health sensitive.
//  13. Verify that queues suspend.
//  14. Stop the session.
//
// Testing manipulators:
//   - configureQueue
// ------------------------------------------------------------------------

{
    bmqtst::TestHelper::printTestName(
        "HOST HEALTH SENSITIVITY RECONFIGURATION TEST");

    ManualHostHealthMonitor* monitor = new (
        *bmqtst::TestHelperUtil::allocator())
        ManualHostHealthMonitor(bmqt::HostHealthState::e_HEALTHY,
                                bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::HostHealthMonitor> monitor_sp(
        monitor,
        bmqtst::TestHelperUtil::allocator());

    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);
    bmqt::SessionOptions  sessionOptions;
    sessionOptions.setNumProcessingThreads(1).setHostHealthMonitor(monitor_sp);

    PVV_SAFE("Step 1. "
             "Set up bmqimp::BrokerSession with bmqpi::HostHealthMonitor");
    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);

    PVV_SAFE("Step 2. Start the session");
    obj.startAndConnect();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 1u);

    PVV_SAFE("Step 3. Open queue (reader)");
    bmqt::QueueOptions queueOptions;
    queueOptions.setSuspendsOnBadHostHealth(true);

    bsls::TimeInterval timeout(5);

    bsl::shared_ptr<bmqimp::Queue> reader = obj.createQueue(
        "bmq://ts.trades.myapp/reader.queue",
        bmqt::QueueFlags::e_READ,
        queueOptions);
    obj.openQueue(reader, timeout, false);
    BMQTST_ASSERT(!reader->isSuspended());

    bsl::shared_ptr<bmqimp::Queue> writer = obj.createQueue(
        "bmq://ts.trades.myapp/writer.queue",
        bmqt::QueueFlags::e_WRITE,
        queueOptions);
    obj.openQueue(writer, timeout, false);
    BMQTST_ASSERT(!writer->isSuspended());

    PVV_SAFE("Step 4. Set the host health to unhealthy");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);

    PVV_SAFE("Step 5. Verify that queues suspend");
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());

    // Writer should suspend immediately without configure request.
    obj.waitQueueSuspendedEvent();
    // Reader needs to wait for the configure-roundtrip.
    bmqp_ctrlmsg::ControlMessage request(bmqtst::TestHelperUtil::allocator());
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    bmqp_ctrlmsg::ConsumerInfo ci;
    getConsumerInfo(&ci, request);

    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 0);
    BMQTST_ASSERT_EQ(ci.consumerPriority(),
                     bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());

    BMQTST_ASSERT(reader->isSuspended());
    BMQTST_ASSERT(writer->isSuspended());
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE(
        "Step 6. Configure each queue as no longer host health sensitive");
    queueOptions.setSuspendsOnBadHostHealth(false);
    queueOptions.setMaxUnconfirmedMessages(456);

    int rc = obj.session().configureQueueAsync(reader, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    rc = obj.session().configureQueueAsync(writer, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    getConsumerInfo(&ci, request);

    BMQTST_ASSERT_EQ(ci.consumerPriority(), 0);
    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 456);

    PVV_SAFE(
        "Step 7. Verify each queue resumes, with no HOST_HEALTH_RESTORED");
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(writer->options(), queueOptions);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueResumedEvent());
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(reader->options(), queueOptions);
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 8. Set the host health back to healthy");
    monitor->setState(bmqt::HostHealthState::e_HEALTHY);

    PVV_SAFE("Step 9. Verify HOST_HEALTH_RESTORED event is issued");
    BMQTST_ASSERT(obj.waitHostHealthRestoredEvent());

    PVV_SAFE("Step 10. Set the host back to unhealthy");
    monitor->setState(bmqt::HostHealthState::e_UNHEALTHY);

    PVV_SAFE("Step 11. Verify HOST_UNHEALTHY with no QUEUE_SUSPENDED events");
    BMQTST_ASSERT(obj.waitHostUnhealthyEvent());
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 12. Configure each queue back to host health sensitive");
    queueOptions.setSuspendsOnBadHostHealth(true);
    queueOptions.setMaxUnconfirmedMessages(789);

    rc = obj.session().configureQueueAsync(writer, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    rc = obj.session().configureQueueAsync(reader, queueOptions, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    getConsumerInfo(&ci, request);

    BMQTST_ASSERT_EQ(ci.consumerPriority(),
                     bmqp::Protocol::k_CONSUMER_PRIORITY_INVALID);
    BMQTST_ASSERT_EQ(ci.maxUnconfirmedMessages(), 0);

    PVV_SAFE("Step 13. Verify that queues suspend");
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(writer->options(), queueOptions);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.waitQueueSuspendedEvent());
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT_EQ(reader->options(), queueOptions);
    BMQTST_ASSERT(obj.checkNoEvent());

    PVV_SAFE("Step 14. Stop the session");
    obj.stopGracefully();
    BMQTST_ASSERT_EQ(monitor->numRegistrants(), 0u);
}

static void test49_controlsBuffering()
// ------------------------------------------------------------------------
// CONTROLS BUFFERING TEST
//
// Concerns:
//   1. Check the behavior of the bmqimp::BrokerSession when it has a
//      mocked connection with the broker and control messages are sent
//      when the channel is not writable.
//   2. Set the channel to return e_LIMIT status.
//   3. Open several queues async and check outbound requests.
//   4. Schedule LWM event but keep the e_LIMIT status and check control
//      blob resending attempt.
//   5. Schedule LWM event and set the e_SUCCESS status.  Verify all the
//      pending control blobs are sent.
//   6. Stop the session.
//   7. Verify pending Open requests are canceled.
//
// Testing manipulators:
//   - openQueueAsync
//   - handleChannelWatermark
//-------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONTROLS BUFFERING TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());

    sessionOptions.setNumProcessingThreads(1);

    // Create test session with the system time source
    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> queueFoo = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=foo",
        bmqt::QueueFlags::e_READ,
        queueOptions);

    bsl::shared_ptr<bmqimp::Queue> queueBar = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=bar",
        bmqt::QueueFlags::e_READ,
        queueOptions);

    bsl::shared_ptr<bmqimp::Queue> queueBaz = obj.createQueue(
        "bmq://ts.trades.myapp/my.queue?id=baz",
        bmqt::QueueFlags::e_READ,
        queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Set the channel to return e_LIMIT on write");
    obj.channel().setWriteStatus(bmqio::StatusCategory::e_LIMIT);

    PVV_SAFE("Step 3. Open the queues async");
    int rc = obj.session().openQueueAsync(queueFoo, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    rc = obj.session().openQueueAsync(queueBar, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    rc = obj.session().openQueueAsync(queueBaz, timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::OpenQueueResult::e_SUCCESS);

    BMQTST_ASSERT(
        obj.waitForQueueState(queueFoo, bmqimp::QueueState::e_OPENING_OPN));
    BMQTST_ASSERT(
        obj.waitForQueueState(queueBar, bmqimp::QueueState::e_OPENING_OPN));
    BMQTST_ASSERT(
        obj.waitForQueueState(queueBaz, bmqimp::QueueState::e_OPENING_OPN));

    // Verify there is only 1 outstanding request
    obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 0u);

    PVV_SAFE("Step 4. Schedule LWM and check control requests");

    // LWM arrives but the channel still returns e_LIMIT
    scheduler.scheduleEvent(
        bmqsys::Time::nowMonotonicClock(),
        bdlf::BindUtil::bind(&TestSession::setChannelLowWaterMark,
                             &obj,
                             bmqio::StatusCategory::e_LIMIT));

    // Only one write attempt happened
    obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 0u);

    PVV_SAFE("Step 5. Schedule LWM and check all requests proceeded");

    // Set write status to e_SUCCESS
    scheduler.scheduleEvent(
        bmqsys::Time::nowMonotonicClock(),
        bdlf::BindUtil::bind(&TestSession::setChannelLowWaterMark,
                             &obj,
                             bmqio::StatusCategory::e_SUCCESS));

    // All buffered control messages should be written
    obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);
    obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);
    obj.verifyRequestSent(TestSession::e_REQ_OPEN_QUEUE);
    BMQTST_ASSERT_EQ(obj.channel().writeCalls().size(), 0u);

    PV_SAFE("Step 6. Stop the session");
    obj.stopGracefully(false);  // don't wait for DISCONNECTED event

    PV_SAFE("Step 7. Check that pending Open queue requests are canceled");
    const bmqp_ctrlmsg::StatusCategory::Value res =
        bmqp_ctrlmsg::StatusCategory::E_NOT_CONNECTED;
    obj.verifyOpenQueueErrorResult(res,
                                   queueFoo,
                                   bmqimp::QueueState::e_CLOSED);
    obj.verifyOpenQueueErrorResult(res,
                                   queueBar,
                                   bmqimp::QueueState::e_CLOSED);
    obj.verifyOpenQueueErrorResult(res,
                                   queueBaz,
                                   bmqimp::QueueState::e_CLOSED);

    BMQTST_ASSERT(obj.waitForChannelClose());
    BMQTST_ASSERT(obj.waitDisconnectedEvent());
}

static void test50_putRetransmittingTest()
// ------------------------------------------------------------------------
// PUT RETRANSMITTING TEST
//
// Concerns:
//   1. Check the unACKed PUT messages generate no NACK on channel down
//      and are retransmitted when the channel is up.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open a queue for writing.
//   3. Post a PUT event with two messages.
//   4. Emulate the first message is ACKed by the broker.
//   5. Trigger channel down and verify no NACK message arrives.
//   6. Post one more PUT message while the channel is down.
//   7. Restore the channel and reopen the queue.
//   8. Verify two PUT messages are retransmitted.
//   9. Trigger channel down again and emulate retransmission timeout.
//   10. Verify two NACK related to the expired messages arrive.
//   11. Post one more PUT message while the channel is down.
//   12. Restore the channel, reopen the queue.
//   13. Trigger channel down again, close the queue, and verify that a
//       single NACK arrives.
//   14. Verify close queue result.
//   15. Stop the session.
//
// Testing manipulators:
//   - start
//   - openQueue
//   - post
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE RETRANSMITTING TEST");

    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    const int k_ACK_STATUS_SUCCESS = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_SUCCESS);
    const int k_ACK_STATUS_UNKNOWN = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_UNKNOWN);

    const bsls::TimeInterval       timeout = bsls::TimeInterval(5);
    int                            phFlags = 0;
    bmqt::SessionOptions           sessionOptions;
    bmqt::QueueOptions             queueOptions;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder     putEventBuilder(&blobSpPool,
                                          bmqtst::TestHelperUtil::allocator());
    bmqp::PutMessageIterator  putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    bmqp::AckEventBuilder     ackEventBuilder(&blobSpPool,
                                          bmqtst::TestHelperUtil::allocator());
    bmqp::Event               rawEvent(bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId corrIdFirst(243);
    const bmqt::CorrelationId corrIdSecond(987);
    const bmqt::CorrelationId corrIdThird(591);
    const bmqt::CorrelationId corrIdFourth(193);
    bdlmt::EventScheduler     scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                 testClock(scheduler);

    bmqt::MessageGUID guidFirst  = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID guidSecond = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID guidThird  = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID guidFourth = bmqp::MessageGUIDGenerator::testGUID();

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Send PUT messages");
    // Set flags
    bmqp::PutHeaderFlagUtil::setFlag(&phFlags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    // Create Event object and get CorrelationId container
    bsl::shared_ptr<bmqimp::Event> putEvent = obj.session().createEvent();

    bmqimp::MessageCorrelationIdContainer* idsContainer =
        putEvent->messageCorrelationIdContainer();
    const bmqp::QueueId qid(pQueue->id(), pQueue->subQueueId());

    // Add CorrelationIds
    idsContainer->add(guidFirst, corrIdFirst, qid);
    idsContainer->add(guidSecond, corrIdSecond, qid);
    idsContainer->add(guidThird, corrIdThird, qid);
    idsContainer->add(guidFourth, corrIdFourth, qid);

    // Create the event blob with two messages
    putEventBuilder.startMessage();
    putEventBuilder.setMessageGUID(guidFirst)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);

    bmqt::EventBuilderResult::Enum rc = putEventBuilder.packMessage(
        pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    putEventBuilder.startMessage();
    putEventBuilder.setMessageGUID(guidSecond)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);
    rc = putEventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using event blob
    int res = obj.session().post(*putEventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    PVV_SAFE("Step 4. Verify PUT event is sent");
    obj.getOutboundEvent(&rawEvent);

    BMQTST_ASSERT(rawEvent.isPutEvent());

    // Send ACK for one message
    ackEventBuilder.appendMessage(k_ACK_STATUS_SUCCESS,
                                  bmqp::AckMessage::k_NULL_CORRELATION_ID,
                                  guidFirst,
                                  pQueue->id());

    obj.session().processPacket(*ackEventBuilder.blob());

    bsl::shared_ptr<bmqimp::Event> ackEvent = obj.waitAckEvent();

    BMQTST_ASSERT(ackEvent);

    bmqp::AckMessageIterator* ackIter = ackEvent->ackMessageIterator();
    BMQTST_ASSERT_EQ(1, ackIter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), ackIter->message().queueId());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_SUCCESS, ackIter->message().status());
    BMQTST_ASSERT_EQ(guidFirst, ackIter->message().messageGUID());
    BMQTST_ASSERT_EQ(1, ackEvent->numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrIdFirst, ackEvent->correlationId(0));
    BMQTST_ASSERT_EQ(0, ackIter->next());

    PVV_SAFE("Step 5. Trigger channel drop and verify no NACK event is sent");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    PVV_SAFE("Step 6. Send one more PUT message while the channel is down");
    putEventBuilder.reset();
    putEventBuilder.startMessage();
    putEventBuilder.setMessageGUID(guidThird)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);
    rc = putEventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using event blob
    res = obj.session().post(*putEventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    // Restore the connection
    obj.setChannel();

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    PVV_SAFE("Step 7. Reopen the queue");
    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 8. Verify PUT event is sent with two messages");
    rawEvent.clear();

    obj.getOutboundEvent(&rawEvent);

    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT(putIter.isValid());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(pQueue->id(), putIter.header().queueId());
    BMQTST_ASSERT_EQ(guidSecond, putIter.header().messageGUID());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(pQueue->id(), putIter.header().queueId());
    BMQTST_ASSERT_EQ(guidThird, putIter.header().messageGUID());
    BMQTST_ASSERT_EQ(0, putIter.next());

    PVV_SAFE("Step 9. Trigger channel drop and advance the time");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    // Advance the time to emulate retransmission timeout
    bsls::TimeInterval retransmissionTimeout(0);
    retransmissionTimeout.addMilliseconds(
        bmqp_ctrlmsg::OpenQueueResponse::
            DEFAULT_INITIALIZER_DEDUPLICATION_TIME_MS);
    obj.advanceTime(retransmissionTimeout);

    PVV_SAFE("Step 10. Verify NACKs for expired messages");
    bsl::shared_ptr<bmqimp::Event> nackEvent = obj.waitAckEvent();

    BMQTST_ASSERT(nackEvent);

    bmqp::AckMessageIterator* iter = nackEvent->ackMessageIterator();
    BMQTST_ASSERT_EQ(1, iter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), iter->message().queueId());
    BMQTST_ASSERT_EQ(guidSecond, iter->message().messageGUID());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_UNKNOWN, iter->message().status());
    BMQTST_ASSERT_EQ(2, nackEvent->numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrIdSecond, nackEvent->correlationId(0));

    BMQTST_ASSERT_EQ(1, iter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), iter->message().queueId());
    BMQTST_ASSERT_EQ(guidThird, iter->message().messageGUID());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_UNKNOWN, iter->message().status());
    BMQTST_ASSERT_EQ(corrIdThird, nackEvent->correlationId(1));
    BMQTST_ASSERT_EQ(0, iter->next());

    PVV_SAFE("Step 11. Send one more PUT message while the channel is down");
    putEventBuilder.reset();
    putEventBuilder.startMessage();
    putEventBuilder.setMessageGUID(guidFourth)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);
    rc = putEventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using event blob
    res = obj.session().post(*putEventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    // Restore the connection
    obj.setChannel();

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    PVV_SAFE("Step 12. Reopen the queue");
    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 13. "
             "Drop the channel, close the queue and verify NACK event");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    obj.session().closeQueueAsync(pQueue, timeout);

    nackEvent = obj.waitAckEvent();

    BMQTST_ASSERT(nackEvent);

    iter = nackEvent->ackMessageIterator();
    BMQTST_ASSERT_EQ(1, iter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), iter->message().queueId());
    BMQTST_ASSERT_EQ(guidFourth, iter->message().messageGUID());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_UNKNOWN, iter->message().status());
    BMQTST_ASSERT_EQ(1, nackEvent->numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrIdFourth, nackEvent->correlationId(0));
    BMQTST_ASSERT_EQ(0, iter->next());

    PVV_SAFE("Step 14. Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                               pQueue);

    PVV_SAFE("Step 15. Stop the session when not connected");
    BMQTST_ASSERT(obj.stop());

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
    rawEvent.clear();
}

static void test51_putRetransmittingNoAckTest()
// ------------------------------------------------------------------------
// PUT RETRANSMITTING NO ACK TEST
//
// Concerns:
//   1. Check the PUT messages without CorrelationId generate no NACK on
//      channel down, and are not retransmitted when reconnected.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Open a queue for writing.
//   3. Post a PUT event with two messages, one with and another without
//      CorrelationId.
//   4. Verify the PUT event is sent into the test channel.
//   5. Trigger channel down and verify no NACK message arrives.
//   6. Post one more PUT message without CorrelationId while the channel
//      is down.
//   7. Restore the channel and reopen the queue.
//   8. Verify PUT event with two messages is retransmitted.
//   9. Trigger channel down and up.
//   10. Reopen the queue again.
//   11. Verify only one message is retransmitted.
//   12. Trigger channel down again, close the queue, and verify that
//       NACK for only one message that has the CorrelationId arrives.
//   13. Verify queue close result arrives.
//   14. Stop the session.
//
// Testing manipulators:
//   - start
//   - openQueue
//   - post
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PUT RETRANSMITTING NO ACK TEST");

    const char* k_PAYLOAD     = "abcdefghijklmnopqrstuvwxyz";
    const int   k_PAYLOAD_LEN = bsl::strlen(k_PAYLOAD);

    const int k_ACK_STATUS_UNKNOWN = bmqp::ProtocolUtil::ackResultToCode(
        bmqt::AckResult::e_UNKNOWN);

    const bsls::TimeInterval       timeout = bsls::TimeInterval(5);
    int                            phFlags = 0;
    bmqt::SessionOptions           sessionOptions;
    bmqt::QueueOptions             queueOptions;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPool blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder     putEventBuilder(&blobSpPool,
                                          bmqtst::TestHelperUtil::allocator());
    bmqp::PutMessageIterator  putIter(&bufferFactory,
                                     bmqtst::TestHelperUtil::allocator());
    bmqp::Event               rawEvent(bmqtst::TestHelperUtil::allocator());
    const bmqt::CorrelationId corrId(243);
    bdlmt::EventScheduler     scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    bmqt::MessageGUID         guid1 = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID         guid2 = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID         guid3 = bmqp::MessageGUIDGenerator::testGUID();

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    scheduler,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Open the queue");
    obj.openQueue(pQueue, timeout);

    PVV_SAFE("Step 3. Send PUT messages");
    // Set flags
    bmqp::PutHeaderFlagUtil::setFlag(&phFlags,
                                     bmqp::PutHeaderFlags::e_ACK_REQUESTED);

    // Create Event object and get CorrelationId container
    bsl::shared_ptr<bmqimp::Event> putEvent = obj.session().createEvent();

    bmqimp::MessageCorrelationIdContainer* idsContainer =
        putEvent->messageCorrelationIdContainer();
    const bmqp::QueueId qid(pQueue->id(), pQueue->subQueueId());

    // Associate CorrelationId with guid2
    idsContainer->add(guid2, corrId, qid);

    // Create the event blob with two messages, the first one without
    // associated CorrelationId and ACK_REQUESTED flag
    putEventBuilder.startMessage();
    putEventBuilder.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setMessageGUID(guid1);

    bmqt::EventBuilderResult::Enum rc = putEventBuilder.packMessage(
        pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // The second message has associated CorrelationId and ACK_REQUESTED flag
    putEventBuilder.startMessage();
    putEventBuilder.setMessageGUID(guid2)
        .setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setFlags(phFlags);
    rc = putEventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using event blob
    int res = obj.session().post(*putEventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    PVV_SAFE("Step 4. Verify PUT event is sent");
    obj.getOutboundEvent(&rawEvent);

    BMQTST_ASSERT(rawEvent.isPutEvent());

    PVV_SAFE("Step 5. Trigger channel drop and verify no NACK event is sent");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    PVV_SAFE("Step 6. Send one more PUT message while the channel is down");
    putEventBuilder.reset();
    putEventBuilder.startMessage();
    putEventBuilder.setMessagePayload(k_PAYLOAD, k_PAYLOAD_LEN)
        .setMessageGUID(guid3);

    rc = putEventBuilder.packMessage(pQueue->id());

    BMQTST_ASSERT_EQ(rc, bmqt::EventBuilderResult::e_SUCCESS);

    // Post the event using event blob
    res = obj.session().post(*putEventBuilder.blob(), timeout);

    BMQTST_ASSERT_EQ(res, bmqt::PostResult::e_SUCCESS);

    // Restore the connection
    obj.setChannel();

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    PVV_SAFE("Step 7. Reopen the queue");
    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 8. Verify two PUT events are retransmitted");
    rawEvent.clear();

    obj.getOutboundEvent(&rawEvent);

    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT(putIter.isValid());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(pQueue->id(), putIter.header().queueId());
    BMQTST_ASSERT_EQ(guid2, putIter.header().messageGUID());

    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(pQueue->id(), putIter.header().queueId());
    BMQTST_ASSERT_EQ(guid3, putIter.header().messageGUID());
    BMQTST_ASSERT_EQ(0, putIter.next());

    PVV_SAFE("Step 9. Trigger channel restart once again");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    // Restore the connection
    obj.setChannel();

    BMQTST_ASSERT(obj.waitReconnectedEvent());

    PVV_SAFE("Step 10. Reopen the queue");
    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    PVV_SAFE("Step 11. Verify single PUT event is retransmitted");
    rawEvent.clear();

    obj.getOutboundEvent(&rawEvent);

    rawEvent.loadPutMessageIterator(&putIter, true);

    BMQTST_ASSERT(putIter.isValid());
    BMQTST_ASSERT_EQ(1, putIter.next());
    BMQTST_ASSERT_EQ(pQueue->id(), putIter.header().queueId());
    BMQTST_ASSERT_EQ(guid2, putIter.header().messageGUID());
    BMQTST_ASSERT_EQ(0, putIter.next());

    PVV_SAFE("Step 12. "
             "Drop the channel, close the queue and verify NACK event");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    obj.session().closeQueueAsync(pQueue, timeout);

    bsl::shared_ptr<bmqimp::Event> nackEvent = obj.waitAckEvent();

    BMQTST_ASSERT(nackEvent);

    bmqp::AckMessageIterator* iter = nackEvent->ackMessageIterator();
    BMQTST_ASSERT_EQ(1, iter->next());
    BMQTST_ASSERT_EQ(pQueue->id(), iter->message().queueId());
    BMQTST_ASSERT_EQ(guid2, iter->message().messageGUID());
    BMQTST_ASSERT_EQ(k_ACK_STATUS_UNKNOWN, iter->message().status());
    BMQTST_ASSERT_EQ(1, nackEvent->numCorrrelationIds());
    BMQTST_ASSERT_EQ(corrId, nackEvent->correlationId(0));
    BMQTST_ASSERT_EQ(0, iter->next());

    PVV_SAFE("Step 13. Waiting QUEUE_CLOSE_RESULT event...");
    obj.verifyCloseQueueResult(bmqp_ctrlmsg::StatusCategory::E_SUCCESS,
                               pQueue);

    PVV_SAFE("Step 14. Stop the session when not connected");
    BMQTST_ASSERT(obj.stop());

    BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT_EQ(pQueue->isValid(), false);
    rawEvent.clear();
}

static void test52_controlRetransmission()
// ------------------------------------------------------------------------
// CONTROL RETRANSMISSION TEST
//
// Concerns:
//   1. Check the open and configure queue requests may be done when the
//      session has no connection with the broker.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Trigger channel down and call openQueueAsync. Verify no error is
//      reported.
//   3. Restore the channel and verify the open queue request is sent and
//      queue can be opened successfully.
//   4. Repeat the steps 2 and 3 for configureQueueAsync.
//   5. Repeat the steps 2 for configureQueueAsync and emulate request
//      timeout.
//   6. Restore the channel and verify the expired request is not
//      retransmitted.
//   7. Reset the channel, configure the queue, then close it.
//   8. Restore the channel and check there is no configure request.
//   9. Stop the session.
//
// Testing manipulators:
//   - start
//   - openQueueAsync
//   - configureQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CONTROL RETRANSMISSION TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ, queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Reset the channel and open a queue");

    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    int rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    PVV_SAFE("Step 3. Restore the channel and finish queue opening");

    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());
    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // Advance time to verify the request is handled in the FSM.
    obj.advanceTime(TestSession::k_TIME_SOURCE_STEP);

    BMQTST_ASSERT(pQueue->pendingConfigureId() !=
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    obj.sendResponse(request);

    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    BMQTST_ASSERT(pQueue->pendingConfigureId() ==
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    PVV_SAFE("Step 4. Reset the channel and configure the queue");

    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

    PVV_SAFE("Step 5. Restore the channel and check the queue is configured");

    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());

    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    // Advance time to verify the request is handled in the FSM.
    obj.advanceTime(TestSession::k_TIME_SOURCE_STEP);

    BMQTST_ASSERT(pQueue->pendingConfigureId() !=
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    obj.sendResponse(request);

    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));

    BMQTST_ASSERT(pQueue->pendingConfigureId() ==
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    PVV_SAFE("Step 6. "
             "Reset the channel, configure the queue, emulate timeout");

    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

    // Emulate request timeout.
    obj.advanceTime(timeout);

    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

    BMQTST_ASSERT(pQueue->pendingConfigureId() ==
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    PVV_SAFE("Step 7. "
             "Restore the channel and check there is no configure request");

    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());

    obj.reopenQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    BMQTST_ASSERT(obj.isChannelEmpty());

    PVV_SAFE("Step 8. Reset the channel, configure the queue, then close it");

    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());
    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    rc = obj.session().configureQueueAsync(pQueue, pQueue->options(), timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

    rc = obj.session().closeQueue(pQueue, timeout);

    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_NOT_SUPPORTED));

    BMQTST_ASSERT(pQueue->pendingConfigureId() ==
                  bmqimp::Queue::k_INVALID_CONFIGURE_ID);

    PVV_SAFE("Step 9. Restore the channel and check there is no configure "
             "request");

    obj.setChannel();
    BMQTST_ASSERT(obj.waitReconnectedEvent());
    BMQTST_ASSERT(obj.waitStateRestoredEvent());

    BMQTST_ASSERT(obj.isChannelEmpty());

    PVV_SAFE("Step 10. Stop the session");

    obj.stopGracefully();
}

static void queueExpired(bsls::Types::Uint64 queueFlags)
{
    bmqtst::TestHelper::printTestName("QUEUE EXPIRED TEST");

    const bsls::TimeInterval timeout = bsls::TimeInterval(500);

    bmqt::SessionOptions  sessionOptions;
    bmqt::QueueOptions    queueOptions;
    bdlmt::EventScheduler scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock             testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue = obj.createQueue(k_URI,
                                                            queueFlags,
                                                            queueOptions);

    PVV_SAFE("Step 1. Start the session");
    obj.startAndConnect();

    PVV_SAFE("Step 2. Trigger channel drop");
    obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

    BMQTST_ASSERT(obj.waitConnectionLostEvent());

    PVV_SAFE("Step 3. Check expired open request");
    {
        int rc = obj.session().openQueueAsync(pQueue, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

        // Emulate request timeout.
        obj.advanceTime(timeout);

        PVV_SAFE("Step 4. Waiting QUEUE_OPEN_RESULT event...");
        BMQTST_ASSERT(obj.verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
            bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

        // The queue is closed and can be reused
        BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);

        PVV_SAFE("Step 5. Restore the channel");

        obj.setChannel();
        BMQTST_ASSERT(obj.waitReconnectedEvent());
        BMQTST_ASSERT(obj.waitStateRestoredEvent());
    }

    PVV_SAFE("Step 6. Check expired open-configure request");
    {
        // Applicable only for reader since writer has no open-configure step
        if (bmqt::QueueFlagsUtil::isReader(queueFlags)) {
            int rc = obj.session().openQueueAsync(pQueue, timeout);

            BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

            obj.sendResponse(
                obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE));

            PVV_SAFE("Step 7. Verify config request was sent");
            obj.verifyRequestSent(TestSession::e_REQ_CONFIG_QUEUE);

            PVV_SAFE("Step 8. Trigger channel drop");
            obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

            BMQTST_ASSERT(obj.waitConnectionLostEvent());

            // Emulate request timeout.
            obj.advanceTime(timeout);

            PVV_SAFE("Step 9. Waiting QUEUE_OPEN_RESULT event...");
            BMQTST_ASSERT(obj.verifyOperationResult(
                bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                bmqp_ctrlmsg::StatusCategory::E_TIMEOUT));

            // The queue should be closed
            BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_CLOSED);

            PVV_SAFE("Step 10. Restore the channel");

            obj.setChannel();
            BMQTST_ASSERT(obj.waitReconnectedEvent());
            BMQTST_ASSERT(obj.waitStateRestoredEvent());
        }
    }

    PVV_SAFE("Step 11. Check expired standalone configure request");
    {
        obj.openQueue(pQueue, timeout);

        PVV_SAFE("Step 12. Trigger channel drop");
        obj.session().setChannel(bsl::shared_ptr<bmqio::Channel>());

        BMQTST_ASSERT(obj.waitConnectionLostEvent());

        PVV_SAFE("Step 13. Checking async configure...");
        int rc = obj.session().configureQueueAsync(pQueue,
                                                   queueOptions,
                                                   timeout);
        BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);

        // Emulate request timeout.
        obj.advanceTime(timeout);

        PVV_SAFE("Step 14. Waiting QUEUE_CONFIGURE_RESULT event...");

        // For writer configure is always successful. For reader expect
        // timeout.
        bmqp_ctrlmsg::StatusCategory::Value configRes =
            bmqp_ctrlmsg::StatusCategory::E_SUCCESS;
        if (bmqt::QueueFlagsUtil::isReader(queueFlags)) {
            configRes = bmqp_ctrlmsg::StatusCategory::E_TIMEOUT;
        }

        BMQTST_ASSERT(obj.verifyOperationResult(
            bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
            configRes));

        // The queue is PENDING
        BMQTST_ASSERT_EQ(pQueue->state(), bmqimp::QueueState::e_PENDING);
    }

    PVV_SAFE("Step 15. Stop the session");
    BMQTST_ASSERT(obj.stop());
}

static void test53_queueExpired()
// ------------------------------------------------------------------------
// QUEUE EXPIRED TEST
//
// Concerns:
//   1. Check the behavior of the buffered requests when they are timed out
//      while there is no connection with the broker.
//
// Plan:
//   1. Create bmqimp::BrokerSession test wrapper object
//      and start the session with a test network channel.
//   2. Drop the channel.
//   3. Open a queue and emulate request timeout.
//   4. Verify open queue result shows E_TIMEOUT error and the queue is
//      closed.
//   5. Restore the channel.
//   6. Open the queue again and send open queue respose.
//   7. Verify open-configure request is sent.
//   8. Drop the channel and emulate request timeout.
//   9. Verify queue open result shows E_TIMEOUT error and the queue is
//      closed.
//   10. Restore the channel.
//   11. Fully open the queue.
//   12. Drop the channel.
//   13. Invoke configure request and emulate request timeout.
//   14. Verify configure result (E_TIMEOUT for reader) and the queue is in
//       PENDING state.
//   15. Stop the session.
//
// Testing manipulators:
//   - start
//   - setChannel
//   - openQueue
//   - configureQueue
//   - stop
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("QUEUE EXPIRED TEST");

    PVV_SAFE("Check READER");
    queueExpired(bmqt::QueueFlags::e_READ);

    PVV_SAFE("Check WRITER");
    queueExpired(bmqt::QueueFlags::e_WRITE);

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    PVV_SAFE("Check READER and WRITER");
    queueExpired(flags);
}

static void test54_distributedTrace()
// ------------------------------------------------------------------------
// DISTRIBUTED TRACE TEST
//
// Concerns:
//   1. Verify that 'BrokerSession' created Distributed Trace spans for:
//      StartSession, StopSession, OpenQueue, ConfigureQueue, CloseQueue.
//   2. Verify that if a span is active when an operation begins, the
//      span representing that operation is a child of the active span.
//   3. Verify that spans are created and torn down in the expected order.
//   4. Verify that no unexpected spans are created (e.g., for queues that
//      fail to open, no configure-span for write-only queues, etc).
//   5. Verify that baggage metadata is propagated as expected.
//
// Plan:
//   1. Start the session. Verify that a 'bmq.session.start' span reports
//      its start and end of execution.
//   2. Open a queue, while an existing span is active. Verify that the
//      open-span is created as a child of the active one. Destroy the
//      parent span before the response to the open-request is received.
//      Verify that the parent span reports its end of execution without
//      waiting for the end of the child operation. Receive the response,
//      and verify that a 'bmq.queue.openConfigure' span is created as a
//      child of the 'bmq.queue.open' span. Finish opening the queue, and
//      verify that all spans are as expected.
//   3. Reconfigure the queue. Verify that a 'bmq.queue.configure' span is
//      created, with the expected baggage. Complete the operation, and
//      verify that it reports its end of execution.
//   4. Close the queue, and verify that this initiates a 'bmq.queue.close'
//      span, as well as a 'bmq.queue.closeConfigure' span as its child.
//      Verify that both spans report their end of execution as expected.
//   5. Observe a failure to open a queue. Verify that a 'bmq.queue.open'
//      span is created, and terminates without creating a span for
//      'bmq.queue.openConfigure'.
//   6. Open a write-only queue. Verify that a 'bmq.queue.open' span is
//      created, and terminates without creating a span for
//      'bmq.queue.openConfigure'.
//   7. Stop the session with the write-only queue still open. Verify that
//      the 'bmq.session.step' span reports its start and end of execution,
//      without creating any subsequent queue-level spans (since no request
//      is sent to the broker to close the queue).
//
//   Note that this test uses test-only implementations of 'bmqpi::DTSpan',
//   'bmqpi::DTTracer', and 'bmqpi::DTContext', which are all implemented
//   above. The format for "events" reported by 'DTTestSpan' is:
//     EV {Operation} [< {AncestorOperation}]*[; BaggageKey=BaggageValue]*
//   where 'EV' is 'START' or 'END', based on whether the event records the
//   start or end of a span of execution. Note that 'END'-events do not
//   record baggage-values. e.g.,
//     START bmq.session.start < parent; date=2021/02/01; time=12:00:00pm
//     END bmq.session.start < parent
//
// Testing manipulators:
//   - start
//   - openQueueAsync
//   - configureQueueAsync
//   - closeQueueAsync
//   - stop
//   ----------------------------------------------------------------------
{
    using namespace bdlf::PlaceHolders;

    bmqtst::TestHelper::printTestName("DISTRIBUTED TRACE TEST");

    struct localFns {
        static void addSpacer(bdlcc::Deque<bsl::string>& events)
        {
            events.pushBack("spacer");
        }

        static void getSpacer(bdlcc::Deque<bsl::string>& events)
        {
            bsl::string spacer(bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_EQ(events.tryPopFront(&spacer), 0);
            BMQTST_ASSERT_EQ(spacer, "spacer");
        }

        static void fillEventBufferFn(bsl::vector<bsl::string>&  buffer,
                                      bdlcc::Deque<bsl::string>& events,
                                      size_t                     expectedNum)
        {
            getSpacer(events);

            // Get the desired number of events from the concurrent dequeue.
            // The test expected to fail if number of events is not sufficient
            // with the given default timeout.
            buffer.resize(expectedNum);
            for (size_t i = 0; i < expectedNum; i++) {
                BMQTST_ASSERT_EQ(
                    events.timedPopFront(&buffer[i],
                                         bdlt::CurrentTime::now() +
                                             bsls::TimeInterval(0.1)),
                    0);
            }

            addSpacer(events);
        }

        static void checkNoEvents(bdlcc::Deque<bsl::string>& events)
        {
            getSpacer(events);

            // Make sure no other events arrive with the given default timeout.
            bsl::string emptyStr(bmqtst::TestHelperUtil::allocator());
            BMQTST_ASSERT_NE(
                events.timedPopFront(&emptyStr,
                                     bdlt::CurrentTime::now() +
                                         bsls::TimeInterval(0.01)),
                0);
        }
    };

    bsl::vector<bsl::string>  dtEvents(bmqtst::TestHelperUtil::allocator());
    bdlcc::Deque<bsl::string> dtEventsQueue(
        bmqtst::TestHelperUtil::allocator());

    // Contract.  Each event buffer fill is surrounded by spacers:
    // > addSpacer        (first spacer for convenience)
    // ...
    // > BrokerSession...
    // < getSpacer        (fillEventBufferFn)
    // < pop N[k] events  (fillEventBufferFn)
    // > addSpacer        (fillEventBufferFn)
    // ...
    // < getSpacer        (checkNoEvents)
    localFns::addSpacer(dtEventsQueue);

    bsl::shared_ptr<bmqpi::DTContext> dtContext(
        new (*bmqtst::TestHelperUtil::allocator())
            DTTestContext(bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqpi::DTTracer> dtTracer(
        new (*bmqtst::TestHelperUtil::allocator())
            DTTestTracer(&dtEventsQueue, bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());

    const bsls::TimeInterval timeout = bsls::TimeInterval(15);
    bmqt::SessionOptions     sessionOptions;
    bmqt::QueueOptions       queueOptions;
    bdlmt::EventScheduler    scheduler(bsls::SystemClockType::e_MONOTONIC,
                                    bmqtst::TestHelperUtil::allocator());
    TestClock                testClock(scheduler);

    sessionOptions.setNumProcessingThreads(1).setTraceOptions(dtContext,
                                                              dtTracer);

    TestSession obj(sessionOptions,
                    testClock,
                    bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Queue> pQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_READ, queueOptions);

    PVV_SAFE("Step 1. Starting session...");
    obj.startAndConnect();
    BMQTST_ASSERT(obj.isChannelEmpty());

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 2u);
    BMQTST_ASSERT_EQ(dtEvents[0], "START bmq.session.start");
    BMQTST_ASSERT_EQ(dtEvents[1], "END bmq.session.start");
    dtEvents.clear();
    {
        bmqpi::DTSpan::Baggage baggage(bmqtst::TestHelperUtil::allocator());
        baggage.put("tag", "value");

        bsl::shared_ptr<bmqpi::DTSpan> span(
            new (*bmqtst::TestHelperUtil::allocator())
                DTTestSpan("test54",
                           baggage,
                           &dtEventsQueue,
                           bmqtst::TestHelperUtil::allocator()),
            bmqtst::TestHelperUtil::allocator());
        bslma::ManagedPtr<void> scopeGuard(dtContext->scope(span));

        PVV_SAFE("Step 2. Open a queue...");
        int rc = obj.session().openQueueAsync(pQueue, timeout);
        BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);
    }
    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 3u);
    BMQTST_ASSERT_EQ(dtEvents[0], "START test54; tag=value");
    BMQTST_ASSERT_EQ(dtEvents[1],
                     "START bmq.queue.open < test54; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    BMQTST_ASSERT_EQ(dtEvents[2], "END test54");
    dtEvents.clear();

    bmqp_ctrlmsg::ControlMessage request = obj.getNextOutboundRequest(
        TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);
    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 1u);
    BMQTST_ASSERT_EQ(
        dtEvents[0],
        "START bmq.queue.openConfigure < bmq.queue.open < test54; "
        "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    dtEvents.clear();

    obj.sendResponse(request);
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT(obj.isChannelEmpty());

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 2u);
    BMQTST_ASSERT_EQ(dtEvents[0], "END bmq.queue.open < test54");
    BMQTST_ASSERT_EQ(dtEvents[1],
                     "END bmq.queue.openConfigure < bmq.queue.open < test54");
    dtEvents.clear();

    PVV_SAFE("Step 3. Configure a queue");
    BMQTST_ASSERT(obj.channel().writeCalls().empty());
    int rc = obj.session().configureQueueAsync(pQueue,
                                               pQueue->options(),
                                               timeout);
    BMQTST_ASSERT_EQ(rc, bmqt::ConfigureQueueResult::e_SUCCESS);
    BMQTST_ASSERT(obj.channel().waitFor(1, false, bsls::TimeInterval(1)));

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 1u);
    BMQTST_ASSERT_EQ(dtEvents[0],
                     "START bmq.queue.configure; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    dtEvents.clear();

    request = obj.getNextOutboundRequest(TestSession::e_REQ_CONFIG_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(obj.verifyOperationResult(
        bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT,
        bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT(obj.isChannelEmpty());

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 1u);
    BMQTST_ASSERT_EQ(dtEvents[0], "END bmq.queue.configure");
    dtEvents.clear();

    PVV_SAFE("Step 4. Close a queue");
    obj.closeQueue(pQueue, timeout, true);

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 4u);
    BMQTST_ASSERT_EQ(dtEvents[0],
                     "START bmq.queue.close; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    BMQTST_ASSERT_EQ(dtEvents[1],
                     "START bmq.queue.closeConfigure < bmq.queue.close; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    BMQTST_ASSERT_EQ(dtEvents[2],
                     "END bmq.queue.closeConfigure < bmq.queue.close");
    BMQTST_ASSERT_EQ(dtEvents[3], "END bmq.queue.close");
    dtEvents.clear();

    PVV_SAFE("Step 5. Fail to open a queue");
    rc = obj.session().openQueueAsync(pQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    request = obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);
    obj.sendStatus(request);
    obj.verifyOpenQueueErrorResult(bmqp_ctrlmsg::StatusCategory::E_UNKNOWN,
                                   pQueue,
                                   bmqimp::QueueState::e_CLOSED);
    BMQTST_ASSERT(obj.isChannelEmpty());

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 2u);
    BMQTST_ASSERT_EQ(dtEvents[0],
                     "START bmq.queue.open; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    BMQTST_ASSERT_EQ(dtEvents[1], "END bmq.queue.open");
    dtEvents.clear();

    PVV_SAFE("Step 6. Open a write-only queue");
    bsl::shared_ptr<bmqimp::Queue> pWriterQueue =
        obj.createQueue(k_URI, bmqt::QueueFlags::e_WRITE, queueOptions);
    rc = obj.session().openQueueAsync(pWriterQueue, timeout);
    BMQTST_ASSERT_EQ(rc, bmqp_ctrlmsg::StatusCategory::E_SUCCESS);

    request = obj.getNextOutboundRequest(TestSession::e_REQ_OPEN_QUEUE);
    obj.sendResponse(request);
    BMQTST_ASSERT(
        obj.verifyOperationResult(bmqt::SessionEventType::e_QUEUE_OPEN_RESULT,
                                  bmqp_ctrlmsg::StatusCategory::E_SUCCESS));
    BMQTST_ASSERT(obj.isChannelEmpty());

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 2u);
    BMQTST_ASSERT_EQ(dtEvents[0],
                     "START bmq.queue.open; "
                     "bmq.queue.uri=bmq://ts.trades.myapp/my.queue?id=my.app");
    BMQTST_ASSERT_EQ(dtEvents[1], "END bmq.queue.open");
    dtEvents.clear();

    PVV_SAFE("Step 7. Stop the session (with queue open)");
    obj.stopGracefully();

    localFns::fillEventBufferFn(dtEvents, dtEventsQueue, 2u);
    BMQTST_ASSERT_EQ(dtEvents[0], "START bmq.session.stop");
    BMQTST_ASSERT_EQ(dtEvents[1], "END bmq.session.stop");
    dtEvents.clear();

    PVV_SAFE("Step 8. Re-stopping the session does not produce more events");
    obj.session().stop();
    localFns::checkNoEvents(dtEventsQueue);
}

static void test55_queueAsyncCanceled2()
{
    bmqtst::TestHelper::printTestName("QUEUE ASYNC CANCELED TEST 2");

    queueAsyncCanceled_REOPEN_OPENING();

    queueAsyncCanceled_REOPEN_CONFIGURING();
}

static void test56_queueAsyncCanceled3()
{
    bmqtst::TestHelper::printTestName("QUEUE ASYNC CANCELED TEST 3");

    queueAsyncCanceled_CONFIGURING();

    queueAsyncCanceled_CONFIGURING_RECONFIGURING();
}

static void test57_queueAsyncCanceled4()
{
    bmqtst::TestHelper::printTestName("QUEUE ASYNC CANCELED TEST 4");

    queueAsyncCanceled_CLOSE_CONFIGURING();
}

static void test58_queueAsyncCanceled5()
{
    bmqtst::TestHelper::printTestName("QUEUE ASYNC CANCELED TEST 5");

    queueAsyncCanceled_CLOSE_CLOSING();
}

static void test59_queueLateAsyncCanceledReader2()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED READER TEST 2");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_OPEN_CONFIGURING_CLS,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_REOPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test60_queueLateAsyncCanceledReader3()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED READER TEST 3");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_REOPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_RECONFIGURING,
                           bmqimp::QueueState::e_PENDING);
}

static void test61_queueLateAsyncCanceledReader4()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED READER TEST 4");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_CLOSE_CONFIGURING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test62_queueLateAsyncCanceledReader5()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED READER TEST 5");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_READ,
                           TestSession::e_LATE_CLOSE_CLOSING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test63_queueLateAsyncCanceledWriter2()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED WRITER TEST 2");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_OPEN_CONFIGURING_CLS,
                           bmqimp::QueueState::e_PENDING);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_REOPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test64_queueLateAsyncCanceledWriter3()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED WRITER TEST 3");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_REOPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_PENDING);

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_RECONFIGURING,
                           bmqimp::QueueState::e_PENDING);
}

static void test65_queueLateAsyncCanceledWriter4()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED WRITER TEST 4");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_CLOSE_CONFIGURING,
                           bmqimp::QueueState::e_PENDING);
}

static void test66_queueLateAsyncCanceledWriter5()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED WRITER TEST 5");

    queueLateAsyncCanceled(L_,
                           bmqt::QueueFlags::e_WRITE,
                           TestSession::e_LATE_CLOSE_CLOSING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test67_queueLateAsyncCanceledHybrid2()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED HYBRID TEST 2");

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_OPEN_CONFIGURING_CLS,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_REOPEN_OPENING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test68_queueLateAsyncCanceledHybrid3()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED HYBRID TEST 3");

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_REOPEN_CONFIGURING_CFG,
                           bmqimp::QueueState::e_CLOSED);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_RECONFIGURING,
                           bmqimp::QueueState::e_PENDING);
}

static void test69_queueLateAsyncCanceledHybrid4()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED HYBRID TEST 4");

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_CLOSE_CONFIGURING,
                           bmqimp::QueueState::e_CLOSED);
}

static void test70_queueLateAsyncCanceledHybrid5()
{
    bmqtst::TestHelper::printTestName(
        "QUEUE LATE ASYNC CANCELED HYBRID TEST 5");

    bsls::Types::Uint64 flags = 0;
    bmqt::QueueFlagsUtil::setReader(&flags);
    bmqt::QueueFlagsUtil::setWriter(&flags);

    queueLateAsyncCanceled(L_,
                           flags,
                           TestSession::e_LATE_CLOSE_CLOSING,
                           bmqimp::QueueState::e_CLOSED);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());
    bmqp::Crc32c::initialize();

    switch (_testCase) {
    case 0:
    case 70: test70_queueLateAsyncCanceledHybrid5(); break;
    case 69: test69_queueLateAsyncCanceledHybrid4(); break;
    case 68: test68_queueLateAsyncCanceledHybrid3(); break;
    case 67: test67_queueLateAsyncCanceledHybrid2(); break;
    case 66: test66_queueLateAsyncCanceledWriter5(); break;
    case 65: test65_queueLateAsyncCanceledWriter4(); break;
    case 64: test64_queueLateAsyncCanceledWriter3(); break;
    case 63: test63_queueLateAsyncCanceledWriter2(); break;
    case 62: test62_queueLateAsyncCanceledReader5(); break;
    case 61: test61_queueLateAsyncCanceledReader4(); break;
    case 60: test60_queueLateAsyncCanceledReader3(); break;
    case 59: test59_queueLateAsyncCanceledReader2(); break;
    case 58: test58_queueAsyncCanceled5(); break;
    case 57: test57_queueAsyncCanceled4(); break;
    case 56: test56_queueAsyncCanceled3(); break;
    case 55: test55_queueAsyncCanceled2(); break;
    case 54: test54_distributedTrace(); break;
    case 53: test53_queueExpired(); break;
    case 52: test52_controlRetransmission(); break;
    case 51: test51_putRetransmittingNoAckTest(); break;
    case 50: test50_putRetransmittingTest(); break;
    case 49: test49_controlsBuffering(); break;
    case 48: test48_hostHealthSensitivityReconfiguration(); break;
    case 47: test47_configureMergesQueueOptions(); break;
    case 46: test46_hostHealthMonitoringDeferredResumeClose(); break;
    case 45: test45_hostHealthMonitoringPendingStandalone(); break;
    case 44: test44_hostHealthMonitoringMultipleQueues(); break;
    case 43: test43_hostHealthMonitoringErrors(); break;
    case 42: test42_asyncOpenConfigureCloseWithoutHandler(); break;
    case 41: test41_asyncOpenConfigureCloseWithHandler(); break;
    case 40: test40_syncCalledFromEventHandler(); break;
    case 39: test39_syncOpenConfigureCloseWithoutHandler(); break;
    case 38: test38_syncOpenConfigureCloseWithHandler(); break;
    case 37: test37_closingPendingConfig(); break;
    case 36: test36_closingAfterConfigTimeout(); break;
    case 35: test35_hostHealthMonitoring(); break;
    case 34: test34_reopenError(); break;
    case 33: test33_queueNackTest(); break;
    case 32: test32_queueDoubleOpenCorrelationId(); break;
    case 31: test31_queueDoubleOpenUri(); break;
    case 30: test30_queueLateAsyncCanceledHybrid1(); break;
    case 29: test29_queueLateAsyncCanceledWriter1(); break;
    case 28: test28_queueLateAsyncCanceledReader1(); break;
    case 27: test27_openCloseMultipleSubqueuesWithErrors(); break;
    case 26: test26_openCloseMultipleSubqueues(); break;
    case 25: test25_sessionFsmTable(); break;
    case 24: test24_queueAsyncCanceled1(); break;
    case 23: test23_queueCloseSync(); break;
    case 22: test22_confirm_Limit(); break;
    case 21: test21_post_Limit(); break;
    case 20: test20_queueOpen_LateConfigureQueueResponse(); break;
    case 19: test19_queueOpen_LateOpenQueueResponse(); break;
    case 18: test18_disconnectRequestLimit(); break;
    case 17: test17_disconnectRequestCanceled(); break;
    case 16: test16_disconnectRequestTimeout(); break;
    case 15: test15_disconnectRequestConnection(); break;
    case 14: test14_disconnectRequestGenericErr(); break;
    case 13: test13_disconnectTimeout(); break;
    case 12: test12_disconnectStatus(); break;
    case 11: test11_disconnect(); break;
    case 10: test10_queueOpenCloseAsync(); break;
    case 9: test9_queueOpenErrorTest(); break;
    case 8: test8_queueWriterConfigureTest(); break;
    case 7: test7_queueOpenTimeoutTest(); break;
    case 6: test6_setChannelTest(); break;
    case 5: test5_queueErrorsTest(); break;
    case 4: test4_createEventTest(); break;
    case 3: test3_nullChannelTest(); break;
    case 2: test2_basicAccessorsTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqsys::Time::shutdown();
    bmqt::UriParser::shutdown();
    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_DEFAULT);
    // Global:  The global allocator is used to initialize the
    //          bmqt::URIParser RegEx.
    // Default: EventQueue uses bmqc::MonitoredFixedQueue, which uses
    //          'bdlcc::SharedObjectPool' which uses bslmt::Semaphore which
    //          generates a unique name using an ostringstream, hence the
    //          default allocator.
}
