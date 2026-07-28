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

#include <bmqc_monitoredqueue_bdlccsingleconsumerqueue.h>

// BDE
#include <bdlcc_singleconsumerqueue.h>
#include <bdlf_bind.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsla_annotations.h>
#include <bslmt_barrier.h>
#include <bslmt_latch.h>
#include <bslmt_threadattributes.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_table.h>
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

/// Value pushed onto the queue for each measured element.
const int k_ITEM = 1;

/// Sentinel value pushed after all producers are done, signaling the
/// consumer thread to stop.
const int k_SENTINEL = 0;

/// Wait on the specified `startBarrier`, pop elements from the specified
/// `queue` until the sentinel value is dequeued, then arrive at the
/// specified `doneLatch`.
template <class QUEUE>
static void queuePopper(QUEUE*          queue,
                        bslmt::Barrier* startBarrier,
                        bslmt::Latch*   doneLatch)
{
    startBarrier->wait();

    while (true) {
        int value = k_ITEM;
        queue->popFront(&value);

        if (value == k_SENTINEL) {
            break;  // BREAK
        }
    }

    doneLatch->arrive();
}

/// Wait on the specified `startBarrier`, push the specified `iterations`
/// number of elements onto the specified `queue`, then arrive at the
/// specified `doneLatch`.
template <class QUEUE>
static void queuePusher(int             iterations,
                        QUEUE*          queue,
                        bslmt::Barrier* startBarrier,
                        bslmt::Latch*   doneLatch)
{
    startBarrier->wait();

    for (int i = 0; i < iterations; ++i) {
        queue->pushBack(k_ITEM);
    }

    doneLatch->arrive();
}

/// Append a row to the specified `table` reporting that a queue named
/// `queueName` driven by `numPushers` producer threads processed
/// `numItems` elements in `elapsedTime` nanoseconds.
static void addResult(bmqtst::Table*     table,
                      const char*        queueName,
                      int                numPushers,
                      int                numItems,
                      bsls::Types::Int64 elapsedTime)
{
    const double numSeconds = static_cast<double>(elapsedTime) / 1000000000LL;
    const bsls::Types::Uint64 itemsPerSec = static_cast<bsls::Types::Uint64>(
        numItems / numSeconds);

    table->column("Queue").insertValue(queueName);
    table->column("Pushers").insertValue(
        static_cast<bsls::Types::Uint64>(numPushers));
    table->column("Items").insertValue(
        static_cast<bsls::Types::Uint64>(numItems));
    table->column("Time (ns)")
        .insertValue(static_cast<bsls::Types::Uint64>(elapsedTime));
    table->column("Per op (ns)")
        .insertValue(static_cast<bsls::Types::Uint64>(elapsedTime / numItems));
    table->column("Items/s").insertValue(itemsPerSec);
}

/// @brief Measure concurrent push/pop throughput of a queue and record it.
///
/// Push `numIterations` elements onto `queue` using `numPushers` producer
/// threads, all drained by a single consumer thread, and append the measured
/// results to `table`.  A synchronous warmup, whose cost is not measured,
/// fills and drains the queue first so that the measured round is not skewed
/// by the underlying queue's internal allocations or cold caches.
///
/// @param table         Table to which the measured row is appended.
/// @param queueName     Display name of the queue under test.
/// @param queue         Queue to exercise; empty on entry and on return.
/// @param queueSize     Working size used to size the warmup fill/drain.
/// @param numIterations Total number of elements to push in the measured run.
/// @param numPushers    Number of concurrent producer threads.
template <class QUEUE>
static void runPerformanceTest(bmqtst::Table* table,
                               const char*    queueName,
                               QUEUE*         queue,
                               int            queueSize,
                               int            numIterations,
                               int            numPushers)
{
    // Warmup: fill the queue to its working size then drain it, so that the
    // underlying queue's internal structures are allocated and the caches are
    // warm before the measured round.  This leaves the queue empty.
    for (int i = 0; i < queueSize; ++i) {
        queue->pushBack(k_ITEM);
    }
    for (int i = 0; i < queueSize; ++i) {
        int value = k_ITEM;
        queue->popFront(&value);
    }

    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        numPushers + 1,                   // minThreads (pushers + popper)
        numPushers + 1,                   // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);

    // The popper, all pushers and this thread rendezvous on the barrier so
    // that the timing below excludes thread startup latency and every worker
    // begins at the same instant.
    bslmt::Barrier startBarrier(numPushers + 2);

    bslmt::Latch popperDone(1);
    threadPool.enqueueJob(
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &queuePopper<QUEUE>,
                              queue,
                              &startBarrier,
                              &popperDone));

    bslmt::Latch pushersDone(numPushers);

    for (int i = 0; i < numPushers; ++i) {
        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &queuePusher<QUEUE>,
                                  numIterations / numPushers,
                                  queue,
                                  &startBarrier,
                                  &pushersDone));
    }

    startBarrier.wait();
    const bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();

    pushersDone.wait();

    queue->pushBack(k_SENTINEL);

    popperDone.wait();

    const bsls::Types::Int64 elapsed = bsls::TimeUtil::getTimer() - startTime;

    addResult(table, queueName, numPushers, numIterations, elapsed);
}

}  // Close anonymous namespace

