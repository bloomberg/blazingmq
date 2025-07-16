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

// bmqimp_eventqueue.t.cpp                                            -*-C++-*-
#include <bmqimp_eventqueue.h>

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

#include <bmqst_statcontext.h>
#include <bmqst_statvalue.h>

// BDE
#include <bdlbb_pooledblobbufferfactory.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bmqimp_stat.h>
#include <bslma_managedptr.h>
#include <bsls_atomic.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

void eventHandler(const bsl::shared_ptr<bmqimp::Event>& event,
                  bsls::AtomicInt&                      eventCounter)
{
    PVVV("Handler: " << *event);
    ++eventCounter;
}

/// Create an `Event` object at the specified `address` using the supplied
/// allocator `allocator`; This is used by the Object Pool.
void poolCreateEvent(void*                     address,
                     bdlbb::BlobBufferFactory* bufferFactory,
                     bslma::Allocator*         allocator)
{
    new (address) bmqimp::Event(bufferFactory, allocator);
}

/// A struct to provide controllable timer functions.
struct TestClock {
    // DATA
    bsls::TimeInterval d_realtimeClock;
    bsls::TimeInterval d_monotonicClock;
    bsls::Types::Int64 d_highResTimer;

    // CREATORS
    TestClock()
    : d_realtimeClock()
    , d_monotonicClock()
    , d_highResTimer()
    {
    }

    // The value to return by the next call of the corresponding timer
    // function.

    // MANIPULATORS
    bsls::TimeInterval realtimeClock() { return d_realtimeClock; }

    bsls::TimeInterval monotonicClock() { return d_monotonicClock; }

    bsls::Types::Int64 highResTimer() { return d_highResTimer; }
};

void performanceTestQueuePusher(bmqimp::EventQueue* queue, int numIter)
{
    PRINT_SAFE("Enqueuing " << numIter << " items.");
    for (int i = 0; i < numIter; ++i) {
        bsl::shared_ptr<bmqimp::Event> event = queue->eventPool()->getObject();
        event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED,
                                       1,
                                       bmqt::CorrelationId(),
                                       "");
        queue->pushBack(event);
    }
    PRINT_SAFE("Enqueued " << numIter << " items.");
}

void performanceTestQueuePopper(bmqimp::EventQueue* queue, int numIter)
{
    int i = 0;
    for (; i < numIter; i++) {
        bsl::shared_ptr<bmqimp::Event> event = queue->popFront();
        if (!event) {
            break;  // BREAK
        }
    }
    PRINT_SAFE("Finished popping " << i << " items.");
}

void printProcessedItems(int numItems, bsls::Types::Int64 elapsedTime)
{
    const double numSeconds = static_cast<double>(elapsedTime) / 1000000000LL;
    const bsls::Types::Int64 itemsPerSec = numItems / numSeconds;

    bsl::cout << "Processed " << numItems << " items in "
              << bmqu::PrintUtil::prettyTimeInterval(elapsedTime) << ". "
              << bmqu::PrintUtil::prettyNumber(itemsPerSec) << "/s"
              << bsl::endl;
}

void queuePerformance(int                       numReaders,
                      int                       numWriters,
                      int                       numIter,
                      int                       queueSize,
                      bdlbb::BlobBufferFactory* bufferFactory)
{
    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        numReaders + numWriters,          // minThreads
        numReaders + numWriters,          // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);

    PRINT_SAFE("===================");
    PRINT_SAFE("Queue numReaders: " << numReaders
                                    << " numWriters: " << numWriters);
    PRINT_SAFE("===================");
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bmqimp::EventQueue                       obj(
        &eventPool,
        queueSize,      // initialCapacity
        queueSize / 3,  // lowWatermark
        queueSize / 2,  // highWatermark
        emptyEventHandler,
        0,  // numProcessingThreads
        bmqimp::SessionId(bmqp_ctrlmsg::NegotiationMessage()),
        bmqtst::TestHelperUtil::allocator());

    for (int i = 0; i < numReaders; i++) {
        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &performanceTestQueuePopper,
                                  &obj,
                                  numIter / numReaders));
    }

    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    for (int i = 0; i < numWriters; i++) {
        threadPool.enqueueJob(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &performanceTestQueuePusher,
                                  &obj,
                                  numIter / numWriters));
    }
    threadPool.drain();
    bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

    printProcessedItems(numIter, endTime - startTime);
}

}  // close unnamed namespace

