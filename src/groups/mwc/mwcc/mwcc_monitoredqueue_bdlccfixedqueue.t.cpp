// Copyright 2023 Bloomberg Finance L.P.
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

// mwcc_monitoredqueue_bdlccfixedqueue.t.cpp                          -*-C++-*-
#include <mwcc_monitoredqueue_bdlccfixedqueue.h>

// MWC
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlcc_objectpool.h>
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadattributes.h>
#include <bslmt_threadutil.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

const int           k_BUSY_WORK        = 3;
static unsigned int s_antiOptimization = 0;

static inline void busyWork(int load)
{
    int j = 1;
    for (int i = 0; i < load; ++i) {
        j = j * 3 % 7;
    }
    s_antiOptimization += j;
}

struct PerformanceTestObject {
    int d_value;
};

typedef bdlcc::ObjectPool<PerformanceTestObject> PerformanceTestObjectPool;

typedef mwcc::MonitoredQueue<bdlcc::FixedQueue<PerformanceTestObject*> >
    PerformanceTestObjectQueue;

template <class QUEUE>
static void performanceTestPopper(QUEUE*                     queue,
                                  PerformanceTestObjectPool* pool)
{
    while (true) {
        PerformanceTestObject* obj = 0;
        queue->popFront(&obj);

        if (!obj) {
            break;  // BREAK
        }

        busyWork(k_BUSY_WORK);
        pool->releaseObject(obj);
    }
}

template <class QUEUE>
static void performanceTestPusher(int                        iterations,
                                  QUEUE*                     queue,
                                  PerformanceTestObjectPool* pool,
                                  bslmt::Semaphore*          sem)
{
    for (int i = 0; i < iterations; ++i) {
        PerformanceTestObject* obj = pool->getObject();
        obj->d_value               = 0;
        queue->pushBack(obj);
    }

    sem->post();
}

static void printProcessedItems(int numItems, bsls::Types::Int64 elapsedTime)
{
    const double numSeconds = static_cast<double>(elapsedTime) / 1000000000LL;
    const bsls::Types::Int64 itemsPerSec = static_cast<bsls::Types::Int64>(
        numItems / numSeconds);

    bsl::cout << "Processed " << numItems << " items in "
              << mwcu::PrintUtil::prettyTimeInterval(elapsedTime) << ". "
              << mwcu::PrintUtil::prettyNumber(itemsPerSec) << "/s"
              << bsl::endl;
}

}  // Close anonymous namespace

// Check that all member functions can be instantiated.