// Check that all member functions can be instantiated.

namespace BloombergLP {
namespace bmqc {

template class MonitoredQueue<bdlcc::SingleConsumerQueue<int> >;

}  // close package namespace
}  // close enterprise namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_MonitoredSingleConsumerQueue_breathingTest()
// ------------------------------------------------------------------------
// MONITORED SINGLECONSUMER QUEUE - BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
//   MonitoredSingleConsumerQueue(int      queueSize,
//                       bslma::Allocator *basicAllocator = 0);
//   MonitoredSingleConsumerQueue(int      queueSize,
//                       bool              supportTimedOperations,
//                       bslma::Allocator *basicAllocator = 0);
//
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MONITORED SINGLECONSUMER QUEUE "
                                      "- BREATHING TEST");

    // CONSTRAINS
    const int k_QUEUE_SIZE      = 10;
    const int k_LOW_WATERMARK   = 3;
    const int k_HIGH_WATERMARK  = 6;
    const int k_HIGH_WATERMARK2 = 9;

    {
        PV("Constructor without 'timedOpertions' flag");

        bmqc::MonitoredQueue<bdlcc::SingleConsumerQueue<int> > queue(
            k_QUEUE_SIZE,
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
        BMQTST_ASSERT_EQ(queue.state(), bmqc::MonitoredQueueState::e_NORMAL);

        queue.setWatermarks(k_LOW_WATERMARK,
                            k_HIGH_WATERMARK,
                            k_HIGH_WATERMARK2);

        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
        BMQTST_ASSERT_EQ(queue.state(), bmqc::MonitoredQueueState::e_NORMAL);

        BMQTST_ASSERT_EQ(queue.lowWatermark(), k_LOW_WATERMARK);
        BMQTST_ASSERT_EQ(queue.highWatermark(), k_HIGH_WATERMARK);
        BMQTST_ASSERT_EQ(queue.highWatermark2(), k_HIGH_WATERMARK2);

        // pushBack two items
        BMQTST_ASSERT_EQ(queue.pushBack(1), 0);
        BMQTST_ASSERT_EQ(queue.numElements(), 1);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        BMQTST_ASSERT_EQ(queue.tryPushBack(2), 0);
        BMQTST_ASSERT_EQ(queue.numElements(), 2);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        // Verify timed popFront is undefined
        int item = -1;

        // popFront two items
        item = -1;
        BMQTST_ASSERT_EQ(queue.tryPopFront(&item), 0);
        BMQTST_ASSERT_EQ(item, 1);
        BMQTST_ASSERT_EQ(queue.numElements(), 1);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        item = -1;
        queue.popFront(&item);
        BMQTST_ASSERT_EQ(item, 2);
        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
    }

    {
        PV("Constructor with 'timedOperations' flag");

        bmqc::MonitoredQueue<bdlcc::SingleConsumerQueue<int> > queue(
            k_QUEUE_SIZE,
            true,
            // supportTimedOperations
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
        BMQTST_ASSERT_EQ(queue.state(), bmqc::MonitoredQueueState::e_NORMAL);

        queue.setWatermarks(k_LOW_WATERMARK,
                            k_HIGH_WATERMARK,
                            k_HIGH_WATERMARK2);

        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
        BMQTST_ASSERT_EQ(queue.state(), bmqc::MonitoredQueueState::e_NORMAL);

        BMQTST_ASSERT_EQ(queue.lowWatermark(), k_LOW_WATERMARK);
        BMQTST_ASSERT_EQ(queue.highWatermark(), k_HIGH_WATERMARK);
        BMQTST_ASSERT_EQ(queue.highWatermark2(), k_HIGH_WATERMARK2);

        // pushBack two items
        BMQTST_ASSERT_EQ(queue.pushBack(1), 0);
        BMQTST_ASSERT_EQ(queue.numElements(), 1);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        BMQTST_ASSERT_EQ(queue.pushBack(2), 0);
        BMQTST_ASSERT_EQ(queue.numElements(), 2);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        // popFront two items
        // 1. timedPopFront
        int                      item    = -1;
        const bsls::TimeInterval timeout = bsls::TimeInterval(
            0,
            5 * bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND);
        BMQTST_ASSERT_EQ(queue.timedPopFront(&item, timeout), 0);
        BMQTST_ASSERT_EQ(item, 1)
        BMQTST_ASSERT_EQ(queue.numElements(), 1);
        BMQTST_ASSERT_EQ(queue.isEmpty(), false);

        // 2. popFront
        item = -1;
        queue.popFront(&item);
        BMQTST_ASSERT_EQ(item, 2);
        BMQTST_ASSERT_EQ(queue.numElements(), 0);
        BMQTST_ASSERT_EQ(queue.isEmpty(), true);
    }
}

static void test2_MonitoredSingleConsumerQueue_exceed_reset()
// ------------------------------------------------------------------------
// MONITORED SINGLECONSUMER QUEUE - RESET
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
    bmqtst::TestHelper::printTestName(
        "MONITORED SINGLECONSUMER QUEUE - RESET");

    // CONSTRAINS
    const int k_QUEUE_SIZE      = 10;
    const int k_LOW_WATERMARK   = 3;
    const int k_HIGH_WATERMARK  = 6;
    const int k_HIGH_WATERMARK2 = 9;

    bmqc::MonitoredQueue<bdlcc::SingleConsumerQueue<int> > queue(
        k_QUEUE_SIZE,
        bmqtst::TestHelperUtil::allocator());
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

    BMQTST_ASSERT_EQ(queue.tryPushBack(10), 0);

    BMQTST_ASSERT_EQ(queue.numElements(), k_QUEUE_SIZE + 1);
    BMQTST_ASSERT_EQ(queue.isEmpty(), false);

    // 2. Reset the queue and verify that items were removed and state is reset
    //    to an empty queue.
    queue.reset();

    BMQTST_ASSERT_EQ(queue.numElements(), 0);
    BMQTST_ASSERT_EQ(queue.isEmpty(), true);
    BMQTST_ASSERT_EQ(queue.state(), bmqc::MonitoredQueueState::e_NORMAL);
}

