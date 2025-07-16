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

// bmqimp_eventqueue.cpp                                              -*-C++-*-
#include <bmqimp_eventqueue.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqt_correlationid.h>

#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>
#include <bmqst_tableutil.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlb_scopeexit.h>
#include <bdlf_bind.h>
#include <bdlf_memfn.h>
#include <bdlf_placeholder.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeutil.h>

namespace BloombergLP {
namespace bmqimp {

namespace {
BALL_LOG_SET_NAMESPACE_CATEGORY("BMQIMP.EVENTQUEUE");

/// Name of the stat context to create for holding this component's stats
const char k_STAT_NAME[] = "EventQueue";

enum {
    // Index of the different stat values
    k_STAT_QUEUE = 0  // Queue/Dequeue
    ,
    k_STAT_TIME = 1  // Event queued time
};

}  // close unnamed namespace

// ----------------------------
// struct EventQueue::QueueItem
// ----------------------------

EventQueue::QueueItem::QueueItem()
: d_event_sp(0)
, d_enqueueTime(0)
{
    // NOTHING
}

EventQueue::QueueItem::QueueItem(const bsl::shared_ptr<Event>& event,
                                 bsls::Types::Int64            enqueueTime)
: d_event_sp(event)
, d_enqueueTime(enqueueTime)
{
    // NOTHING
}

// ----------------
// class EventQueue
// ----------------
bsl::shared_ptr<Event> EventQueue::getEvent()
{
    return d_eventPool_p->getObject();
}

void EventQueue::stateCallback(bmqc::MonitoredQueueState::Enum state)
{
    // Because of MessageEvent that should be dropped while SessionEvent should
    // still be queueable, we use two highWatermark levels, with the following
    // meanings:
    //: o lowWatermark:  the queue is back to its low watermark
    //: o highWatermark: the queue has reached the user provided high watermark
    //: o queueFilled:   should never happen with a resizable queue

    switch (state) {
    case bmqc::MonitoredQueueState::e_NORMAL: {
        BALL_LOG_INFO_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM << "EventQueue has reached its "
                                   << "low-watermark of "
                                   << d_queue.lowWatermark() << ", ";
            printLastEventTime(BALL_LOG_OUTPUT_STREAM);
        }

        // Enqueue a back to normal event
        bsl::shared_ptr<Event> event = getEvent();
        event->configureAsSessionEvent(
            bmqt::SessionEventType::e_SLOWCONSUMER_NORMAL);
        pushBack(event);
    } break;
    case bmqc::MonitoredQueueState::e_HIGH_WATERMARK_REACHED: {
        d_shouldEmitHighWatermark = 1;  // i.e., enqueue a HIGH watermark event

        // Print an alarm catchable string to stderr.  We can't use a
        // LogAlarmObserver to write to cerr because the task may not have BALL
        // observer.
        bmqu::MemOutStream os;
        os << "BMQALARM [EVENTQUEUE::HIGH_WATERMARK]: BlazingMQ EventQueue "
           << "(buffer between the events delivered by the broker and the "
           << "application processing them in the event handler) has reached "
           << "its high-watermark of " << d_queue.highWatermark() << ", ";
        printLastEventTime(os);
        bsl::cerr << os.str() << '\n' << bsl::flush;
        // Also print warning in users log
        BALL_LOG_WARN << os.str();
    } break;
    case bmqc::MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED: {
        BALL_LOG_ERROR
            << "EventQueue has reached an un-expected highWatermark2 state";
        BSLS_ASSERT_SAFE(false &&
                         "Impossible - highWatermark2 is not reachable");
    } break;
    case bmqc::MonitoredQueueState::e_QUEUE_FILLED: {
        BALL_LOG_ERROR
            << "EventQueue has reached an un-expected queue filled state";
        // This should NEVER happen while using 'bdlcc::SingleProducerQueue'.
        BSLS_ASSERT_SAFE(false && "Impossible - Queue has reached capacity");
    } break;
    default: {
        BALL_LOG_ERROR_BLOCK
        {
            BALL_LOG_OUTPUT_STREAM
                << "BlazingMQ EventQueue (buffer between the events delivered "
                   "by the"
                   " broker and the application processing them in the event "
                   "handler) has reached an unknown state ("
                << state
                << "), "
                   " it contains "
                << d_queue.numElements() << ".";
            printLastEventTime(BALL_LOG_OUTPUT_STREAM);
        }
    } break;
    }
}

bool EventQueue::hasPriorityEvents(bsl::shared_ptr<Event>* event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event);

