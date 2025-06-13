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

// bmqimp_eventqueue.h                                                -*-C++-*-
#ifndef INCLUDED_BMQIMP_EVENTQUEUE
#define INCLUDED_BMQIMP_EVENTQUEUE

//@PURPOSE: Provide a thread safe queue of pooled bmqimp::Event items.
//
//@CLASSES:
//  bmqimp::EventQueue: Thread safe queue of pooled bmqimp::Event items.
//
//@SEE_ALSO:
//  bmqimp::Event: Type of the items being used by this queue.
//
//@DESCRIPTION: 'bmqimp::EventQueue' provides an efficient queue of pooled
// items of type 'Event'. The EventQueue can be configured to create
// 'numProcessingThreads' that will read events from the queue and dispatch
// them to a provided callback method.
//
// The queue has a built-in monitoring mechanism that will emit alarms when it
// reaches certain user-customizable thresholds.
//
/// Statistics
///----------
// If configured for the queue can keep keep track of the following statistics:
//
//: o !Queue::EnqueueDelta!: number of events that were enqueued since the last
//:   print
//:
//: o !Queue::DequeueDelta!: number of events that were dequeued since the last
//:   print
//:
//: o !Queue::Size!: current size of the queue (at time of print)
//:
//: o !Queue::Max!: maximum size reached by the queue in the interval between
//:   the previous print and the current print
//:
//: o !Queue::Abs.Max!: maximum size ever reached by the queue
//:
//: o !QueueTime::Min!: minimum time spent in the queue by an event, in the
//:   interval between the previous print and the current print
//:
//: o !QueueTime::Avg!: average time spent in the queue by an event, in the
//:   interval between the previous print and the current print
//:
//: o !QueueTime::Max!: maximum time spent in the queue by an event, in the
//:   interval between the previous print and the current print
//:
//: o !QueueTime::Abs.Max!: maximum time ever spent in the queue by an event
//
/// Thread Safety
///-------------
// Thread safe with minimal locking.
//
/// Usage Example (no internal processing thread)
///---------------------------------------------
// In this example, we are going to create an EventQueue, push item and pop
// them out without using the built-in multithread support.
//..
//  // Create an empty event handler to indicate we will use our own way of
//  // reading messages from the queue
//  bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
//
//
//  // Create the EventQueue
//  bmqimp::EventQueue queue(10,                // initialCapacity
//                           50,                // lowWatermark
//                           100,               // highWatermark
//                           emptyEventHandler,
//                           0,                 // num threads
//                           allocator);
//
//  // Ask the queue for an item from the objectPool, and configure it
//  bsl::shared_ptr<Event> event = queue.createEvent();
//  event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED);
//
//  // We can now push the item to the queue
//  queue.pushBack(event);
//
//  // Let's read the item from the queue
//  bsl::shared_ptr<Event> dequeuedEvent = queue.popFront();
//  BMQTST_ASSERT(*dequeuedEvent == *event);
//..
//
/// Usage Example (using the thread pool for processing events)
///-----------------------------------------------------------
// In this example, we are going to create an EventQueue with 2 threads popping
// out events.
//
// First, let's create the callback that will be called with the events:
//..
//  void eventHandler(const bsl::shared_ptr<Event>& event)
//  {
//    bsl::cout << "Received an event: " << *event << bsl::endl;
//  }
//..
//
// Then we can create the queue and push events to it
//..
//  // Create the EventQueue
//  bmqimp::EventQueue queue(10,             // initialCapacity
//                           50,             // lowWatermark
//                           100,            // highWatermark
//                           &eventHandler,
//                           2,             // num threads
//                           allocator);
//
//  // Ask the queue for an item from the objectPool, and configure it
//  bsl::shared_ptr<Event> event = queue.createEvent();
//  event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED);
//
//  // We can now push the item to the queue
//  queue.pushBack(event);
//
//  // The provided 'eventHandler' will be called with the event that was just
//  // pushed in.
//..
//

// BMQ

#include <bmqimp_event.h>
#include <bmqimp_sessionid.h>

#include <bmqc_monitoredqueue_bdlccsingleproducerqueue.h>
#include <bmqc_multiqueuethreadpool.h>
#include <bmqsys_time.h>