static void testN1_performance()
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the 'bmqc::MonitoredQueue' over a raw
//     'bdlcc::SingleConsumerQueue' for concurrent push/pop, varying the
//     number of producer threads.
//
// Plan:
//  1) For each queue type and each producer-thread count, push a fixed
//     number of elements as quickly as possible while a single consumer
//     drains the queue, and measure the elapsed time.
//  2) Tabulate the results.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_ITERATIONS  = 10 * 1000 * 1000;  // 10 M
    const int k_QUEUE_SIZE      = 250 * 1000;        // 250K
    const int k_PUSHERS[]       = {1, 5};
    const int k_NUM_PUSHER_SETS = sizeof(k_PUSHERS) / sizeof(k_PUSHERS[0]);

    typedef bmqc::MonitoredQueue<bdlcc::SingleConsumerQueue<int> >
                                            MonitoredIntQueue;
    typedef bdlcc::SingleConsumerQueue<int> SingleConsumerIntQueue;

    bmqtst::Table table(bmqtst::TestHelperUtil::allocator());

    for (int i = 0; i < k_NUM_PUSHER_SETS; ++i) {
        const int numPushers = k_PUSHERS[i];

        {
            MonitoredIntQueue queue(k_QUEUE_SIZE,
                                    bmqtst::TestHelperUtil::allocator());
            runPerformanceTest(&table,
                               "bmqc::MonitoredQueue",
                               &queue,
                               k_QUEUE_SIZE,
                               k_NUM_ITERATIONS,
                               numPushers);
        }

        {
            SingleConsumerIntQueue queue(k_QUEUE_SIZE,
                                         bmqtst::TestHelperUtil::allocator());
            runPerformanceTest(&table,
                               "bdlcc::SingleConsumerQueue",
                               &queue,
                               k_QUEUE_SIZE,
                               k_NUM_ITERATIONS,
                               numPushers);
        }
    }

    table.print(bsl::cout);
}

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 2: test2_MonitoredSingleConsumerQueue_exceed_reset(); break;
    case 1: test1_MonitoredSingleConsumerQueue_breathingTest(); break;
    case -1: testN1_performance(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

// ----------------------------------------------------------------------------