    // Check if should emit a SlowConsumer::HighWatermark event
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            d_shouldEmitHighWatermark.testAndSwap(1, 0) == 1)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        // Create a SlowConsumer::HighWatermark event
        *event = getEvent();
        (*event)->configureAsSessionEvent(
            bmqt::SessionEventType::e_SLOWCONSUMER_HIGHWATERMARK);

        // Update stats
        if (d_stats_mp) {
            d_stats_mp->adjustValue(k_STAT_QUEUE, 1);
        }

        return true;  // RETURN
    }

    return false;
}

void EventQueue::afterEventPopped(const QueueItem& item)
{
    const bsls::Types::Int64 popOutTime = bmqsys::Time::highResolutionTimer();
    const bsls::Types::Int64 queuedTime = popOutTime - item.d_enqueueTime;

    {  // d_lasPoppedOutSpinLock   LOCKED
        bsls::SpinLockGuard guard(&d_lastPoppedOutSpinLock);
        d_lastPoppedOutTime = popOutTime;
        d_lastInQueueTime   = queuedTime;
    }  // d_lasPoppedOutSpinLock UNLOCKED

    BALL_LOG_TRACE_BLOCK
    {
        BALL_LOG_OUTPUT_STREAM << "Popped out: ";
        if (item.d_event_sp) {
            BALL_LOG_OUTPUT_STREAM << *item.d_event_sp;
        }
        else {
            BALL_LOG_OUTPUT_STREAM << "poison pill event";
        }
        BALL_LOG_OUTPUT_STREAM
            << " (queuedTime: "
            << bmqu::PrintUtil::prettyTimeInterval(queuedTime) << ")";
    }

    // Update stats
    if (d_stats_mp) {
        d_stats_mp->adjustValue(k_STAT_QUEUE, -1);
        d_stats_mp->reportValue(k_STAT_TIME, queuedTime);
    }
}

void EventQueue::printLastEventTime(bsl::ostream& stream)
{
    bsls::Types::Int64 poppedOutTime = 0;
    bsls::Types::Int64 queuedTime    = 0;

    {  // d_lasPoppedOutSpinLock   LOCKED
        bsls::SpinLockGuard guard(&d_lastPoppedOutSpinLock);
        poppedOutTime = d_lastPoppedOutTime;
        queuedTime    = d_lastInQueueTime;
    }  // d_lasPoppedOutSpinLock UNLOCKED

    if (queuedTime == 0) {
        stream << "no item was ever popped out from the queue.";
    }
    else {
        stream << "last item was popped out "
               << bmqu::PrintUtil::prettyTimeInterval(
                      bmqsys::Time::highResolutionTimer() - poppedOutTime)
               << " ago after spending "
               << bmqu::PrintUtil::prettyTimeInterval(queuedTime)
               << " in the queue.";
    }
}

void EventQueue::dispatchNextEvent()
{
    // executed by (one of) the *EVENT_THREAD_POOL* thread
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_eventHandler);

    BALL_LOG_INFO << id() << "EventHandler thread started "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";

    while (true) {
        const bsl::shared_ptr<Event> eventSp = popFront();

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!eventSp)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Empty event is the poison pill signal; terminate the thread
            break;  // BREAK
        }

        d_eventHandler(eventSp);
    }

    BALL_LOG_INFO << id() << "EventHandler thread terminated "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";
}

EventQueue::EventQueue(EventPool*                  eventPool,
                       int                         initialCapacity,
                       int                         lowWatermark,
                       int                         highWatermark,
                       const EventHandlerCallback& eventHandler,
                       int                         numProcessingThreads,
                       const SessionId&            sessionId,
                       bslma::Allocator*           allocator)