#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_statvalue.h>
#include <bmqst_table.h>

// BDE
#include <bdlcc_sharedobjectpool.h>
#include <bdlcc_singleproducerqueue.h>
#include <bdlmt_fixedthreadpool.h>
#include <bdlt_currenttime.h>
#include <bsl_functional.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_atomic.h>
#include <bsls_cpp11.h>
#include <bsls_spinlock.h>
#include <bsls_systemtime.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqimp {

// ================
// class EventQueue
// ================

/// Thread safe queue of pooled bmqimp::Event items.
class EventQueue {
  public:
    // TYPES

    /// Signature of a callback method to dispatch popped-out messages to.
    typedef bsl::function<void(const bsl::shared_ptr<bmqimp::Event>& event)>
        EventHandlerCallback;

  public:
    // PUBLIC TYPES

    /// Type of the object pool used to efficiently manage the Events.
    typedef bdlcc::SharedObjectPool<Event,
                                    bdlcc::ObjectPoolFunctors::DefaultCreator,
                                    bdlcc::ObjectPoolFunctors::Clear<Event> >
        EventPool;

  private:
    // PRIVATE TYPES

    /// Shortcut alias
    typedef bslma::ManagedPtr<bdlmt::FixedThreadPool> FixedThreadPoolMP;

    /// Struct holding a pointer to the enqueued event and a timestamp
    /// representing the time the event was pushed into the queue.
    struct QueueItem {
        // PUBLIC DATA
        bsl::shared_ptr<Event> d_event_sp;
        // Pointer to the enqueued event

        bsls::Types::Int64 d_enqueueTime;
        // Enqueue time

        // CREATORS
        QueueItem();
        QueueItem(const bsl::shared_ptr<Event>& event,
                  bsls::Types::Int64            enqueueTime);
    };

    typedef bmqc::MonitoredQueue<bdlcc::SingleProducerQueue<QueueItem> >
        MonitoredEventQueue;

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use

    EventPool* d_eventPool_p;
    // Pointer to the ObjectPool of
    // Event (held, not owned)

    MonitoredEventQueue d_queue;
    // The queue

    FixedThreadPoolMP d_threadPool_mp;
    // Thread pool to process items
    // from the queue

    EventHandlerCallback d_eventHandler;
    // Callback for events processing,
    // if any.

    int d_numProcessingThreads;
    // If an eventHandler is used,
    // number of threads to configure
    // the internal thread pool with.

    bsls::AtomicInt d_shouldEmitHighWatermark;
    // 1 means next
    // popFront/timedPopFront should
    // return high watermark.  (needed
    // to prioritize it so that the
    // event makes sense)

    bsls::SpinLock d_lastPoppedOutSpinLock;
    // SpinLock to synchronize update
    // of 'd_lastPoppedOutTime' and
    // 'd_lastInQueueTime', so that
    // they are always both referring
    // to the same item at any point.

    bsls::Types::Int64 d_lastPoppedOutTime;
    // Time of when the last item was
    // popped out from the queue.

    bsls::Types::Int64 d_lastInQueueTime;
    // Time the last popped out item
    // stayed in the queue.

    bslma::ManagedPtr<bmqst::StatContext> d_stats_mp;
    // Stat context to use

    bmqst::Table d_statTable;
    // Table to use for dumping the
    // stats

    bmqst::BasicTableInfoProvider d_statTip;
    // Tip to use when printing stats,
    // include the delta stats fields
    // (diffs since previous print)

    bmqst::BasicTableInfoProvider d_statTipNoDelta;
    // Tip to use when printing stats,
    // excluding the delta stats fields
    // (typically used when printing
    // stats on demand (outside of the
    // stat history size), for example
    // at exit.

    bsls::SpinLock d_pushBackSpinlock;
    // SpinLock to synchronize
    // 'pushBack'

    const SessionId d_sessionId;

  private:
    // NOT IMPLEMENTED
    EventQueue(const EventQueue& other) BSLS_CPP11_DELETED;
    EventQueue& operator=(const EventQueue& rhs) BSLS_CPP11_DELETED;

  private:
    // PRIVATE MANIPULATORS

    /// Queries the object pool for a new event item.
    bsl::shared_ptr<Event> getEvent();