// TBD: Add a test case that tests the stats

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bdlbb::PooledBlobBufferFactory           bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue obj(&eventPool,
                           1,  // initialCapacity
                           3,  // lowWatermark
                           6,  // highWatermark
                           emptyEventHandler,
                           0,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    // Basic testing.. enqueue one item, pop it out ..
    bsl::shared_ptr<bmqimp::Event> event = eventPool.getObject();
    event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED,
                                   1,
                                   bmqt::CorrelationId(),
                                   "");
    PV("Enqueuing: " << (*event));
    obj.pushBack(event);

    // Dequeue.. expect statusCode 1
    event = obj.popFront();
    PV("Dequeued: " << (*event));
    BMQTST_ASSERT_EQ(event->statusCode(), 1);
}

static void test2_capacityTest()
// ------------------------------------------------------------------------
// CAPACITY TEST
//
// Concerns:
//   1. Check that bmqimp::EventQueue can store more message events than
//      its initial capacity (provided by the user).
//
// Plan:
//   1. Create bmqimp::EventQueue of a minimal 'size'.
//   2. Enqueue lots of message events until after we hit capacity for
//      message events and ensure all events are successfully enqueued
//      (size + 1 message events).
//   3. Dequeue events and verify that the queue contains exactly
//      'size + 2' events (one additional session events).
//
// Testing manipulators:
//   - createEvent
//   - pushBack
//   - popFront
//   - timedPopFront
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("CAPACITY TEST");

    const int k_INITIAL_CAPACITY = 3;

    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder         builder(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;

    bmqimp::EventQueue obj(&eventPool,
                           k_INITIAL_CAPACITY,      // initialCapacity
                           0,                       // lowWatermark
                           k_INITIAL_CAPACITY - 1,  // highWatermark
                           emptyEventHandler,
                           0,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    builder.startMessage();
    const bdlbb::Blob& eventBlob = *builder.blob();

    bmqp::Event rawEvent(&eventBlob, bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqimp::Event> event;

    // Enqueue k_INITIAL_CAPACITY events
    for (int i = 0; i < k_INITIAL_CAPACITY; ++i) {
        event = eventPool.getObject();
        event->configureAsMessageEvent(rawEvent);
        PVV("[" << i << "] Enqueuing: " << (*event));
        BMQTST_ASSERT_EQ_D(i, obj.pushBack(event), 0);
    }

    // Trying to push one over succeeds
    event = eventPool.getObject();
    event->configureAsMessageEvent(rawEvent);

    BMQTST_ASSERT_EQ(obj.pushBack(event), 0);

    // Dequeue k_INITIAL_CAPACITY + 1 plus two events (2 session events)
    for (int i = 0; i < k_INITIAL_CAPACITY + 1 + 2; ++i) {
        event = obj.popFront();
        PVV("Dequeued: " << (*event));
        BMQTST_ASSERT(event != 0);
    }

    // No more events
    bsls::TimeInterval timeout(0, 2000000);  // 2 ms
    event = obj.timedPopFront(timeout);

    BMQTST_ASSERT(event != 0);
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_TIMEOUT);
}

static void test3_watermark()
{
    bmqtst::TestHelper::printTestName("WATERMARK");

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bdlbb::PooledBlobBufferFactory           bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue obj(&eventPool,
                           1,  // initialCapacity
                           3,  // lowWatermark
                           6,  // highWatermark
                           emptyEventHandler,
                           0,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    bsl::shared_ptr<bmqimp::Event> event;

    // Test that HighWatermark is created and prioritized
    PV("Enqueue 6 items");
    for (int i = 0; i < 6; ++i) {
        event = eventPool.getObject();
        event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED,
                                       i,
                                       bmqt::CorrelationId(),
                                       "");
        PVV("Enqueuing: " << (*event));
        obj.pushBack(event);
    }

    // Now deque.. it should be a HighWatermark event
    event = obj.popFront();
    PV("Dequeued: " << (*event));
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_SLOWCONSUMER_HIGHWATERMARK);

    // And dequeue 6 times checking order
    for (int i = 0; i < 6; ++i) {
        event = obj.popFront();
        PVV("Dequeued: " << (*event));
        BMQTST_ASSERT_EQ(event->statusCode(), i);
    }

    // Finally we should be able to dequeue a last item, consumer_normal
    // (note that this event is not prioritized)
    event = obj.popFront();
    PV("Dequeued: " << (*event));
    BMQTST_ASSERT_EQ(event->sessionEventType(),
                     bmqt::SessionEventType::e_SLOWCONSUMER_NORMAL);
}