: d_allocator_p(allocator)
, d_eventPool_p(eventPool)
, d_queue(initialCapacity, true, allocator)
, d_threadPool_mp()
, d_eventHandler(bsl::allocator_arg, allocator, eventHandler)
, d_numProcessingThreads(numProcessingThreads)
, d_shouldEmitHighWatermark(0)
, d_lastPoppedOutSpinLock(bsls::SpinLock::s_unlocked)
, d_lastPoppedOutTime(0)
, d_lastInQueueTime(0)
, d_stats_mp(0)
, d_statTable(allocator)
, d_statTip(&d_statTable, allocator)
, d_statTipNoDelta(&d_statTable, allocator)
, d_pushBackSpinlock(bsls::SpinLock::s_unlocked)
, d_sessionId(sessionId)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT((eventHandler && numProcessingThreads > 0) ||
                    (!eventHandler && numProcessingThreads == 0));

    bmqu::MemOutStream outStream(d_allocator_p);
    outStream << "Creating event queue with" << " [lowWatermark: "
              << bmqu::PrintUtil::prettyNumber(lowWatermark)
              << ", highWatermark: "
              << bmqu::PrintUtil::prettyNumber(highWatermark)
              << ", initialCapacity: "
              << bmqu::PrintUtil::prettyNumber(initialCapacity);
    if (eventHandler) {
        outStream << ", using " << d_numProcessingThreads << " threads]";
    }
    else {
        outStream << ", NOT using eventHandler]";
    }

    BALL_LOG_INFO << id() << outStream.str();

    d_queue.setWatermarks(lowWatermark, highWatermark);
    d_queue.setStateCallback(
        bdlf::BindUtil::bind(&EventQueue::stateCallback,
                             this,
                             bdlf::PlaceHolders::_1));  // state
}

EventQueue::~EventQueue()
{
    stop();
}