namespace BloombergLP {
namespace mwcc {

template class MonitoredQueue<bdlcc::FixedQueue<PerformanceTestObject*> >;

}  // close package namespace
}  // close enterprise namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_MonitoredQueue_breathingTest()
// ------------------------------------------------------------------------
// MONITORED QUEUE - BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
//   MonitoredQueue(int               queueSize,
//                       bslma::Allocator *basicAllocator = 0);
//   MonitoredQueue(int               queueSize,
//                       bool              supportTimedOperations,
//                       bslma::Allocator *basicAllocator = 0);
//
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("MONITORED QUEUE - BREATHING TEST");

    // CONSTRAINS
    const int k_QUEUE_SIZE      = 10;
    const int k_LOW_WATERMARK   = 3;
    const int k_HIGH_WATERMARK  = 6;
    const int k_HIGH_WATERMARK2 = 9;

    {
        PV("Constructor without 'timedOpertions' flag");

        mwcc::MonitoredQueue<bdlcc::FixedQueue<int> > queue(k_QUEUE_SIZE,
                                                            s_allocator_p);

        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
        ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_NORMAL);

        queue.setWatermarks(k_LOW_WATERMARK,
                            k_HIGH_WATERMARK,
                            k_HIGH_WATERMARK2);

        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
        ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_NORMAL);

        ASSERT_EQ(queue.lowWatermark(), k_LOW_WATERMARK);
        ASSERT_EQ(queue.highWatermark(), k_HIGH_WATERMARK);
        ASSERT_EQ(queue.highWatermark2(), k_HIGH_WATERMARK2);

        // pushBack two items
        ASSERT_EQ(queue.pushBack(1), 0);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 1);
        ASSERT_EQ(queue.isEmpty(), false);

        ASSERT_EQ(queue.tryPushBack(2), 0);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 2);
        ASSERT_EQ(queue.isEmpty(), false);

        int item = -1;

        // Disabled because failure is reported for the "wrong" component :-|
        // Failure in component mwcc_monitoredqueue but expected component
        // mwcc_monitoredqueue_bdlccfixedqueue
        //
        // Verify timed popFront is undefined
        // // int                      item    = -1;
        // const bsls::TimeInterval timeout = bsls::TimeInterval(
        //     0, 5 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
        // ASSERT_SAFE_FAIL(queue.timedPopFront(&item, timeout));
        // (void)timeout;  // prod-build compiler happiness

        // popfront two items
        item = -1;
        ASSERT_EQ(queue.tryPopFront(&item), 0);
        ASSERT_EQ(item, 1);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 1);
        ASSERT_EQ(queue.isEmpty(), false);

        item = -1;
        queue.popFront(&item);
        ASSERT_EQ(item, 2);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
    }

    {
        PV("Constructor with 'timedOperations' flag");

        mwcc::MonitoredQueue<bdlcc::FixedQueue<int> > queue(
            k_QUEUE_SIZE,
            true,  // supportTimedOperations
            s_allocator_p);

        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
        ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_NORMAL);

        queue.setWatermarks(k_LOW_WATERMARK,
                            k_HIGH_WATERMARK,
                            k_HIGH_WATERMARK2);

        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
        ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_NORMAL);

        ASSERT_EQ(queue.lowWatermark(), k_LOW_WATERMARK);
        ASSERT_EQ(queue.highWatermark(), k_HIGH_WATERMARK);
        ASSERT_EQ(queue.highWatermark2(), k_HIGH_WATERMARK2);

        // pushBack two items
        ASSERT_EQ(queue.pushBack(1), 0);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 1);
        ASSERT_EQ(queue.isEmpty(), false);

        ASSERT_EQ(queue.pushBack(2), 0);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 2);
        ASSERT_EQ(queue.isEmpty(), false);

        // popFront two items
        // 1. timedPopFront
        int                      item    = -1;
        const bsls::TimeInterval timeout = bsls::TimeInterval(
            0,
            5 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
        ASSERT_EQ(queue.timedPopFront(&item, timeout), 0);
        ASSERT_EQ(item, 1)
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 1);
        ASSERT_EQ(queue.isEmpty(), false);

        // 2. popFront
        item = -1;
        queue.popFront(&item);
        ASSERT_EQ(item, 2);
        ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
        ASSERT_EQ(queue.numElements(), 0);
        ASSERT_EQ(queue.isEmpty(), true);
    }
}

static void test2_MonitoredQueue_reset()
// ------------------------------------------------------------------------
// MONITORED QUEUE - RESET
//
// Concerns:
//   Ensure that resetting the queue removes all items from the queue and
//   resets its state to an empty queue.
//
// Plan:
//   1. Enqueue items until the queue is full
//   2. Reset the queue and verify that items were removed and state is
//      reset to an empty queue.
//
// Testing:
//   reset
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("MONITORED QUEUE - RESET");

    // CONSTRAINS
    const int k_QUEUE_SIZE      = 10;
    const int k_LOW_WATERMARK   = 3;
    const int k_HIGH_WATERMARK  = 6;
    const int k_HIGH_WATERMARK2 = 9;

    mwcc::MonitoredQueue<bdlcc::FixedQueue<int> > queue(k_QUEUE_SIZE,
                                                        s_allocator_p);
    queue.setWatermarks(k_LOW_WATERMARK, k_HIGH_WATERMARK, k_HIGH_WATERMARK2);

    // 1. Enqueue items until the queue is full
    queue.tryPushBack(0);
    queue.tryPushBack(1);
    queue.tryPushBack(2);
    queue.tryPushBack(3);
    queue.tryPushBack(4);
    queue.tryPushBack(5);
    queue.tryPushBack(6);
    queue.tryPushBack(7);
    queue.tryPushBack(8);
    queue.tryPushBack(9);

    ASSERT_EQ(queue.tryPushBack(10), -1);

    ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
    ASSERT_EQ(queue.numElements(), k_QUEUE_SIZE);
    ASSERT_EQ(queue.isEmpty(), false);
    ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_QUEUE_FILLED);

    // 2. Reset the queue and verify that items were removed and state is reset
    //    to an empty queue.
    queue.reset();

    ASSERT_EQ(queue.capacity(), k_QUEUE_SIZE);
    ASSERT_EQ(queue.numElements(), 0);
    ASSERT_EQ(queue.isEmpty(), true);
    ASSERT_EQ(queue.state(), mwcc::MonitoredQueueState::e_NORMAL);
}

