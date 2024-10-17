// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqc_multiqueuethreadpool.h                                        -*-C++-*-
#ifndef INCLUDED_BMQC_MULTIQUEUETHREADPOOL
#define INCLUDED_BMQC_MULTIQUEUETHREADPOOL

//@PURPOSE: Provide a set of monitored queues processed by a thread pool.
// MultiQueueThreadPool is referred to as MQTP below.
//
//@CLASSES:
//  bmqc::MultiQueueThreadPool: Queues processed by a thread pool
//  bmqc::MultiQueueThreadPoolConfig: Configuration for a MQTP
//  bmqc::MultiQueueThreadPoolUtil: Utilities for MQTP
//  bmqc::MultiQueueThreadPool_QueueCreatorRet: MQTP queueCreator args
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqc::MultiQueueThreadPool', which encapsulates the common pattern of
// creating a number of Queues processed by a dedicated thread, using an
// ObjectPool to create the queue items for performance.  Aside from this
// common use case, the 'bmqc::MultiQueueThreadPool' offers additional options
// for its operation:
// - Creating it with a 'bdlmt::EventScheduler' allows the
//   'bmqc::MultiQueueThreadPool' to enqueue items on the appropriate queue at
//   the requested time.
// - Creating the 'bmqc::MultiQueueThreadPool' without a 'bdlmt::ThreadPool'
//   allows the user to flush the queues manually. This is intended to be used
//   in test drivers to obviate the need for any synchronization logic while
//   keeping the operation of the object under test unchanged.
//
/// Usage
///-----
// Consider using a 'MultiQueueThreadPool' (MQTP) to process a number of
// important integer messages on three(3) queues.  The first thing we should do
// is establish a more convenient typedef for our MQTP:
//..
//  typedef MultiQueueThreadPool< bmqc::MonitoredFixedQueue<int> > MQTP;
//..
// The context for each of our queues will be a 'bsl::vector<int>' that the
// queue will push its very important integer messages into, and that we'll be
// able to example to verify that the messages were processed correctly.
//..
//  bsl::map<int, bsl::vector<int> > queueContextMap;
//..
//
// Now we can start by defining two functions: the queue creator, and the
// event handler.  The queue creator is responsible for constructing one of
// the queues used by the MQTP, as well as a context for the queue, and to
// perform any other necessary initialization for the queue.
//..
//  MQTP::Queue* queueCreator(MQTP::QueueCreatorRet *ret,
//                            int                     queueId,
//                            bslma::Allocator       *allocator)
//  {
//      ret->context().load(&queueContextMap[queueId],
//                          0,
//                          &bslma::ManagedPtrUtil::noOpDeleter);
//
//      return new (*allocator) MQTP::Queue(10, allocator);
//  }
//..
// This function creates our queue, and assigns a 'bsl::vector<int>' as its
// context.  The context is passed to the event handler with every event it
// receives.
//
// Now, our event handler can simply push the integers it receives into its
// context vector.
//..
//  void eventCb(int queueId, void *context, MQTP::Event *event)
//  {
//      if (event->type() == MQTP::Event::BMQC_USER) {
//          bsl::vector<int> *vec = reinterpret_cast<bsl::vector<int> *>(
//                                                                    context);
//          vec->push_back(event->object());
//      }
//  }
//..
// Notice that the handler first checks the type of event.  'BMQC_USER'
// indicates that the event was enqueued by the user, and that
// 'event->object()' is a valid object.  The MQTP can call the callback with
// several other types of events that are beyond the scope of this simple
// example.
//
// Now, we're ready to create a thread pool and the MQTP using it to process
// our messages.
//..
//  bdlmt::ThreadPool threadPool(
//                            bslmt::ThreadAttributes(),        // default
//                            3,                                // minThreads
//                            3,                                // maxThreads
//                            bsl::numeric_limits<int>::max()); // maxIdleTime
//  threadPool.start();
//
//  using namespace bdlf::PlaceHolders;
//  MQTP mfqtp(MQTP::Config(
//                        3,    // number of queues,
//                        bdlf::BindUtil::bind(&eventCb, _1, _2, _3),
//                        bdlf::BindUtil::bind(&queueCreator, _1, _2, _3),
//                        MultiQueueThreadPoolUtil::defaultCreator<int>(),
//                        MultiQueueThreadPoolUtil::noOpResetter<int>())
//                                                     .threadPool(&threadPool)
//                                                     .exclusive(true));
//  mfqtp.start();
//..
// Now we can attempt to enqueue some events on our queues.  Since we created
// three queues, valid queue indices are '0..2'.  For this simple example,
// we'll enqueue the integers '0', '1', and '2' on the corresponding queue, and
// then the integer '3' on all the queues.
//..
//  MQTP::Event event = mfqtp.getUnmanagedEvent();
//  event->object() = 0;
//  mfqtp.enqueueEvent(event, 0);
//
//  event = mfqtp.getUnmanagedEvent();
//  event->object() = 1;
//  mfqtp.enqueueEvent(event, 1);
//
//  event = mfqtp.getUnmanagedEvent();
//  event->object() = 2;
//  mfqtp.enqueueEvent(event, 2);
//
//  event = mfqtp.getUnmanagedEvent();
//  event->object() = 3;
//  mfqtp.enqueueEventOnAllQueues(event);
//..
// Finally, we stop the MQTP, which will block until all queues are empty, and
// verify that each of the queues received the right messages.
//..
//  mfqtp.stop();
//
//  assert(queueContextMap[0].size() == 2);
//  assert(queueContextMap[0][0] == 0);
//  assert(queueContextMap[0][1] == 3);
//
//  assert(queueContextMap[1].size() == 2);
//  assert(queueContextMap[1][0] == 1);
//  assert(queueContextMap[1][1] == 3);
//
//  assert(queueContextMap[2].size() == 2);
//  assert(queueContextMap[2][0] == 2);
//  assert(queueContextMap[2][1] == 3);
//..

#include <bmqc_monitoredqueue_bdlccsingleconsumerqueue.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlcc_objectpool.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_eventscheduler.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bsl_cstddef.h>
#include <bsl_cstdint.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_keyword.h>
#include <bsls_objectbuffer.h>
#include <bsls_performancehint.h>
#include <bsls_spinlock.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqc {

// FORWARD DECLARE
template <typename TYPE>
class MultiQueueThreadPool;
struct MultiQueueThreadPoolUtil;