void EventQueue::initializeStats(
    bmqst::StatContext*                       rootStatContext,
    const bmqst::StatValue::SnapshotLocation& start,
    const bmqst::StatValue::SnapshotLocation& end)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_stats_mp && "Stats already initialized");

    // Create stat sub-context
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    bmqst::StatContextConfiguration config(k_STAT_NAME, &localAllocator);
    config.value("Queue").value("Time", bmqst::StatValue::e_DISCRETE);
    d_stats_mp = rootStatContext->addSubcontext(config);

    // Create table

    // NOTE: Refer to the component documentation in the header for the meaning
    //       of each stats

    // Configure schema
    bmqst::TableSchema& schema = d_statTable.schema();
    schema.addColumn("enqueue_delta",
                     k_STAT_QUEUE,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("dequeue_delta",
                     k_STAT_QUEUE,
                     bmqst::StatUtil::decrementsDifference,
                     start,
                     end);
    schema.addColumn("size", k_STAT_QUEUE, bmqst::StatUtil::value, start);
    schema.addColumn("size_max",
                     k_STAT_QUEUE,
                     bmqst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("size_absmax",
                     k_STAT_QUEUE,
                     bmqst::StatUtil::absoluteMax);

    schema.addColumn("time_min",
                     k_STAT_TIME,
                     bmqst::StatUtil::rangeMin,
                     start,
                     end);
    schema.addColumn("time_avg",
                     k_STAT_TIME,
                     bmqst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("time_max",
                     k_STAT_TIME,
                     bmqst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("time_absmax", k_STAT_TIME, bmqst::StatUtil::absoluteMax);

    // Configure records
    bmqst::TableRecords& records = d_statTable.records();
    records.setContext(d_stats_mp.get());
    records.update();

    // Configure tip (With Delta)
    d_statTip.setColumnGroup("Queue");
    d_statTip.addColumn("enqueue_delta", "Enqueue (delta)").zeroString("");
    d_statTip.addColumn("dequeue_delta", "Dequeue (delta)").zeroString("");
    d_statTip.addColumn("size", "Size");
    d_statTip.addColumn("size_max", "Max");
    d_statTip.addColumn("size_absmax", "Abs. Max");

    d_statTip.setColumnGroup("Queue Time");
    d_statTip.addColumn("time_min", "Min")
        .printAsNsTimeInterval()
        .extremeValueString("");
    d_statTip.addColumn("time_avg", "Avg")
        .printAsNsTimeInterval()
        .extremeValueString("");
    d_statTip.addColumn("time_max", "Max")
        .printAsNsTimeInterval()
        .extremeValueString("");
    d_statTip.addColumn("time_absmax", "Abs. Max")
        .printAsNsTimeInterval()
        .extremeValueString("");

    // Configure tip (without delta)
    d_statTipNoDelta.setColumnGroup("Queue");
    d_statTipNoDelta.addColumn("size_absmax", "Abs. Max");

    d_statTipNoDelta.setColumnGroup("Queue Time");
    d_statTipNoDelta.addColumn("time_absmax", "Abs. Max")
        .printAsNsTimeInterval()
        .extremeValueString("");
}

int EventQueue::start()
{
    // Make sure the queue is empty (so that we can do start, stop, start, ...
    // sequence of operations).
    d_queue.reset();

    // Resets the stats
    if (d_stats_mp) {
        d_stats_mp->clearValues();
    }

    if (!d_eventHandler) {
        // Not using the eventHandler, nothing to do here ...
        BALL_LOG_INFO << id()
                      << "No event handler specified, event queue will not "
                      << "create internal threads for event processing.";
        return 0;  // RETURN
    }

    BALL_LOG_INFO << id() << "Starting EventQueue ThreadPool "
                  << "[numThreads: " << d_numProcessingThreads << "]";

    int rc = 0;

    // Guard to auto-delete the thread pool on failure
    bdlb::ScopeExitAny guard(bdlf::MemFnUtil::memFn(
        &bslma::ManagedPtr<bdlmt::FixedThreadPool>::reset,
        &d_threadPool_mp));

    bslmt::ThreadAttributes threadAttributes =
        bmqsys::ThreadUtil::defaultAttributes();
    threadAttributes.setThreadName("bmqEventQueue");
    d_threadPool_mp.load(new (*d_allocator_p)
                             bdlmt::FixedThreadPool(threadAttributes,
                                                    d_numProcessingThreads,
                                                    d_numProcessingThreads,
                                                    d_allocator_p),
                         d_allocator_p);

    // Start the ThreadPool
    rc = d_threadPool_mp->start();
    if (rc != 0) {
        BALL_LOG_ERROR << id() << "Failed starting EventQueue ThreadPool "
                       << "[rc: " << rc << "]";
        return -1;  // RETURN
    }

    // Enqueue 'numProcessingThreads' jobs
    for (int i = 0; i < d_threadPool_mp->numThreads(); ++i) {
        rc = d_threadPool_mp->tryEnqueueJob(
            bdlf::MemFnUtil::memFn(&EventQueue::dispatchNextEvent, this));
        if (rc != 0) {
            BALL_LOG_ERROR << id()
                           << "Failed to enqueue job to EventQueue ThreadPool "
                           << "[rc: " << rc << "]";
            d_threadPool_mp->stop();
            return -1;  // RETURN
        }
    }

    guard.release();

    return 0;
}

void EventQueue::stop()
{
    if (d_threadPool_mp && d_threadPool_mp->isStarted()) {
        // Enqueue one poison pill for each thread
        for (int i = 0; i < d_numProcessingThreads; ++i) {
            enqueuePoisonPill();
        }

        BALL_LOG_INFO << id() << "Stopping EventQueue ThreadPool...";
        d_threadPool_mp->stop();
        BALL_LOG_INFO << id() << "EventQueue ThreadPool stopped";
    }

    d_threadPool_mp.reset();
}

int EventQueue::pushBack(bsl::shared_ptr<Event>& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event);

    // This method is mostly called from the IO thread. We NEVER want to block
    // the IO thread, because it will push back contention to the broker side.
    // Instead, we read the event and drop it.

    BALL_LOG_TRACE << "Enqueuing " << *event;

    QueueItem item(event, bmqsys::Time::highResolutionTimer());
    int       rc = 0;

    {  // d_pushBackSpinlock   LOCKED
        bsls::SpinLockGuard guard(&d_pushBackSpinlock);
        rc = d_queue.tryPushBack(item);
    }  // d_pushBackSpinlock UNLOCKED

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false && "Impossible - failed to enqueue");
        BALL_LOG_ERROR << id() << "Failed to enqueue: " << *item.d_event_sp;
        return -1;  // RETURN
    }

    // Update stats
    if (d_stats_mp) {
        d_stats_mp->adjustValue(k_STAT_QUEUE, 1);
    }

    return 0;
}