BSLA_MAYBE_UNUSED
static void testN1_MonitoredQueue_performance()
// ------------------------------------------------------------------------
// MONITORED QUEUE - PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MonitoredQueue over a
//     bdlcc::Queue
//
// Plan:
//  1) Create a MonitoredQueue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//  2) Do the same with a bdlcc::Queue
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;

    mwctst::TestHelper::printTestName("MONITORED QUEUE "
                                      "- PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_ITERATIONS = 10 * 1000 * 1000;  // 10 M
    const int k_QUEUE_SIZE     = 250 * 1000;        // 250K
    const int k_NUM_PUSHERS    = 5;

    PRINT("===================");
    PRINT("MonitoredFixedQueue");
    PRINT("===================");
    PerformanceTestObjectPool  objectPool1(-1, s_allocator_p);
    PerformanceTestObjectQueue monitoredQueue(k_QUEUE_SIZE, s_allocator_p);

    // #1
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            1,                                // minThreads
            1,                                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(bdlf::BindUtil::bindS(
            s_allocator_p,
            &performanceTestPopper<PerformanceTestObjectQueue>,
            &monitoredQueue,
            &objectPool1));

        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        PRINT("Enqueuing " << k_NUM_ITERATIONS << " items.");
        for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
            PerformanceTestObject* obj = objectPool1.getObject();
            obj->d_value               = 0;
            monitoredQueue.pushBack(obj);
        }
        monitoredQueue.pushBack(0);
        PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

        while (!monitoredQueue.isEmpty()) {
            bslmt::ThreadUtil::yield();
        }
        bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

        printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);
    }

    // #2 .. using multiple producer threads
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(bdlf::BindUtil::bindS(
            s_allocator_p,
            &performanceTestPopper<PerformanceTestObjectQueue>,
            &monitoredQueue,
            &objectPool1));

        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        PRINT("Enqueuing " << k_NUM_ITERATIONS << " items using "
                           << k_NUM_PUSHERS << " threads.");

        bslmt::Semaphore pushersDone;

        for (int i = 0; i < k_NUM_PUSHERS; ++i) {
            threadPool.enqueueJob(bdlf::BindUtil::bindS(
                s_allocator_p,
                &performanceTestPusher<PerformanceTestObjectQueue>,
                k_NUM_ITERATIONS / k_NUM_PUSHERS,
                &monitoredQueue,
                &objectPool1,
                &pushersDone));
        }

        for (int i = 0; i < k_NUM_PUSHERS; ++i) {
            pushersDone.wait();
        }

        monitoredQueue.pushBack(0);
        PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

        while (!monitoredQueue.isEmpty()) {
            bslmt::ThreadUtil::yield();
        }

        bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

        printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);
    }

    PRINT("=================");
    PRINT("bdlcc::FixedQueue");
    PRINT("=================");

    PerformanceTestObjectPool objectPool2(-1, s_allocator_p);
    typedef bdlcc::FixedQueue<PerformanceTestObject*> UnmonitoredQueue;
    UnmonitoredQueue queue(k_QUEUE_SIZE, s_allocator_p);

    // #1
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &performanceTestPopper<UnmonitoredQueue>,
                                  &queue,
                                  &objectPool2));

        PRINT("Enqueuing " << k_NUM_ITERATIONS << " items ...");
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
        for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
            PerformanceTestObject* obj = objectPool2.getObject();
            obj->d_value               = 0;
            queue.pushBack(obj);
        }
        queue.pushBack(0);
        PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

        while (!queue.isEmpty()) {
            bslmt::ThreadUtil::yield();
        }
        bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

        printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);
    }

    // #2 .. again
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);
        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &performanceTestPopper<UnmonitoredQueue>,
                                  &queue,
                                  &objectPool2));

        PRINT("Enqueuing " << k_NUM_ITERATIONS << " items using "
                           << k_NUM_PUSHERS << " threads.");

        bslmt::Semaphore   pushersDone;
        bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();

        for (int i = 0; i < k_NUM_PUSHERS; ++i) {
            threadPool.enqueueJob(
                bdlf::BindUtil::bindS(s_allocator_p,
                                      &performanceTestPusher<UnmonitoredQueue>,
                                      k_NUM_ITERATIONS / k_NUM_PUSHERS,
                                      &queue,
                                      &objectPool2,
                                      &pushersDone));
        }

        for (int i = 0; i < k_NUM_PUSHERS; ++i) {
            pushersDone.wait();
        }

        queue.pushBack(0);
        PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

        while (!queue.isEmpty()) {
            bslmt::ThreadUtil::yield();
        }
        bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

        printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);
    }

    PRINT("s_antiOptimization = " << s_antiOptimization);
}