// ===============================
// class MultiQueueThreadPoolEvent
// ===============================

/// Event provided to the event handling function of a
/// `MultiQueueThreadPoolEvent`
template <typename TYPE>
class MultiQueueThreadPoolEvent {
  public:
    // PUBLIC TYPES
    enum Type {
        BMQC_USER  // Event generated by the user.
        ,
        BMQC_FINALIZE_EVENT  // For events that were enqueued on multiple
                             // queues, this event will be called for the last
                             // queue that finished processing it, right before
                             // the event is returned to the pool.
        ,
        BMQC_QUEUE_EMPTY  // Automatically generated when no more items are
                          // available in a queue.
    };

    /// `CreatorFn` is an alias for a functor creating an object of `TYPE`
    /// in the specified `arena` using the specified `allocator`.
    typedef bsl::function<void(void* arena, bslma::Allocator* allocator)>
        CreatorFn;

    /// `ResetterFn` is an alias for a functor invoked to reset an object of
    /// `TYPE` to a reusable state.
    typedef bsl::function<void(TYPE*)> ResetterFn;

  private:
    // DATA
    bsls::ObjectBuffer<TYPE> d_object;  // The user defined object owned by
                                        // this event

    ResetterFn d_objectResetterFn;

    bsls::AtomicInt d_refCount;

    Type d_type;  // Event type

    bool d_singleThreadedImmediateExecute;
    // When the 'MultiQueueThreadPool' is
    // running in single-threaded mode
    // (without a ThreadPool), should this
    // event be processed immediately or
    // enqueued and processed by the next
    // 'flushQueues' call?

    bool d_enqueuedOnMultipleQueues;
    // True if this event was enqueued on
    // multiple queues

  private:
    // NOT IMPLEMENTED
    MultiQueueThreadPoolEvent(const MultiQueueThreadPoolEvent&)
        BSLS_KEYWORD_DELETED;
    MultiQueueThreadPoolEvent&
    operator=(const MultiQueueThreadPoolEvent&) BSLS_KEYWORD_DELETED;

    // FRIENDS
    template <typename T>
    friend class MultiQueueThreadPool;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MultiQueueThreadPoolEvent,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    MultiQueueThreadPoolEvent(const CreatorFn&  creator,
                              const ResetterFn& resetter,
                              bslma::Allocator* basicAllocator = 0);

    ~MultiQueueThreadPoolEvent();

    // MANIPULATORS

    /// Reset this Event and the owned object
    void reset();

    TYPE& object();

    bool& singleThreadedImmediateExecute();

    // ACCESSORS

    /// Return this event's type.
    Type type() const;

    const TYPE& object() const;
    bool        singleThreadedImmediateExecute() const;
};

// ==========================================
// class MultiQueueThreadPool_QueueCreatorRet
// ==========================================

/// Additional output arguments of a
/// MultiQueueThreadPool::QueueCreatorFn
class MultiQueueThreadPool_QueueCreatorRet {
  private:
    // DATA
    bslma::ManagedPtr<void> d_context_mp;
    // User context associated with the
    // returned queue

    bsl::string d_name;
    // Optional name of this queue