    /// Callback invoked by the MonitoredFixedQueue when it has changed to
    /// the specified `state`.
    void stateCallback(bmqc::MonitoredQueueState::Enum state);

    /// Return true and populate the specified `event` if any prioritized
    /// one was pending; return false and leave `event` untouched if no
    /// prioritized events was scheduled.
    bool hasPriorityEvents(bsl::shared_ptr<Event>* event);

    /// Called after the specified `item` was successfully popped out from
    /// the queue, just before it being delivered to the caller.
    void afterEventPopped(const QueueItem& item);

    /// Print to the specified `stream` a message describing timings of the
    /// latest event that was successfully popped out from the queue.
    void printLastEventTime(bsl::ostream& stream);

    /// Main method of the threads from the thread pool: reads messages from
    /// the queue and call out the provided EventHandler.
    void dispatchNextEvent();

    // PRIVATE ACCESSORS
    const SessionId& id() const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(EventQueue, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `EventQueue` using the specified `initialCapacity`,
    /// `lowWatermark` and `highWatermark`.  If the specified `eventHandler`
    /// is callable, a thread pool having the specified
    /// `numProcessingThreads` will be created and used to dispatch
    /// processing of the events by invoking the `eventHandler`.  Use the
    /// specified `allocator` for any memory allocations.
    EventQueue(EventPool*                  eventPool,
               int                         initialCapacity,
               int                         lowWatermark,
               int                         highWatermark,
               const EventHandlerCallback& eventHandler,
               int                         numProcessingThreads,
               const SessionId&            sessionId,
               bslma::Allocator*           allocator);

    /// Destructor
    ~EventQueue();

    // MANIPULATORS

    /// Configure this component to keep track of statistics: create a
    /// sub-context from the specified `rootStatContext`, using the
    /// specified `start` and `end` snapshot location.
    void initializeStats(bmqst::StatContext* rootStatContext,
                         const bmqst::StatValue::SnapshotLocation& start,
                         const bmqst::StatValue::SnapshotLocation& end);

    /// Start the EventQueue and return 0 on success, or a non zero code on
    /// error.  If an `eventHandler` was provided at construction, this will
    /// start the thread pool.
    int start();

    /// Stop the EventQueue. If an `eventHandler` was provided at
    /// construction, this method will block until all events in the queue
    /// have been processed.
    void stop();

    /// Push the specified `event` to the queue, returning 0 on success or
    /// non-zero on failure to push.
    int pushBack(bsl::shared_ptr<Event>& event);

    /// Return the front item of the queue, if the queue is not empty; or
    /// block and wait until an item is being pushed to the queue.
    bsl::shared_ptr<Event> popFront();

    /// Return the front item of the queue, if the queue is not empty; or
    /// wait for up to the specified `timeout` in respect to the specified
    /// `now` - as a relative offset from between
    /// `bmqsys::Time::nowMonotonicClock()` - argument for an item to be
    /// pushed to the queue.  If no item is found after the provided
    /// `timeout`, the method will return a `SessionEvent` of type
    /// `bmqt::SessionEventType::e_TIMEOUT`.  If an error occurs while
    /// attempting to pop an item from the front of the queue, the method
    /// will return a `SessionEvent` of type
    /// `bmqt::SessionEventType::e_ERROR`.
    bsl::shared_ptr<Event> timedPopFront(
        const bsls::TimeInterval& timeout,
        const bsls::TimeInterval& now = bsls::SystemTime::nowMonotonicClock());

    /// Enqueue a PoisonPill event; this event represents the termination
    /// condition for the thread reading items from the queue.
    void enqueuePoisonPill();

    // ACCESSORS

    /// Return the event pool use by this object.
    EventPool* eventPool() const;

    /// Print the statistics of this `EventQueue` to the specified `stream`.
    /// If the specified `includeDelta` is true, the printed report will
    /// include delta statistics (if any) representing variations since the
    /// last print.  The behavior is undefined unless the statistics were
    /// initialized by a call to `initializeStats`.
    void printStats(bsl::ostream& stream, bool includeDelta) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// class EventQueue
// ----------------

inline EventQueue::EventPool* EventQueue::eventPool() const
{
    return d_eventPool_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