static void test4_basicEventHandlerTest()
// ------------------------------------------------------------------------
// BASIC EVENT HANDLER TEST
//
// Concerns:
//   1. Check that bmqimp::EventQueue calls custom event handler.
//
// Plan:
//   1. Create bmqimp::EventQueue with several 'NUM_THREADS' processing
//      threads and
//      user defined event handler.
//   2. In the main thread enqueue one session message.
//   3. Stop the queue and check that the handler is called exactly
//      'NUM_THREADS' + 1 times (there should be DISCONNECTED events from
//      each thread).
//
// Testing manipulators:
//   - createEvent
//   - pushBack
//   - popFront
//   - timedPopFront
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BASIC EVENT HANDLER");

    const int                      k_NUM_THREADS = 15;
    bsls::AtomicInt                eventCounter;
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue obj(&eventPool,
                           100,  // initialCapacity
                           3,    // lowWatermark
                           90,   // highWatermark
                           bdlf::BindUtil::bind(&eventHandler,
                                                bdlf::PlaceHolders::_1,
                                                bsl::ref(eventCounter)),
                           k_NUM_THREADS,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    obj.start();

    // Enqueue one item
    bsl::shared_ptr<bmqimp::Event> event;
    event = eventPool.getObject();
    event->configureAsSessionEvent(bmqt::SessionEventType::e_UNDEFINED);

    PVV("Enqueuing: " << (*event));
    obj.pushBack(event);

    // Stop the queue
    obj.enqueuePoisonPill();
    obj.stop();

    // The event handler should be called once for the user event
    BMQTST_ASSERT_EQ(eventCounter, 1);
}

static void test5_emptyStatsTest()
// ------------------------------------------------------------------------
// EMPTY STATS TEST
//
// Concerns:
//   1. Check the basic behavior of bmqimp::EventQueue statistics
//      functions.
//
// Plan:
//   1. Create bmqimp::EventQueue object and enable statistics collection.
//   2. Without any push and pop operations print the statistics to an
//      output stream and compare the output with the expected pattern.
//
// Testing manipulators:
//   - initializeStats
//   - printStats
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EMPTY STATS");

    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());

    expected << "\n"
             << " Queue  | Queue Time"
             << "\n"
             << "Abs. Max| Abs. Max"
             << "\n"
             << "--------+---------"
             << "\n"
             << "       0|         "
             << "\n"
             << "\n";

    bmqst::StatContextConfiguration config(
        "stats",
        bmqtst::TestHelperUtil::allocator());

    config.defaultHistorySize(2);

    bmqst::StatContext rootStatContext(config,
                                       bmqtst::TestHelperUtil::allocator());

    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;

    start.setLevel(0).setIndex(0);
    end.setLevel(0).setIndex(1);

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;
    bdlbb::PooledBlobBufferFactory           bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue obj(&eventPool,
                           1,  // initialCapacity
                           3,  // lowWatermark
                           6,  // highWatermark
                           emptyEventHandler,
                           0,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_SAFE_FAIL(obj.printStats(out, false));

    obj.initializeStats(&rootStatContext, start, end);

    BMQTST_ASSERT_EQ(rootStatContext.numSubcontexts(), 0);

    rootStatContext.snapshot();

    BMQTST_ASSERT_EQ(rootStatContext.numSubcontexts(), 1);

    obj.printStats(out, false);

    BMQTST_ASSERT_EQ(out.str(), expected.str());
}