#ifdef BSLS_PLATFORM_OS_LINUX
static void
testN1_MonitoredQueue_performance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// MONITORED QUEUE - PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MonitoredQueue
//
// Plan:
//  1) Create a MonitoredQueue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;

    mwctst::TestHelper::printTestName("MONITORED QUEUE "
                                      "- PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_ITERATIONS = 10 * 1000 * 1000;  // 10 M
    const int k_QUEUE_SIZE     = 250 * 1000;        // 250K

    PRINT("===================");
    PRINT("MonitoredFixedQueue");
    PRINT("===================");
    PerformanceTestObjectPool  objectPool1(-1, s_allocator_p);
    PerformanceTestObjectQueue monitoredQueue(k_QUEUE_SIZE, s_allocator_p);

    // #1
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            1,                                // minThreads
            1,                                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(bdlf::BindUtil::bindS(
            s_allocator_p,
            &performanceTestPopper<PerformanceTestObjectQueue>,
            &monitoredQueue,
            &objectPool1));
        for (auto _ : state) {
            for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
                PerformanceTestObject* obj = objectPool1.getObject();
                obj->d_value               = 0;
                monitoredQueue.pushBack(obj);
            }
            monitoredQueue.pushBack(0);

            while (!monitoredQueue.isEmpty()) {
                bslmt::ThreadUtil::yield();
            }
        }
    }
}

static void testN1_MonitoredQueueThreaded_performance_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// MONITORED QUEUE - THREADED PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MonitoredQueue with multithreading
//
// Plan:
//  1) Create a MonitoredQueue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;

    mwctst::TestHelper::printTestName("MONITORED QUEUE "
                                      "- PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_ITERATIONS = 10 * 1000 * 1000;  // 10 M
    const int k_QUEUE_SIZE     = 250 * 1000;        // 250K
    const int k_NUM_PUSHERS    = 5;

    PerformanceTestObjectPool  objectPool1(-1, s_allocator_p);
    PerformanceTestObjectQueue monitoredQueue(k_QUEUE_SIZE, s_allocator_p);
    // #2 .. using multiple producer threads
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(bdlf::BindUtil::bindS(
            s_allocator_p,
            &performanceTestPopper<PerformanceTestObjectQueue>,
            &monitoredQueue,
            &objectPool1));
        for (auto _ : state) {
            bslmt::Semaphore pushersDone;

            for (int i = 0; i < k_NUM_PUSHERS; ++i) {
                threadPool.enqueueJob(bdlf::BindUtil::bindS(
                    s_allocator_p,
                    &performanceTestPusher<PerformanceTestObjectQueue>,
                    k_NUM_ITERATIONS / k_NUM_PUSHERS,
                    &monitoredQueue,
                    &objectPool1,
                    &pushersDone));
            }

            for (int i = 0; i < k_NUM_PUSHERS; ++i) {
                pushersDone.wait();
            }

            monitoredQueue.pushBack(0);

            while (!monitoredQueue.isEmpty()) {
                bslmt::ThreadUtil::yield();
            }
        }
    }
}