bsl::shared_ptr<Event> EventQueue::popFront()
{
    bsl::shared_ptr<Event> event;

    // Check for priority events first
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(hasPriorityEvents(&event))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        afterEventPopped(
            QueueItem(event, bmqsys::Time::highResolutionTimer()));
        return event;  // RETURN
    }

    // Look in the queue
    QueueItem                   item;
    BSLA_MAYBE_UNUSED const int rc = d_queue.popFront(&item);
    BSLS_ASSERT_SAFE(rc == 0);
    event = item.d_event_sp;
    afterEventPopped(item);
    return event;
}

bsl::shared_ptr<Event>
EventQueue::timedPopFront(const bsls::TimeInterval& timeout,
                          const bsls::TimeInterval& now)
{
    bsl::shared_ptr<Event> event;

    // Check for priority events first
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(hasPriorityEvents(&event))) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        afterEventPopped(
            QueueItem(event, bmqsys::Time::highResolutionTimer()));
        return event;  // RETURN
    }

    const bsls::TimeInterval absTimeOut = timeout + now;
    // Look in the queue
    QueueItem item;
    const int rc = d_queue.timedPopFront(&item, absTimeOut);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        bmqt::SessionEventType::Enum type;
        bsl::string                  errorDescription;
        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(rc == -1)) {
            // We timed out.. create a timeout event
            type             = bmqt::SessionEventType::e_TIMEOUT;
            errorDescription = "No events to pop from queue during the "
                               "specified timeInterval";
        }
        else {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Error occurred
            type             = bmqt::SessionEventType::e_ERROR;
            errorDescription = "An error occurred while attempting to pop from"
                               " the queue during the specified timeInterval";
        }

        event = getEvent();
        event->configureAsSessionEvent(type,
                                       rc,
                                       bmqt::CorrelationId(),
                                       errorDescription);
        item.d_enqueueTime = bmqsys::Time::highResolutionTimer();
        item.d_event_sp    = event;
        // Update stats ('afterEventPopped()' will decrement the counter, so we
        // need to manually increment it here since we artificially created an
        // event).
        if (d_stats_mp) {
            d_stats_mp->adjustValue(k_STAT_QUEUE, 1);
        }
    }
    else {
        event = item.d_event_sp;
    }

    afterEventPopped(item);
    return event;
}

void EventQueue::enqueuePoisonPill()
{
    // PoisonPill has a null event
    QueueItem item(0, bmqsys::Time::highResolutionTimer());

    {  // d_pushBackSpinlock   LOCKED
        bsls::SpinLockGuard guard(&d_pushBackSpinlock);
        d_queue.tryPushBack(item);
    }  // d_pushBackSpinlock UNLOCKED

    // Update stats
    if (d_stats_mp) {
        d_stats_mp->adjustValue(k_STAT_QUEUE, 1);
    }
}

void EventQueue::printStats(bsl::ostream& stream, bool includeDelta) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(d_stats_mp && "Stats NOT initialized");

    if (includeDelta) {
        bmqst::TableUtil::printTable(stream, d_statTip);
    }
    else {
        bmqst::TableUtil::printTable(stream, d_statTipNoDelta);
    }
    stream << "\n";
}

inline const SessionId& EventQueue::id() const
{
    return d_sessionId;
}

}  // close package namespace
}  // close enterprise namespace