static void test6_workingStatsTest()
// ------------------------------------------------------------------------
// WORKING STATS TEST
//
// Concerns:
//   1. Check the behavior of bmqimp::EventQueue statistics after some
//      push and pop operations.
//
// Plan:
//   1. Create bmqimp::EventQueue object with a custom time source and
//      enable statistics collection.
//   2. Perform push and pop operations and manually increase the custom
//      time source.
//   3. Print the extended statistics to an output stream and compare the
//      output with the expected pattern.
//
// Testing manipulators:
//   - initializeStats
//   - printStats
//   ----------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EMPTY STATS");

    const int k_MILL_SEC = bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND;

    const int k_INITIAL_CAPACITY = 100;
    const int k_QUEUE_LWM        = 3;
    const int k_QUEUE_HWM        = k_INITIAL_CAPACITY - 4;
    const int k_QUEUE_POPPED     = k_INITIAL_CAPACITY - 2;
    const int k_QUEUE_WAIT       = k_MILL_SEC * 3;
    // NOTE an extra item will be internally enqueued by the object for the
    //      HighWatermark and another one for the LowWatermark

    TestClock          testClock;
    bmqu::MemOutStream out(bmqtst::TestHelperUtil::allocator());
    bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator());
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bmqp::BlobPoolUtil::BlobSpPoolSp blobSpPool(
        bmqp::BlobPoolUtil::createBlobPool(
            &bufferFactory,
            bmqtst::TestHelperUtil::allocator()));
    bmqp::PutEventBuilder         builder(blobSpPool.get(),
                                  bmqtst::TestHelperUtil::allocator());
    bmqimp::EventQueue::EventPool eventPool(
        bdlf::BindUtil::bind(&poolCreateEvent,
                             bdlf::PlaceHolders::_1,  // address
                             &bufferFactory,
                             bdlf::PlaceHolders::_2),  // allocator
        -1,
        bmqtst::TestHelperUtil::allocator());

    bmqimp::EventQueue::EventHandlerCallback emptyEventHandler;

    bmqst::StatContextConfiguration config(
        "stats",
        bmqtst::TestHelperUtil::allocator());
    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;

    config.defaultHistorySize(3);

    bmqst::StatContext rootStatContext(config,
                                       bmqtst::TestHelperUtil::allocator());

    start.setLevel(0).setIndex(0);
    end.setLevel(0).setIndex(1);

    bmqsys::Time::shutdown();
    bmqsys::Time::initialize(
        bdlf::BindUtil::bind(&TestClock::realtimeClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::monotonicClock, &testClock),
        bdlf::BindUtil::bind(&TestClock::highResTimer, &testClock),
        bmqtst::TestHelperUtil::allocator());

    expected << "\n"
             << "                        Queue                        |       "
                "       Queue Time             \n"
                "Enqueue (delta)| Dequeue (delta)| Size| Max| Abs. Max| Min | "
                "  Avg   |    Max   | Abs. Max \n"
                "---------------+----------------+-----+----+---------+-----+-"
                "--------+----------+----------\n"
                "            102|              98|    4| 101|      101| 0 ns| "
                "54.43 ms| 103.00 ms| 103.00 ms\n"
             << "\n";

    bmqimp::EventQueue obj(&eventPool,
                           k_INITIAL_CAPACITY,  // initialCapacity
                           k_QUEUE_LWM,         // lowWatermark
                           k_QUEUE_HWM,         // highWatermark
                           emptyEventHandler,
                           0,  // numProcessingThreads
                           bmqimp::SessionId(),
                           bmqtst::TestHelperUtil::allocator());

    // May also call 'start' for the queue without custom event
    // handler
    BMQTST_ASSERT_EQ(obj.start(), 0);

    obj.initializeStats(&rootStatContext, start, end);

    builder.startMessage();
    const bdlbb::Blob& eventBlob = *builder.blob();

    bmqp::Event rawEvent(&eventBlob, bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<bmqimp::Event> event;

    // Enqueue events
    for (int i = 0; i < k_INITIAL_CAPACITY; ++i) {
        event = eventPool.getObject();
        event->configureAsMessageEvent(rawEvent);
        PVV("[" << i << "] Enqueuing: " << (*event));
        BMQTST_ASSERT_EQ_D(i, obj.pushBack(event), 0);
        testClock.d_highResTimer += k_MILL_SEC;
    }

    testClock.d_highResTimer += k_QUEUE_WAIT;

    // And dequeue some of them
    for (int i = 0; i < k_QUEUE_POPPED; ++i) {
        event = obj.popFront();
        BMQTST_ASSERT(event != 0);
        PVV("Dequeued: " << (*event));
    }

    rootStatContext.snapshot();
    obj.printStats(out, true);

    PVV(out.str());

    BMQTST_ASSERT_EQ(out.str(), expected.str());

    const bmqst::StatContext* pSubCtx = rootStatContext.getSubcontext(
        "EventQueue");

    BMQTST_ASSERT(pSubCtx != 0);
    BMQTST_ASSERT_EQ(pSubCtx->numValues(), 2);
    BMQTST_ASSERT_EQ(pSubCtx->valueName(0), "Queue");
    BMQTST_ASSERT_EQ(pSubCtx->valueName(1), "Time");

    bmqst::StatValue valQueue =
        pSubCtx->value(bmqst::StatContext::e_DIRECT_VALUE, 0);

    bmqst::StatValue valTime =
        pSubCtx->value(bmqst::StatContext::e_DIRECT_VALUE, 1);

    BMQTST_ASSERT_EQ(valQueue.min(), 0);
    BMQTST_ASSERT_EQ(valQueue.max(),
                     k_INITIAL_CAPACITY + 1);  // +1 for HighWatermark
    BMQTST_ASSERT_EQ(valTime.min(), 0);
    BMQTST_ASSERT_EQ(valTime.max(),
                     k_INITIAL_CAPACITY * k_MILL_SEC + k_QUEUE_WAIT);
}