  private:
    // NOT IMPLEMENTED
    MultiQueueThreadPool_QueueCreatorRet(
        const MultiQueueThreadPool_QueueCreatorRet&) BSLS_KEYWORD_DELETED;
    MultiQueueThreadPool_QueueCreatorRet& operator=(
        const MultiQueueThreadPool_QueueCreatorRet&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a new `bmqc::MultiQueueThreadPool_QueueCreatorRet` object
    /// using the optionally specified `basicAllocator`.
    explicit MultiQueueThreadPool_QueueCreatorRet(
        bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Return a refernce offering modifiable access to the user context to
    /// associate with the returned Queue.  This context will be provided to
    /// the event callback whenever an event from this queue is being
    /// executed.
    bslma::ManagedPtr<void>& context();

    // ACCESSORS

    /// Return a reference offering non-modifiable access to the optional
    /// name of this queue.
    const bsl::string& name();
};

// ================================
// class MultiQueueThreadPoolConfig
// ================================

/// Configuration for a MultiQueueThreadPool
template <typename TYPE>
class MultiQueueThreadPoolConfig {
  public:
    // PUBLIC TYPES
    enum EventFinalizationType {
        // Enum controlling for which events 'BMQC_FINALIZE_EVENT' events are
        // generated.

        BMQC_FINALIZE_NONE  // Don't finalize any events
        ,
        BMQC_FINALIZE_MULTI_QUEUE  // Finalize only events that were enqueued
                                   // on multiple queues
        ,
        BMQC_FINALIZE_ALL  // Finalize all events
    };

  private:
    // PRIVATE TYPES
    typedef MultiQueueThreadPoolUtil Util;

    typedef MultiQueueThreadPoolEvent<TYPE> Event;

    /// `CreatorFn` is an alias for a functor creating an object of `TYPE`
    /// in the specified `arena` using the specified `allocator`.
    typedef bsl::function<void(void* arena, bslma::Allocator* allocator)>
        CreatorFn;

    /// `ResetterFn` is an alias for a functor invoked to reset an object of
    /// `TYPE` to a reusable state.
    typedef bsl::function<void(TYPE*)> ResetterFn;

    struct QueueItem {
        // PUBLIC DATA
        bool d_monitorEvent;  // this is a monitor event

        Event* d_event_p;  // Pointer to event
    };

    typedef MonitoredQueue<bdlcc::SingleConsumerQueue<QueueItem> > Queue;

    typedef MultiQueueThreadPool_QueueCreatorRet QueueCreatorRet;

    /// Create the queue for the specified `queueId` using the specified
    /// `allocator`.  Populate the specified `ret` with additional options
    /// associated with the returned queue.
    typedef bsl::function<
        Queue*(QueueCreatorRet* ret, int queueId, bslma::Allocator* allocator)>
        QueueCreatorFn;

    /// Callback invoked when processing the specified `event` popped from
    /// the queue having the specified `queueId` and created with the
    /// specified `queueContext`.
    typedef bsl::function<void(int queueId, void* queueContext, Event* event)>
        EventFn;

    // FRIENDS
    template <typename T>
    friend class MultiQueueThreadPool;

  private:
    // DATA
    const int d_numQueues;  // Number of monitored single consumer
                            // queues

    bdlmt::ThreadPool* d_threadPool_p;
    // Thread pool to use.  If non-null,
    // one thread will be tied per queue.
    // May be set to null to allow the
    // user to flush queues manually and
    // thus obviate the need for
    // synchronization (useful in test
    // drivers)

    bdlmt::EventScheduler* d_eventScheduler_p;
    // Optional Event Scheduler used to
    // enqueue items on the appropriate
    // queue at the requested time.

    EventFn d_eventCallbackFn;

    CreatorFn d_objectCreatorFn;

    ResetterFn d_objectResetterFn;

    QueueCreatorFn d_queueCreatorFn;

    bsl::string d_name;

    EventFinalizationType d_finalizeType;

    bsl::string d_monitorAlarmString;

    bsls::TimeInterval d_monitorAlarmTimeout;

    int d_growBy;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MultiQueueThreadPoolConfig,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `bmqc::MultiQueueThreadPoolConfig` for a
    /// MultiQueueThreadPool with the specified `numQueues` number of queues
    /// and using `numQueues` threads from the specified `threadPool` to
    /// exclusively process events.  Use the specified `eventCallback` to
    /// process events, the specified `queueCreator` to create the queues,
    /// and the optionally specified `objectCreator` and `objectResetter` to
    /// create and reset objects of the parameterized `TYPE`.
    MultiQueueThreadPoolConfig(int                   numQueues,
                               bdlmt::ThreadPool*    threadPool,
                               const EventFn&        eventCallback,
                               const QueueCreatorFn& queueCreator,
                               bslma::Allocator*     basicAllocator = 0);
    MultiQueueThreadPoolConfig(int                   numQueues,
                               bdlmt::ThreadPool*    threadPool,
                               const EventFn&        eventCallback,
                               const QueueCreatorFn& queueCreator,
                               const CreatorFn&      objectCreator,
                               const ResetterFn&     objectResetter,
                               bslma::Allocator*     basicAllocator = 0);

    /// Create a `bmqc::MultiQueueThreadPoolConfig` object having the same
    /// value as the specified `original` object.  Use the optionally
    /// specified `basicAllocator` to supply memory.
    MultiQueueThreadPoolConfig(const MultiQueueThreadPoolConfig& other,
                               bslma::Allocator* basicAllocator = 0);

    // MANIPULATORS

    /// Set the event scheduler to be used by the MQTP to the specified
    /// `eventScheduler`.  The scheduler is only necessary only if events
    /// need to be scheduled to be executed at some future point.  If an
    /// event scheduler wasn't provided for this case, those events will
    /// always be enqueued immediately.  Return a reference offering
    /// modifiable access to this object.
    MultiQueueThreadPoolConfig<TYPE>&
    setEventScheduler(bdlmt::EventScheduler* eventScheduler);

    /// Set the function used to create TYPE objects to the specified
    /// `objectCreator`.  By default, objects will be created by calling
    /// their default constructor.  Return a reference offering modifiable
    /// access to this object.
    MultiQueueThreadPoolConfig<TYPE>&
    setObjectCreator(const CreatorFn& objectCreator);

    /// Set the function used to reset TYPE objects once they've been
    /// processed to the specified `objectResetter`.  By default, objects
    /// will be reset by calling `reset` on them.  Return a reference
    /// offering modifiable access to this object.
    MultiQueueThreadPoolConfig<TYPE>&
    setObjectResetter(const ResetterFn& objectResetter);

    /// Set the name of the MQTP to the specified `name` and return a
    /// reference offering modifiable access to this object.
    MultiQueueThreadPoolConfig<TYPE>& setName(bslstl::StringRef name);

    /// Set for which types of events `BMQC_FINALIZE_EVENT` events should be
    /// generated and return a reference offering modifiable access to this
    /// object.  By default, this is `BMQC_FINALIZE_NONE`.
    MultiQueueThreadPoolConfig<TYPE>&
    setFinalizeEvents(EventFinalizationType type);

    /// Monitor the queues of the MQTP to make sure events can pass through
    /// each queue in at most `timeout` time, and print an error message
    /// with the specified `alarmString` if any of the queues is processing
    /// messages too slowly.  Return a reference offering modifiable access
    /// to this object.
    MultiQueueThreadPoolConfig<TYPE>&
    setMonitorAlarm(bslstl::StringRef         alarmString,
                    const bsls::TimeInterval& timeout);

    /// Set the `growBy` parameter for the ObjectPool to the specified `value`.
    MultiQueueThreadPoolConfig<TYPE>& setGrowBy(int value);
};

// ==========================
// class MultiQueueThreadPool
// ==========================

/// Set of queues of the parameterized QUEUE type processed by a number of
/// threads.
template <typename TYPE>
class MultiQueueThreadPool BSLS_KEYWORD_FINAL {
  public:
    // PUBLIC TYPES
    typedef MultiQueueThreadPool<TYPE>       ThisClass;
    typedef MultiQueueThreadPoolUtil         Util;
    typedef MultiQueueThreadPoolConfig<TYPE> Config;
    typedef MultiQueueThreadPoolEvent<TYPE>  Event;
    typedef typename Config::QueueItem       QueueItem;
    typedef typename Config::Queue           Queue;
    typedef typename Config::CreatorFn       CreatorFn;
    typedef typename Config::ResetterFn      ResetterFn;
    typedef typename Config::QueueCreatorRet QueueCreatorRet;
    typedef typename Config::QueueCreatorFn  QueueCreatorFn;
    typedef typename Config::EventFn         EventFn;

  private:
    // PRIVATE TYPES
    typedef bdlcc::
        ObjectPool<Event, CreatorFn, bdlcc::ObjectPoolFunctors::Reset<Event> >
            EventPool;

    enum MonitorEventState {
        e_MONITOR_PENDING  // an event has been enqueued on the queue but the
                           // next 'processMonitorEvents' hasn't been called
                           // yet

        ,
        e_MONITOR_PROCESSED  // a monitor event has been processed by the queue

        ,
        e_MONITOR_STUCK  // the queue hasn't processed its event in at
                         // least one timeout interval
    };

    struct QueueInfo {
        // PUBLIC DATA
        Queue* d_queue_p;
        // Pointer to the queue

        bsl::shared_ptr<void> d_context_p;
        // Pointer to context passed at
        // time of the queue creation

        bsl::string d_name;
        // Name of the queue

        // TODO: Change this to semaphore to avoid busy-waiting in 'stop'?
        bsls::AtomicBool d_processedZero;

        bsls::AtomicInt d_monitorState;

        bslmt::ThreadUtil::Handle d_exclusiveThreadHandle;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueInfo, bslma::UsesBslmaAllocator)

        // CREATORS
        QueueInfo(bslma::Allocator* basicAllocator = 0)
        : d_queue_p(0)
        , d_context_p()
        , d_name(basicAllocator)
        , d_processedZero(false)
        , d_monitorState(e_MONITOR_PROCESSED)
        , d_exclusiveThreadHandle(bslmt::ThreadUtil::invalidHandle())
        {
            // NOTHING
        }

        QueueInfo(const QueueInfo& other, bslma::Allocator* basicAllocator = 0)
        : d_queue_p(other.d_queue_p)
        , d_context_p(other.d_context_p)
        , d_name(other.d_name, basicAllocator)
        , d_processedZero(other.d_processedZero.load())
        , d_monitorState(static_cast<int>(other.d_monitorState))
        , d_exclusiveThreadHandle(other.d_exclusiveThreadHandle)
        {
            // NOTHING
        }

        // MANIPULATORS
        void reset()
        {
            d_queue_p = 0;
            d_context_p.reset();
            d_monitorState          = e_MONITOR_PROCESSED;
            d_exclusiveThreadHandle = bslmt::ThreadUtil::invalidHandle();
        }
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQC.MULTIQUEUETHREADPOOL");

  private:
    // DATA
    Config d_config;

    EventPool d_pool;

    Event d_queueEmptyEvent;

    bsl::vector<QueueInfo> d_queues;

    bool d_started;

    bdlmt::EventScheduler::RecurringEventHandle d_monitorEventHandle;

    const size_t d_eventOffset;
    // Offset which can be used to retrieve the
    // 'Event' from the userEvent.

    bslma::Allocator* d_allocator_p;

    // PRIVATE CLASS METHODS

    /// Creator function passed to `d_pool` to create an event in the
    /// speificed `arena` using the specified `objectCreator`,
    /// `objectResetter` and `allocator`.
    static void eventCreator(const CreatorFn&  objectCreator,
                             const ResetterFn& objectResetter,
                             void*             arena,
                             bslma::Allocator* allocator);

    // PRIVATE MANIPULATORS

    /// Make sure each queue has processed its `monitor` event and enqueue
    /// another one on each queue.
    void processMonitorEvents();

    /// Unref the specified `event` and return `true` if this was the last
    /// reference to it.  If the specified `release` is true, release the
    /// event back to the pool if `true` is returned.
    bool unrefEvent(Event* event, bool release);

    /// Pop and process events from the queue with the specified `queue`
    /// until a `0` event is popped off.
    void processQueue(int queue);

    /// Enqueue the specified `event` on the specified `queue`, or all
    /// queues if `queue == -1`, if we're not stopped.  If the specified
    /// `tryPush` is `true`, do a `tryPushBack` instead of a `pushBack`.
    int enqueueEventImp(Event* event, int queue, bool tryPush);

  private:
    // NOT IMPLEMENTED
    MultiQueueThreadPool(const MultiQueueThreadPool&) BSLS_KEYWORD_DELETED;
    MultiQueueThreadPool&
    operator=(const MultiQueueThreadPool&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `bmqc::MultiQueueThreadPool` with the specified `config`
    /// and the optionally specified `basicAllocator`.
    MultiQueueThreadPool(const Config&     config,
                         bslma::Allocator* basicAllocator = 0);

    ~MultiQueueThreadPool();

    // MANIPULATORS

    /// Create the queues and start processing events.  Return `0` on
    /// success or a negative value otherwise.  Does nothing and returns
    /// `-2` if this MQTP is already started.
    int start();

    /// Stop enqueuing of new events, wait for all events to be processed,
    /// and destroy all queues.  Fire assert if this MQTP has not been
    /// started.
    void stop();

    /// Set the timeout associated with monitoring of the queues of this
    /// MQTP to make sure events can pass through each queue in at most the
    /// specified `timeout` time.  If `timeout` has 0 seconds and 0
    /// nanoseconds, simply cancel monitoring of the queues of this MQTP
    /// altogether.  Return `0` if this MQTP is started, non-zero
    /// otherwise. The behavior is undefined unless this MQTP was provided
    /// an EventScheduler in its configuration upon construction.
    int setMonitorAlarmTimeout(const bsls::TimeInterval& timeout);

    /// Get an event that can be enqueued with `enqueueEvent`. The event
    /// must be passed to one of the `enqueueEvent` methods, or it should be
    /// released with a call to `releaseUnmanagedEvent`, otherwise it will
    /// be leaked.
    Event* getUnmanagedEvent();

    /// Release the specified `event` which was obtained by a call to
    /// `getUnmanagedEvent`.
    void releaseUnmanagedEvent(Event* event);

    /// Enqueue the specified `event` on the queue with the specified
    /// `queueId` and return `0` if this `MultiQueueThreadPool` is started,
    /// and return `-1` otherwise'.  Note that if the requested queue is
    /// full, this will block.
    int enqueueEvent(Event* event, int queueId);

    /// Enqueue the specified `userEvent` on the queue with the specified
    /// `queueId` and return `0` if this `MultiQueueThreadPool` is started,
    /// and return `-1` otherwise'.  Note that if the requested queue is
    /// full, this will block.  WARNING: The behavior is undefined unless
    /// the specified `userEvent` is a member of an event obtained from a
    /// call to `getUnmanagedEvent`.
    int enqueueEvent(TYPE* userEvent, int queueId);

    /// Enqueue the specified `event` on all queues and return `0` if this
    /// `MultiQueueThreadPool` is started, and return `-1` otherwise'.  Note
    /// that if any queue is full, this will block.
    int enqueueEventOnAllQueues(Event* event);

    /// Enqueue the specified `event` on all queues and return `0` if this
    /// `MultiQueueThreadPool` is started, and return `-1` otherwise'.  Note
    /// that if any queue is full, this will block.  WARNING: The behavior
    /// is undefined unless the specified `userEvent` is a member of an
    /// event obtained from a call to `getUnmanagedEvent`.
    int enqueueEventOnAllQueues(TYPE* userEvent);

    /// Flush the specified `queue` if `queue >= 0` or all queues if 'queue
    /// < 0'.  The behavior is undefined unless this
    /// `bmqc::MultiQueueThreadPool` was created without a
    /// `bdlmt::ThreadPool`, it has started, and `queue < numQueues()`.
    void flushQueue(int queue = -1);

    /// Block until all queues are empty.  Note that calling this only makes
    /// sense after making sure that no other threads can enqueue any new
    /// events on the MQTP.
    void waitUntilEmpty();

    // ACCESSORS

    /// Return `true` if this `MultiQueueThreadPool` has successfully
    /// started.
    bool isStarted() const;

    /// Return the number of queues specified at construction.
    int numQueues() const;

    /// Return the handle to the thread managing the specified `queueId`.
    /// The behavior is undefined unless this object was created in the
    /// exclusive mode.
    bslmt::ThreadUtil::Handle queueThreadHandle(int queueId) const;

    /// Return true if this `MultiQueueThreadPool` is configured to process
    /// queue events in a single (calling) thread and false otherwise.  Note
    /// that configuring this `MultiQueueThreadPool` to run in a single
    /// thread may be useful in test drivers.
    bool isSingleThreaded() const;
};

// ===============================
// struct MultiQueueThreadPoolUtil
// ===============================

/// Utility functions for `MultiQueueThreadPool`
struct MultiQueueThreadPoolUtil {
  private:
    // PRIVATE CLASS METHODS

    /// Reset the specified `object` by calling `reset()` on it.
    template <typename T>
    static void resetResetterImp(T* object);

    /// Do nothing to the specified `object`.
    template <typename T>
    static void noOpResetterImp(T* object);

    /// Create an object of the parameterized type `T` using the specified
    /// `arena` and `allocator`.
    template <typename T>
    static void defaultCreatorImp(void* arena, bslma::Allocator* allocator);

  public:
    // CLASS METHODS

    /// Return a pointer to a function that resets its argument by calling
    /// `reset` on it.
    template <typename T>
    static void (*resetResetter())(T*);

    /// Return a pointer to a function that `resets` its argument by doing
    /// nothing.
    template <typename T>
    static void (*noOpResetter())(T*);

    /// Return a pointer to a function that default constructs its argument
    /// using the provided allocator.
    template <typename T>
    static void (*defaultCreator())(void*, bslma::Allocator*);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class MultiQueueThreadPoolEvent
// -------------------------------

// CREATORS
template <typename TYPE>
inline MultiQueueThreadPoolEvent<TYPE>::MultiQueueThreadPoolEvent(
    const CreatorFn&  creator,
    const ResetterFn& resetter,
    bslma::Allocator* basicAllocator)
: d_object()
, d_objectResetterFn(bsl::allocator_arg, basicAllocator, resetter)
, d_refCount(0)
, d_type(BMQC_USER)
, d_singleThreadedImmediateExecute(false)
, d_enqueuedOnMultipleQueues(false)
{
    creator(d_object.buffer(), basicAllocator);
}

template <typename TYPE>
inline MultiQueueThreadPoolEvent<TYPE>::~MultiQueueThreadPoolEvent()
{
    d_object.object().~TYPE();
}

// MANIPULATORS
template <typename TYPE>
inline void MultiQueueThreadPoolEvent<TYPE>::reset()
{
    d_type = BMQC_USER;

    d_objectResetterFn(&d_object.object());
    d_singleThreadedImmediateExecute = false;
}

template <typename TYPE>
inline TYPE& MultiQueueThreadPoolEvent<TYPE>::object()
{
    return d_object.object();
}

template <typename TYPE>
inline bool& MultiQueueThreadPoolEvent<TYPE>::singleThreadedImmediateExecute()
{
    return d_singleThreadedImmediateExecute;
}

// ACCESSORS
template <typename TYPE>
inline typename MultiQueueThreadPoolEvent<TYPE>::Type
MultiQueueThreadPoolEvent<TYPE>::type() const
{
    return d_type;
}

template <typename TYPE>
inline const TYPE& MultiQueueThreadPoolEvent<TYPE>::object() const
{
    return d_object.object();
}

template <typename TYPE>
inline bool
MultiQueueThreadPoolEvent<TYPE>::singleThreadedImmediateExecute() const
{
    return d_singleThreadedImmediateExecute;
}

// --------------------------------
// class MultiQueueThreadPoolConfig
// --------------------------------

// CREATORS
template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>::MultiQueueThreadPoolConfig(
    int                   numQueues,
    bdlmt::ThreadPool*    threadPool,
    const EventFn&        eventCallback,
    const QueueCreatorFn& queueCreator,
    bslma::Allocator*     basicAllocator)
: d_numQueues(numQueues)
, d_threadPool_p(threadPool)
, d_eventScheduler_p(0)
, d_eventCallbackFn(bsl::allocator_arg, basicAllocator, eventCallback)
, d_objectCreatorFn(bsl::allocator_arg,
                    basicAllocator,
                    Util::defaultCreator<TYPE>())
, d_objectResetterFn(bsl::allocator_arg,
                     basicAllocator,
                     Util::resetResetter<TYPE>())
, d_queueCreatorFn(bsl::allocator_arg, basicAllocator, queueCreator)
, d_name(basicAllocator)
, d_finalizeType(BMQC_FINALIZE_NONE)
, d_monitorAlarmString(basicAllocator)
, d_monitorAlarmTimeout()
, d_growBy(-1)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!threadPool || threadPool->enabled());
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>::MultiQueueThreadPoolConfig(
    int                   numQueues,
    bdlmt::ThreadPool*    threadPool,
    const EventFn&        eventCallback,
    const QueueCreatorFn& queueCreator,
    const CreatorFn&      objectCreator,
    const ResetterFn&     objectResetter,
    bslma::Allocator*     basicAllocator)
: d_numQueues(numQueues)
, d_threadPool_p(threadPool)
, d_eventScheduler_p(0)
, d_eventCallbackFn(bsl::allocator_arg, basicAllocator, eventCallback)
, d_objectCreatorFn(bsl::allocator_arg, basicAllocator, objectCreator)
, d_objectResetterFn(bsl::allocator_arg, basicAllocator, objectResetter)
, d_queueCreatorFn(bsl::allocator_arg, basicAllocator, queueCreator)
, d_name(basicAllocator)
, d_finalizeType(BMQC_FINALIZE_NONE)
, d_monitorAlarmString(basicAllocator)
, d_monitorAlarmTimeout()
, d_growBy(-1)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!threadPool || threadPool->enabled());
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>::MultiQueueThreadPoolConfig(
    const MultiQueueThreadPoolConfig& other,
    bslma::Allocator*                 basicAllocator)
: d_numQueues(other.d_numQueues)
, d_threadPool_p(other.d_threadPool_p)
, d_eventScheduler_p(other.d_eventScheduler_p)
, d_eventCallbackFn(bsl::allocator_arg,
                    basicAllocator,
                    other.d_eventCallbackFn)
, d_objectCreatorFn(bsl::allocator_arg,
                    basicAllocator,
                    other.d_objectCreatorFn)
, d_objectResetterFn(bsl::allocator_arg,
                     basicAllocator,
                     other.d_objectResetterFn)
, d_queueCreatorFn(bsl::allocator_arg, basicAllocator, other.d_queueCreatorFn)
, d_name(other.d_name, basicAllocator)
, d_finalizeType(other.d_finalizeType)
, d_monitorAlarmString(other.d_monitorAlarmString, basicAllocator)
, d_monitorAlarmTimeout(other.d_monitorAlarmTimeout)
, d_growBy(-1)
{
    // NOTHING
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setEventScheduler(
    bdlmt::EventScheduler* eventScheduler)
{
    d_eventScheduler_p = eventScheduler;
    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setObjectCreator(
    const CreatorFn& objectCreator)
{
    d_objectCreatorFn = objectCreator;
    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setObjectResetter(
    const ResetterFn& objectResetter)
{
    d_objectResetterFn = objectResetter;
    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setName(bslstl::StringRef name)
{
    d_name = name;
    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setFinalizeEvents(EventFinalizationType type)
{
    d_finalizeType = type;
    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setMonitorAlarm(
    bslstl::StringRef         alarmString,
    const bsls::TimeInterval& timeout)
{
    BSLS_ASSERT_SAFE(d_eventScheduler_p &&
                     "Cannot monitor queues if an event scheduler was not "
                     "provided");

    d_monitorAlarmString  = alarmString;
    d_monitorAlarmTimeout = timeout;

    return *this;
}

template <typename TYPE>
inline MultiQueueThreadPoolConfig<TYPE>&
MultiQueueThreadPoolConfig<TYPE>::setGrowBy(int value)
{
    d_growBy = value;

    return *this;
}

// --------------------------
// class MultiQueueThreadPool
// --------------------------

// PRIVATE CLASS METHODS
template <typename TYPE>
inline void
MultiQueueThreadPool<TYPE>::eventCreator(const CreatorFn&  objectCreator,
                                         const ResetterFn& objectResetter,
                                         void*             arena,
                                         bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(arena);
    BSLS_ASSERT_SAFE(allocator);

    bslalg::ScalarPrimitives::construct(reinterpret_cast<Event*>(arena),
                                        objectCreator,
                                        objectResetter,
                                        allocator);
}

// PRIVATE MANIPULATORS
template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::processMonitorEvents()
{
    // Check each queue to make sure it has processed the last monitor event
    // (this will always succeed on the first iteration because of the way
    // QueueInfo is initialized)
    for (size_t i = 0; i < d_queues.size(); ++i) {
        const MonitorEventState prevState = static_cast<MonitorEventState>(
            d_queues[i].d_monitorState.testAndSwap(e_MONITOR_PROCESSED,
                                                   e_MONITOR_PENDING));
        if (prevState == e_MONITOR_PENDING) {
            // The queue hasn't processed its event yet
            BALL_LOG_ERROR_BLOCK
            {
                BALL_LOG_OUTPUT_STREAM
                    << d_config.d_monitorAlarmString << " Queue '"
                    << d_queues[i].d_name
                    << "' hasn't processed an event enqueued "
                    << bmqu::PrintUtil::prettyTimeInterval(
                           d_config.d_monitorAlarmTimeout.totalMicroseconds() *
                           bdlt::TimeUnitRatio::k_NS_PER_US)
                    << " ago.";
            }

            d_queues[i].d_monitorState.testAndSwap(e_MONITOR_PENDING,
                                                   e_MONITOR_STUCK);
        }
        else if (prevState == e_MONITOR_PROCESSED) {
            // Enqueue next event
            QueueItem newItem = {true  // isMonitorEvent
                                 ,
                                 0};  // Pointer to event
            const int ret     = d_queues[i].d_queue_p->tryPushBack(newItem);
            if (ret != 0) {
                BALL_LOG_ERROR << d_config.d_monitorAlarmString
                               << " Couldn't enqueue monitor event on queue '"
                               << d_queues[i].d_name << "'.  Ret: " << ret;

                // Ensure that we try to enqueue again on next pass
                d_queues[i].d_monitorState = e_MONITOR_PROCESSED;
            }
        }

        // If the event was e_MONITOR_STUCK, we don't do anything until it's
        // processed
    }
}

template <typename TYPE>
inline bool MultiQueueThreadPool<TYPE>::unrefEvent(Event* event, bool release)
{
    const bool isLastRef = (--event->d_refCount == 0);
    if (isLastRef && release) {
        d_pool.releaseObject(event);
    }

    return isLastRef;
}

template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::processQueue(int queue)
{
    QueueInfo& info = d_queues[queue];
    QueueItem  item;

    // Store the thread id of the thread being exclusively used
    info.d_exclusiveThreadHandle = bslmt::ThreadUtil::self();

    while (true) {
        const int popRet = info.d_queue_p->tryPopFront(&item);
        if (popRet != 0) {
            // Queue is empty
            d_config.d_eventCallbackFn(queue,
                                       info.d_context_p.get(),
                                       &d_queueEmptyEvent);

            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(isSingleThreaded())) {
                // In single-threaded mode; return here to avoid blocking on
                // the only thread.
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                return;  // RETURN
            }

            info.d_queue_p->popFront(&item);
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(item.d_monitorEvent)) {
            // Process monitor event
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            const MonitorEventState prevState = static_cast<MonitorEventState>(
                info.d_monitorState.swap(e_MONITOR_PROCESSED));
            if (prevState == e_MONITOR_STUCK) {
                // The queue was stuck, but is now back to normal
                BALL_LOG_INFO << "Queue '" << info.d_name << "' is back to "
                              << "work";
            }
            continue;  // CONTINUE
        }

        Event* event = item.d_event_p;
        if (event == 0) {
            // We've been told to return
            info.d_processedZero = true;
            return;  // RETURN
        }

        d_config.d_eventCallbackFn(queue, info.d_context_p.get(), event);

        if (unrefEvent(event, false)) {
            bool finalize = (d_config.d_finalizeType ==
                             Config::BMQC_FINALIZE_ALL);
            finalize      = finalize || ((d_config.d_finalizeType ==
                                     Config::BMQC_FINALIZE_MULTI_QUEUE) &&
                                    event->d_enqueuedOnMultipleQueues);

            if (finalize) {
                event->d_type = Event::BMQC_FINALIZE_EVENT;
                d_config.d_eventCallbackFn(queue,
                                           info.d_context_p.get(),
                                           event);
            }
            d_pool.releaseObject(event);
        }
    }
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEventImp(Event* event,
                                                       int    queue,
                                                       bool   tryPush)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event);
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_TRY_PUSH_BACK_ERROR = -1  // An error was encountered while trying
                                     // to push back (queue is full or
                                     // disabled)
        ,
        rc_PUSH_BACK_ERROR = -2  // An error was encountered while pushing
                                 // back (queue is disabled)
    };

    // Determine start and end queues for which to enqueue
    int startQueue = queue;
    int endQueue   = queue + 1;
    if (queue == -1) {
        startQueue = 0;
        endQueue   = static_cast<int>(d_queues.size());
        event->d_refCount.storeRelaxed(static_cast<int>(d_queues.size()));
        event->d_enqueuedOnMultipleQueues = true;
    }
    else {
        event->d_refCount.storeRelaxed(1);
        event->d_enqueuedOnMultipleQueues = false;
    }

    // Enqueue on each selected queue
    int ret = rc_SUCCESS;
    for (int queueIdx = startQueue; queueIdx < endQueue; ++queueIdx) {
        QueueInfo& info    = d_queues[queueIdx];
        QueueItem  newItem = {false  // isMonitorEvent
                              ,
                              event};  // pointer to event

        // [try to] Push back item
        int pushRet = 0;
        if (tryPush) {
            pushRet = info.d_queue_p->tryPushBack(newItem);
        }
        else {
            pushRet = info.d_queue_p->pushBack(newItem);
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(pushRet != 0)) {
            // [try to] Push back failed
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            ret = 10 * pushRet +
                  (tryPush ? rc_TRY_PUSH_BACK_ERROR : rc_PUSH_BACK_ERROR);
            unrefEvent(event, true);
        }
        else {
            if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                    isSingleThreaded() &&
                    event->singleThreadedImmediateExecute())) {
                BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
                processQueue(queueIdx);
            }
        }
    }

    return ret;
}

// CREATORS
template <typename TYPE>
inline MultiQueueThreadPool<TYPE>::MultiQueueThreadPool(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_pool(bdlf::BindUtil::bind(&MultiQueueThreadPool<TYPE>::eventCreator,
                              config.d_objectCreatorFn,
                              config.d_objectResetterFn,
                              bdlf::PlaceHolders::_1,
                              // arena
                              bdlf::PlaceHolders::_2),  // allocator
         config.d_growBy,
         basicAllocator)
, d_queueEmptyEvent(config.d_objectCreatorFn,
                    config.d_objectResetterFn,
                    basicAllocator)
, d_queues(config.d_numQueues, QueueInfo(), basicAllocator)
, d_started(false)
, d_monitorEventHandle()
, d_eventOffset(reinterpret_cast<uintptr_t>(&d_queueEmptyEvent.d_object) -
                reinterpret_cast<uintptr_t>(&d_queueEmptyEvent))
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    d_queueEmptyEvent.d_type = Event::BMQC_QUEUE_EMPTY;
}

template <typename TYPE>
inline MultiQueueThreadPool<TYPE>::~MultiQueueThreadPool()
{
    BSLS_ASSERT_SAFE(!isStarted() && "'stop' must be explicitly called");
    // 'stop' must be explicitly called
}

// MANIPULATORS
template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::start()
{
    if (isStarted()) {
        // MQTP has already been started
        return -2;  // RETURN
    }

    // Create the queues
    for (size_t i = 0; i < d_queues.size(); ++i) {
        QueueCreatorRet ret;
        d_queues[i].d_queue_p   = d_config.d_queueCreatorFn(&ret,
                                                          static_cast<int>(i),
                                                          d_allocator_p);
        d_queues[i].d_context_p = ret.context();
        if (ret.name().empty()) {
            // Generate a name for the queue
            bsl::ostringstream ss;
            if (!d_config.d_name.empty()) {
                ss << d_config.d_name << ' ';
            }
            ss << "Queue " << i;

            d_queues[i].d_name = ss.str();
        }
        else {
            d_queues[i].d_name = ret.name();
        }

        d_queues[i].d_processedZero = false;
    }

    // Set up threads
    if (!isSingleThreaded()) {
        // Thread pool was provided -> execution is not limited to a single
        // thread
        int numAvailableThreads = d_config.d_threadPool_p->maxThreads() -
                                  d_config.d_threadPool_p->numActiveThreads();
        if (numAvailableThreads < static_cast<int>(d_queues.size())) {
            // Not enough threads for exclusive use
            return -1;  // RETURN
        }

        BSLS_ASSERT_SAFE(d_config.d_threadPool_p->enabled());
        for (size_t i = 0; i < d_queues.size(); ++i) {
            int rc = d_config.d_threadPool_p->enqueueJob(
                bdlf::BindUtil::bind(&ThisClass::processQueue,
                                     this,
                                     static_cast<int>(i)));
            BSLS_ASSERT_SAFE(rc == 0);
            (void)rc;  // Compiler happiness
        }
    }

    // See if we have to start monitoring job
    if (d_config.d_eventScheduler_p &&
        d_config.d_monitorAlarmTimeout != bsls::TimeInterval()) {
        d_config.d_eventScheduler_p->scheduleRecurringEvent(
            &d_monitorEventHandle,
            d_config.d_monitorAlarmTimeout,
            bdlf::BindUtil::bind(&ThisClass::processMonitorEvents, this));
    }

    d_started = true;

    return 0;
}

template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::stop()
{
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    d_started = false;

    if (d_config.d_eventScheduler_p && d_monitorEventHandle) {
        d_config.d_eventScheduler_p->cancelEventAndWait(&d_monitorEventHandle);
        d_monitorEventHandle.release();
    }

    // If we create the queues in the constructor, and delete them in the
    // destructor, then we should be able to just disable the queues in the
    // loop below and let the Queues handle failing the enqueue if we're
    // stopped/stopping.
    for (size_t i = 0; i < d_queues.size(); ++i) {
        QueueInfo& info = d_queues[i];

        QueueItem item = {false  // isMonitorEvent
                          ,
                          0};  // pointer to event
        info.d_queue_p->pushBack(item);
        info.d_queue_p->disablePushBack();

        if (isSingleThreaded()) {
            processQueue(static_cast<int>(i));
        }

        // TODO: Change 'd_processedZero' to semaphore instead to avoid busy
        //       waiting?
        while (!info.d_processedZero) {
            bslmt::ThreadUtil::yield();
        }

        while (!info.d_queue_p->tryPopFront(&item)) {
            d_pool.releaseObject(item.d_event_p);
        }

        d_allocator_p->deleteObject(info.d_queue_p);
        info.reset();
    }
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::setMonitorAlarmTimeout(
    const bsls::TimeInterval& timeout)
{
    BSLS_ASSERT_SAFE(d_config.d_eventScheduler_p &&
                     "Cannot set monitor alarm timeout if an event "
                     "scheduler was not provided");

    if (d_monitorEventHandle) {
        d_config.d_eventScheduler_p->cancelEventAndWait(&d_monitorEventHandle);
        d_monitorEventHandle.release();
    }

    d_config.d_monitorAlarmTimeout = timeout;

    if (timeout == bsls::TimeInterval()) {
        // Disable monitoring of the queues of this MQTP altogether
        return 0;  // RETURN
    }

    if (isStarted()) {
        d_config.d_eventScheduler_p->scheduleRecurringEvent(
            &d_monitorEventHandle,
            d_config.d_monitorAlarmTimeout,
            bdlf::BindUtil::bind(&ThisClass::processMonitorEvents, this));
    }

    return 0;
}

template <typename TYPE>
inline MultiQueueThreadPoolEvent<TYPE>*
MultiQueueThreadPool<TYPE>::getUnmanagedEvent()
{
    return d_pool.getObject();
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEvent(Event* event, int queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(event);
    BSLS_ASSERT(0 <= queueId && queueId < numQueues());
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    const int ret = enqueueEventImp(event, queueId, false);

    return ret;
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEvent(TYPE* userEvent,
                                                    int   queueId)
{
    // The userEvent was 'extracted' from an event, retrieve the event back by
    // pointing to the correct address in memory.
    Event* event = reinterpret_cast<Event*>(
        (reinterpret_cast<char*>(userEvent) - d_eventOffset));

    return enqueueEvent(event, queueId);
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEventOnAllQueues(Event* event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    const int ret = enqueueEventImp(event, -1, false);

    return ret;
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEventOnAllQueues(TYPE* userEvent)

{
    // The userEvent was 'extracted' from an event, retrieve the event back by
    // pointing to the correct address in memory.
    Event* event = reinterpret_cast<Event*>(
        (reinterpret_cast<char*>(userEvent) - d_eventOffset));

    return enqueueEventOnAllQueues(event);
}

template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::flushQueue(int queue)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(isSingleThreaded());
    BSLS_ASSERT_OPT(isStarted() && "MQTP has not been started");
    BSLS_ASSERT_OPT(queue < numQueues());

    if (queue < 0) {
        // Flush all queues
        for (size_t i = 0; i < d_queues.size(); ++i) {
            processQueue(static_cast<int>(i));
        }
    }
    else {
        processQueue(queue);
    }
}

template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::waitUntilEmpty()
{
    bool fullPass = false;
    while (!fullPass) {
        fullPass = true;
        for (size_t i = 0; i < d_queues.size(); ++i) {
            while (!d_queues[i].d_queue_p->isEmpty()) {
                bslmt::ThreadUtil::yield();
                fullPass = false;
            }
        }

        if (d_pool.numObjects() != d_pool.numAvailableObjects()) {
            bslmt::ThreadUtil::yield();
            fullPass = false;
        }
    }
}

// ACCESSORS
template <typename TYPE>
inline bool MultiQueueThreadPool<TYPE>::isStarted() const
{
    return d_started;
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::numQueues() const
{
    return static_cast<int>(d_queues.size());
}

template <typename TYPE>
inline bslmt::ThreadUtil::Handle
MultiQueueThreadPool<TYPE>::queueThreadHandle(int queueId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= queueId);
    BSLS_ASSERT_SAFE(queueId < numQueues());

    return d_queues[queueId].d_exclusiveThreadHandle;
}

template <typename TYPE>
inline bool MultiQueueThreadPool<TYPE>::isSingleThreaded() const
{
    return (d_config.d_threadPool_p == 0);
}

// -------------------------------
// struct MultiQueueThreadPoolUtil
// -------------------------------

// PRIVATE CLASS METHODS
template <typename T>
inline void MultiQueueThreadPoolUtil::resetResetterImp(T* object)
{
    object->reset();
}

template <typename T>
inline void
MultiQueueThreadPoolUtil::noOpResetterImp(BSLS_ANNOTATION_UNUSED T* object)
{
    // NOTHING
}

template <typename T>
inline void
MultiQueueThreadPoolUtil::defaultCreatorImp(void*             arena,
                                            bslma::Allocator* allocator)
{
    bslalg::ScalarPrimitives::defaultConstruct(reinterpret_cast<T*>(arena),
                                               allocator);
}

// CLASS METHODS
template <typename T>
inline void (*MultiQueueThreadPoolUtil::resetResetter())(T*)
{
    return &resetResetterImp<T>;
}

template <typename T>
inline void (*MultiQueueThreadPoolUtil::noOpResetter())(T*)
{
    return &noOpResetterImp<T>;
}

template <typename T>
inline void (*MultiQueueThreadPoolUtil::defaultCreator())(void*,
                                                          bslma::Allocator*)
{
    return &defaultCreatorImp<T>;
}

}  // close package namespace
}  // close enterprise namespace

#endif