static void
testN1_bdlccFixedQueue_performance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
//  bdlcc::FIXED QUEUE - PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the bdlcc::Queue
//
// Plan:
//  1) Create a bdlcc::Queue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------

{
    const int k_NUM_ITERATIONS = 10 * 1000 * 1000;  // 10 M
    const int k_NUM_PUSHERS    = 5;
    const int k_QUEUE_SIZE     = 250 * 1000;  // 250K
    PRINT("=================");
    PRINT("bdlcc::FixedQueue");
    PRINT("=================");

    PerformanceTestObjectPool objectPool2(-1, s_allocator_p);
    typedef bdlcc::FixedQueue<PerformanceTestObject*> UnmonitoredQueue;
    UnmonitoredQueue queue(k_QUEUE_SIZE, s_allocator_p);

    // #1
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);

        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &performanceTestPopper<UnmonitoredQueue>,
                                  &queue,
                                  &objectPool2));

        for (auto _ : state) {
            for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
                PerformanceTestObject* obj = objectPool2.getObject();
                obj->d_value               = 0;
                queue.pushBack(obj);
            }
            queue.pushBack(0);

            while (!queue.isEmpty()) {
                bslmt::ThreadUtil::yield();
            }
        }
    }
}

static void testN1_bdlccFixedQueueThreaded_performance_GoogleBenchmark(
    benchmark::State& state)
// ------------------------------------------------------------------------
// bdlcc::FIXED QUEUE - THREADED PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the bdlcc::Queue with multithreading
//
// Plan:
//  1) Create a bdlcc::Queue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------

{
    const int                 k_NUM_ITERATIONS = 10 * 1000 * 1000;  // 10 M
    const int                 k_QUEUE_SIZE     = 250 * 1000;        // 250K
    const int                 k_NUM_PUSHERS    = 5;
    PerformanceTestObjectPool objectPool2(-1, s_allocator_p);
    typedef bdlcc::FixedQueue<PerformanceTestObject*> UnmonitoredQueue;
    UnmonitoredQueue queue(k_QUEUE_SIZE, s_allocator_p);
    // #2 .. again
    {
        bdlmt::ThreadPool threadPool(
            bslmt::ThreadAttributes(),        // default
            k_NUM_PUSHERS + 1,                // minThreads
            k_NUM_PUSHERS + 1,                // maxThreads
            bsl::numeric_limits<int>::max(),  // maxIdleTime
            s_allocator_p);
        BSLS_ASSERT_OPT(threadPool.start() == 0);
        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &performanceTestPopper<UnmonitoredQueue>,
                                  &queue,
                                  &objectPool2));

        bslmt::Semaphore pushersDone;
        for (auto _ : state) {
            for (int i = 0; i < k_NUM_PUSHERS; ++i) {
                threadPool.enqueueJob(bdlf::BindUtil::bindS(
                    s_allocator_p,
                    &performanceTestPusher<UnmonitoredQueue>,
                    k_NUM_ITERATIONS / k_NUM_PUSHERS,
                    &queue,
                    &objectPool2,
                    &pushersDone));
            }

            for (int i = 0; i < k_NUM_PUSHERS; ++i) {
                pushersDone.wait();
            }

            queue.pushBack(0);

            while (!queue.isEmpty()) {
                bslmt::ThreadUtil::yield();
            }
        }
    }
}
#endif  // BSLS_PLATFORM_OS_LINUX

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_MonitoredQueue_reset(); break;
    case 1: test1_MonitoredQueue_breathingTest(); break;
    case -1:
#ifdef BSLS_PLATFORM_OS_LINUX
        BENCHMARK(testN1_MonitoredQueue_performance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_MonitoredQueueThreaded_performance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_bdlccFixedQueue_performance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_bdlccFixedQueueThreaded_performance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        testN1_MonitoredQueue_performance();
#endif
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

// ----------------------------------------------------------------------------