static void testN1_performance()
// ------------------------------------------------------------------------
// QUEUE - PERFORMANCE TEST
//
// Concerns:
//  a) Check the performance of bmqimp::EventQueue for the case of
//     multiple readers and a single writer.
//
// Plan:
//  1) Create a bmqimp::EventQueue and enqueue events as quickly as
//     possible on it.
//  2) See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    // CONSTANTS
    const int k_NUM_ITERATIONS   = 10 * 1000 * 1000;  // 10 M
    const int k_FIXED_QUEUE_SIZE = 10 * 1000 * 1000;  // 10 M
    bdlbb::PooledBlobBufferFactory bufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());

    queuePerformance(1,
                     1,
                     k_NUM_ITERATIONS,
                     k_FIXED_QUEUE_SIZE,
                     &bufferFactory);
    queuePerformance(2,
                     1,
                     k_NUM_ITERATIONS,
                     k_FIXED_QUEUE_SIZE,
                     &bufferFactory);
    queuePerformance(4,
                     1,
                     k_NUM_ITERATIONS,
                     k_FIXED_QUEUE_SIZE,
                     &bufferFactory);
    queuePerformance(8,
                     1,
                     k_NUM_ITERATIONS,
                     k_FIXED_QUEUE_SIZE,
                     &bufferFactory);
    queuePerformance(16,
                     1,
                     k_NUM_ITERATIONS,
                     k_FIXED_QUEUE_SIZE,
                     &bufferFactory);
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqsys::Time::initialize();
    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 6: test6_workingStatsTest(); break;
    case 5: test5_emptyStatsTest(); break;
    case 4: test4_basicEventHandlerTest(); break;
    case 3: test3_watermark(); break;
    case 2: test2_capacityTest(); break;
    case 1: test1_breathingTest(); break;
    case -1: testN1_performance(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();
    bmqsys::Time::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // Default: EventQueue uses bmqc::MonitoredFixedQueue, which uses
    //          'bdlcc::SharedObjectPool' which uses bslmt::Semaphore which
    //          generates a unique name using an ostringstream, hence the
    //          default allocator.
}
