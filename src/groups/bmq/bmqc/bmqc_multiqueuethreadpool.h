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
//  bmqc::MultiQueueThreadPool_QueueCreatorRet: MQTP queueCreator args
//
//@DESCRIPTION: This component defines a mechanism,
// 'bmqc::MultiQueueThreadPool', which encapsulates the common pattern of
// creating a number of Queues processed by a dedicated thread, using a
// SharedObjectPool to create the queue items for performance.
// Aside from this common use case, the 'bmqc::MultiQueueThreadPool'
// offers additional options for its operation:
// - Creating it with a 'bdlmt::EventScheduler' allows the
//   'bmqc::MultiQueueThreadPool' to enqueue items on the appropriate queue at
//   the requested time.
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
//  void eventCb(int queueId, void *context, const MQTP::EventSp &event)
//  {
//      if (event) {
//          // Non-empty `event` means user event
//          bsl::vector<int> *vec = reinterpret_cast<bsl::vector<int> *>(
//                                                                    context);
//          vec->push_back(event->object());
//      } else {
//          // Empty `event` means empty queue event
//      }
//  }
//..
// Notice that an empty `event` shared pointer passed to the callback is a
// a valid case and it means "queue is empty" event.  Non-empty `event` always
// means a user event.
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
//                        bdlf::BindUtil::bind(&queueCreator, _1, _2, _3))
//                                                     .threadPool(&threadPool)
//                                                     .exclusive(true));
//  mfqtp.start();
//..
// Now we can attempt to enqueue some events on our queues.  Since we created
// three queues, valid queue indices are '0..2'.  For this simple example,
// we'll enqueue the integers '0', '1', and '2' on the corresponding queue, and
// then the integer '3' on all the queues.
//..
//  MQTP::EventSp event = mfqtp.getEvent();
//  event->value() = 0;
//  mfqtp.enqueueEvent(bslmf::MovableRefUtil::move(event), 0);
//
//  event = mfqtp.getEvent();
//  event->value() = 1;
//  mfqtp.enqueueEvent(bslmf::MovableRefUtil::move(event), 1);
//
//  event = mfqtp.getEvent();
//  event->value() = 2;
//  mfqtp.enqueueEvent(bslmf::MovableRefUtil::move(event), 2);
//
//  event = mfqtp.getEvent();
//  event->value() = 3;
//  mfqtp.enqueueEventOnAllQueues(bslmf::MovableRefUtil::move(event));
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
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_default.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_allocatorargt.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bslmt_timedsemaphore.h>
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
  private:
    // PRIVATE TYPES
    typedef TYPE                   Event;
    typedef bsl::shared_ptr<Event> EventSp;

    /// `CreatorFn` is an alias for a functor creating an object of `TYPE`
    /// in the specified `arena` using the specified `allocator`.
    typedef bsl::function<void(void* arena, bslma::Allocator* allocator)>
        CreatorFn;

    typedef MonitoredQueue<bdlcc::SingleConsumerQueue<EventSp> > Queue;

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
    typedef bsl::function<
        void(int queueId, void* queueContext, const EventSp& event)>
        EventFn;

    // FRIENDS
    template <typename T>
    friend class MultiQueueThreadPool;

  private:
    // DATA
    /// Number of monitored single consumer queues
    const int d_numQueues;

    /// Thread pool to use (held, not owned)
    bdlmt::ThreadPool* d_threadPool_p;

    /// Optional Event Scheduler used to enqueue items on the appropriate
    /// queue at the requested time.
    bdlmt::EventScheduler* d_eventScheduler_p;

    EventFn d_eventCallbackFn;

    QueueCreatorFn d_queueCreatorFn;

    bsl::string d_name;

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
    explicit MultiQueueThreadPoolConfig(int                   numQueues,
                                        bdlmt::ThreadPool*    threadPool,
                                        const EventFn&        eventCallback,
                                        const QueueCreatorFn& queueCreator,
                                        bslma::Allocator* basicAllocator = 0);

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

    /// Set the name of the MQTP to the specified `name` and return a
    /// reference offering modifiable access to this object.
    MultiQueueThreadPoolConfig<TYPE>& setName(bslstl::StringRef name);

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
    typedef MultiQueueThreadPoolConfig<TYPE> Config;
    typedef TYPE                             Event;
    typedef bsl::shared_ptr<Event>           EventSp;
    typedef typename Config::Queue           Queue;
    typedef typename Config::CreatorFn       CreatorFn;
    typedef typename Config::QueueCreatorRet QueueCreatorRet;
    typedef typename Config::QueueCreatorFn  QueueCreatorFn;
    typedef typename Config::EventFn         EventFn;

  private:
    // PRIVATE TYPES
    typedef bdlcc::SharedObjectPool<Event,
                                    CreatorFn,
                                    bdlcc::ObjectPoolFunctors::Reset<Event> >
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
        /// Pointer to the queue
        Queue* d_queue_p;

        /// Pointer to context passed at time of the queue creation
        bsl::shared_ptr<void> d_context_p;

        /// Name of the queue
        bsl::string d_name;

        bsls::AtomicInt d_monitorState;

        /// A thread-safe reference counter for the queue processing loop.
        /// The loop will continue processing as long as this counter is
        /// greater than 0.  The rules for updating it are the following:
        /// (1) Always start with ref count = 1.
        /// (2) Increment when monitor event is enqueued to the queue.
        /// (3) Decrement when monitor event is processed by the queue.
        /// (4) Decrement when `stop()` is called, this gets rid of the
        ///     initial reference 1 from case (1).
        /// These rules allow to keep the queue processing loop working
        /// as long as there are any monitor events awaiting in the queue OR
        /// as long as `stop()` was not called.
        bsls::AtomicInt d_processQueueRefCount;

        /// A semaphore used to verify that a queue has stopped.
        /// Note: shared_ptr is used because copy/move are not defined for
        ///       `TimedSemaphore` and we still need to copy `QueueInfo`.
        bsl::shared_ptr<bslmt::TimedSemaphore> d_finished_sp;

        bslmt::ThreadUtil::Id d_threadId;

        // TRAITS
        BSLMF_NESTED_TRAIT_DECLARATION(QueueInfo, bslma::UsesBslmaAllocator)

        // CREATORS
        explicit QueueInfo(bslma::Allocator* basicAllocator = 0)
        : d_queue_p(0)
        , d_context_p()
        , d_name(basicAllocator)
        , d_monitorState(e_MONITOR_PROCESSED)
        , d_processQueueRefCount(1)
        , d_finished_sp(bsl::allocate_shared<bslmt::TimedSemaphore>(
              basicAllocator,
              0,
              bsls::SystemClockType::e_MONOTONIC))
        , d_threadId(0)
        {
            // NOTHING
        }

        explicit QueueInfo(const QueueInfo&  other,
                           bslma::Allocator* basicAllocator = 0)
        : d_queue_p(other.d_queue_p)
        , d_context_p(other.d_context_p)
        , d_name(other.d_name, basicAllocator)
        , d_monitorState(static_cast<int>(other.d_monitorState))
        , d_processQueueRefCount(
              static_cast<int>(other.d_processQueueRefCount))
        , d_finished_sp(other.d_finished_sp)
        , d_threadId(other.d_threadId)
        {
            // NOTHING
        }

        // MANIPULATORS
        void reset()
        {
            d_queue_p = 0;
            d_context_p.reset();
            d_monitorState          = e_MONITOR_PROCESSED;
            d_threadId              = 0;
        }
    };

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQC.MULTIQUEUETHREADPOOL");

    // CLASS DATA
    /// The timeout for `timedWait` when stopping the queues
    static const int k_MAX_WAIT_SECONDS_AT_SHUTDOWN = 300;

    // DATA
    Config d_config;

    EventPool d_pool;

    EventSp d_queueEmptyEvent_sp;

    bsl::vector<QueueInfo> d_queues;

    bool d_started;

    bdlmt::EventScheduler::RecurringEventHandle d_monitorEventHandle;

    bslma::Allocator* d_allocator_p;

    // PRIVATE CLASS METHODS

    /// Creator function passed to `d_pool` to create an event in the
    /// specified `arena` using the specified `allocator`.
    static void eventCreator(void* arena, bslma::Allocator* allocator);

    // PRIVATE MANIPULATORS

    /// Make sure each queue has processed its `monitor` event and enqueue
    /// another one on each queue.
    void processMonitorEvents();

    /// Thread pool worker function.
    /// Pop and process events from the queue with the specified `queue`
    /// until a `0` event is popped off.
    void processQueue(int queue);

  private:
    // NOT IMPLEMENTED
    MultiQueueThreadPool(const MultiQueueThreadPool&) BSLS_KEYWORD_DELETED;
    MultiQueueThreadPool&
    operator=(const MultiQueueThreadPool&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MultiQueueThreadPool,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `bmqc::MultiQueueThreadPool` with the specified `config`
    /// and the optionally specified `basicAllocator`.
    explicit MultiQueueThreadPool(const Config&     config,
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

    /// Get an event that can be enqueued with `enqueueEvent`.
    EventSp getEvent();

    /// @brief Enqueue an event to the specified queue.
    /// @param event Event to enqueue.
    /// @param queueId Queue id of the destination queue for the event.
    /// @return 0 on success, non-zero on failure.
    /// NOTE: if the requested queue is full, this will block.
    int enqueueEvent(bslmf::MovableRef<EventSp> event, int queueId);

    /// @brief Enqueue an event to all queues.
    /// @param event Event to enqueue.
    /// @return 0 on success, non-zero on failure.
    /// NOTE: if the requested queue is full, this will block.
    int enqueueEventOnAllQueues(bslmf::MovableRef<EventSp> event);

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
    bslmt::ThreadUtil::Id queueThreadId(int queueId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

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
, d_queueCreatorFn(bsl::allocator_arg, basicAllocator, queueCreator)
, d_name(basicAllocator)
, d_monitorAlarmString(basicAllocator)
, d_monitorAlarmTimeout()
, d_growBy(-1)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(threadPool);
    BSLS_ASSERT_SAFE(threadPool->enabled());
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
, d_queueCreatorFn(bsl::allocator_arg, basicAllocator, other.d_queueCreatorFn)
, d_name(other.d_name, basicAllocator)
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
MultiQueueThreadPoolConfig<TYPE>::setName(bslstl::StringRef name)
{
    d_name = name;
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
MultiQueueThreadPool<TYPE>::eventCreator(void*             arena,
                                         bslma::Allocator* allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(arena);
    BSLS_ASSERT_SAFE(allocator);

    bslalg::ScalarPrimitives::construct(reinterpret_cast<Event*>(arena),
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
            // Enqueue next monitor event

            d_queues[i].d_processQueueRefCount.addRelaxed(1);
            const int ret = d_queues[i].d_queue_p->tryPushBack(
                NULL /* monitor event */);
            if (ret != 0) {
                BALL_LOG_ERROR << d_config.d_monitorAlarmString
                               << " Couldn't enqueue monitor event on queue '"
                               << d_queues[i].d_name << "'.  Ret: " << ret;

                // Ensure that we try to enqueue again on next pass
                d_queues[i].d_monitorState = e_MONITOR_PROCESSED;

                // Event push failed, update the ref count:
                d_queues[i].d_processQueueRefCount.subtractRelaxed(1);
            }
        }

        // If the event was e_MONITOR_STUCK, we don't do anything until it's
        // processed
    }
}

template <typename TYPE>
inline void MultiQueueThreadPool<TYPE>::processQueue(int queue)
{
    QueueInfo& info = d_queues[queue];

    // Store the thread id of the thread being exclusively used
    info.d_threadId = bslmt::ThreadUtil::selfId();

    while (true) {
        EventSp   event;
        const int popRet = info.d_queue_p->tryPopFront(&event);
        if (popRet != 0) {
            // Queue is empty
            d_config.d_eventCallbackFn(queue,
                                       info.d_context_p.get(),
                                       d_queueEmptyEvent_sp);

            info.d_queue_p->popFront(&event);
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 == event)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // Process monitor event
            if (0 == info.d_processQueueRefCount.subtractRelaxed(1)) {
                // 0 ref count means that:
                // - `stop()` was called: it released the initial reference.
                // - It is the last monitor event enqueued to the queue.
                // No need to process this event, it is time to return.
                // Note: it is possible that another monitor event will be
                //       enqueued right after the check is done, but it's okay.
                //       We will skip it with any remainder events on `stop()`.
                info.d_finished_sp->post();
                return;  // RETURN
            }

            const MonitorEventState prevState = static_cast<MonitorEventState>(
                info.d_monitorState.swap(e_MONITOR_PROCESSED));
            if (prevState == e_MONITOR_STUCK) {
                // The queue was stuck, but is now back to normal
                BALL_LOG_INFO << "Queue '" << info.d_name << "' is back to "
                              << "work";
            }
            continue;  // CONTINUE
        }

        d_config.d_eventCallbackFn(queue, info.d_context_p.get(), event);
    }
}

// CREATORS
template <typename TYPE>
inline MultiQueueThreadPool<TYPE>::MultiQueueThreadPool(
    const Config&     config,
    bslma::Allocator* basicAllocator)
: d_config(config, basicAllocator)
, d_pool(bdlf::BindUtil::bind(&MultiQueueThreadPool<TYPE>::eventCreator,
                              bdlf::PlaceHolders::_1,   // arena
                              bdlf::PlaceHolders::_2),  // allocator
         config.d_growBy,
         basicAllocator)
, d_queueEmptyEvent_sp(0, basicAllocator)
, d_queues(config.d_numQueues, QueueInfo(), basicAllocator)
, d_started(false)
, d_monitorEventHandle()
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // NOTHING
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
    /// Enum for the various RC error categories
    enum RcEnum {
        rc_SUCCESS            = 0,
        rc_NOT_ENOUGH_THREADS = -1,
        rc_ALREADY_STARTED    = -2
    };

    if (isStarted()) {
        // MQTP has already been started
        return rc_ALREADY_STARTED;  // RETURN
    }

    // Verify threads availability
    const int numAvailableThreads =
        d_config.d_threadPool_p->maxThreads() -
        d_config.d_threadPool_p->numActiveThreads();
    if (numAvailableThreads < static_cast<int>(d_queues.size())) {
        // Not enough threads for exclusive use
        return rc_NOT_ENOUGH_THREADS;  // RETURN
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
    }

    BSLS_ASSERT_SAFE(d_config.d_threadPool_p->enabled());

    // Set up threads
    for (size_t i = 0; i < d_queues.size(); ++i) {
        BSLA_MAYBE_UNUSED const int rc = d_config.d_threadPool_p->enqueueJob(
            bdlf::BindUtil::bind(&ThisClass::processQueue,
                                 this,
                                 static_cast<int>(i)));
        BSLS_ASSERT_SAFE(rc == 0);
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

    return rc_SUCCESS;
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

        // According to `d_processQueueRefCount` usage contract,
        // we have to apply the following updates here:
        // (1) Since we enqueue the next monitor event to the queue:
        //     `info.d_processQueueRefCount.addRelaxed(1);`
        // (2) Since we are getting rid of the initial reference:
        //     `info.d_processQueueRefCount.subtractRelaxed(1);`
        //
        // These two updates balance each other (+1 -1 = 0), so we can keep
        // the current value of `info.d_processQueueRefCount` unchanged.

        info.d_queue_p->pushBack(NULL /* monitor event */);
        // It is possible that something is enqueued to the queue between the
        // last monitor event and `disablePushBack()` call, this is expected.
        info.d_queue_p->disablePushBack();

        const bsls::TimeInterval timeout =
            bsls::SystemTime::nowMonotonicClock().addSeconds(
                k_MAX_WAIT_SECONDS_AT_SHUTDOWN);
        const int rc = info.d_finished_sp->timedWait(timeout);
        if (0 != rc) {
            BALL_LOG_ERROR << "#MQTP_STOP_FAILURE MQTP failed to stop in "
                           << k_MAX_WAIT_SECONDS_AT_SHUTDOWN
                           << " seconds while shutting down the queue (" << i
                           << ", " << info.d_name << ", " << info.d_queue_p
                           << "), rc:  " << rc;
            BSLS_ASSERT_OPT(false && "#EXIT Failed to stop MQTP, exiting...");
        }

        EventSp event;
        while (!info.d_queue_p->tryPopFront(&event)) {
            event.reset();
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
inline typename MultiQueueThreadPool<TYPE>::EventSp
MultiQueueThreadPool<TYPE>::getEvent()
{
    return d_pool.getObject();
}

template <typename TYPE>
inline int
MultiQueueThreadPool<TYPE>::enqueueEvent(bslmf::MovableRef<EventSp> event,
                                         int                        queueId)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bslmf::MovableRefUtil::access(event));
    BSLS_ASSERT(0 <= queueId && queueId < numQueues());
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    // [try to] Push back item
    return d_queues[queueId].d_queue_p->pushBack(
        bslmf::MovableRefUtil::move(event));
}

template <typename TYPE>
inline int MultiQueueThreadPool<TYPE>::enqueueEventOnAllQueues(
    bslmf::MovableRef<EventSp> event)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(bslmf::MovableRefUtil::access(event));
    BSLS_ASSERT_SAFE(isStarted() && "MQTP has not been started");

    EventSp eventObj(bslmf::MovableRefUtil::move(event));

    // Enqueue on each selected queue
    int lastError = 0;
    for (size_t queueIdx = 0; queueIdx < d_queues.size(); ++queueIdx) {
        QueueInfo& info = d_queues[queueIdx];

        // [try to] Push back item
        const int pushRet = info.d_queue_p->pushBack(eventObj);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 != pushRet)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            lastError = pushRet;
        }
    }

    return lastError;
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
inline bslmt::ThreadUtil::Id
MultiQueueThreadPool<TYPE>::queueThreadId(int queueId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 <= queueId);
    BSLS_ASSERT_SAFE(queueId < numQueues());

    return d_queues[queueId].d_threadId;
}

}  // close package namespace
}  // close enterprise namespace

#endif
