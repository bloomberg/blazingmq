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

// bmqc_multiqueuethreadpool.t.cpp                                    -*-C++-*-
#include <bmqc_multiqueuethreadpool.h>

#include <bmqc_monitoredqueue_bdlccfixedqueue.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlcc_fixedqueue.h>
#include <bdlcc_objectpool.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bdlmt_threadpool.h>
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_map.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_threadattributes.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>
#include <bsls_timeutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BENCHMARKING LIBRARY
#ifdef BMQTST_BENCHMARK_ENABLED
#include <benchmark/benchmark.h>
#endif

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

struct TestItem {
  private:
    // PRIVATE DATA
    int d_value;

  public:
    // CREATORS
    TestItem()
    : d_value(0)
    {
        // NOTHING
    }

    // MANIPULATORS
    void reset() { d_value = 0; }

    int& value() { return d_value; }
};

typedef bmqc::MultiQueueThreadPool<TestItem> MQTP;

static MQTP::Queue*
queueCreator(MQTP::QueueCreatorRet*            ret,
             int                               queueId,
             bslma::Allocator*                 allocator,
             int                               fixedQueueSize,
             bsl::map<int, bsl::vector<int> >* queueContextMap)
{
    PV("Creating queue [queueId: " << queueId << "]\n");

    ret->context().load(&(*queueContextMap)[queueId],
                        0,
                        &bslma::ManagedPtrUtil::noOpDeleter);

    return new (*allocator) MQTP::Queue(fixedQueueSize, allocator);
}

static void eventCb(BSLA_UNUSED int queueId, void* context, MQTP::Event* event)
{
    if (event->type() == MQTP::Event::BMQC_USER) {
        bsl::vector<int>* vec = reinterpret_cast<bsl::vector<int>*>(context);
        vec->push_back(event->object().value());
    }
}

static MQTP::Queue* performanceTestQueueCreator(bslma::Allocator* allocator,
                                                int fixedQueueSize)
{
    return new (*allocator) MQTP::Queue(fixedQueueSize, allocator);
}

void performanceTestEventCb()
{
    // NOTHING
}

struct PerformanceTestObject {
    int d_value;
};

typedef bdlcc::ObjectPool<PerformanceTestObject> PerformanceTestObjectPool;

static void performanceTestFixedQueuePopper(
    bdlcc::FixedQueue<PerformanceTestObject*>* queue,
    PerformanceTestObjectPool*                 pool)
{
    while (true) {
        PerformanceTestObject* obj = queue->popFront();
        if (!obj) {
            break;  // BREAK
        }

        pool->releaseObject(obj);
    }
}

static void printProcessedItems(int numItems, bsls::Types::Int64 elapsedTime)
{
    const double numSeconds = static_cast<double>(elapsedTime) / 1000000000LL;
    const bsls::Types::Int64 itemsPerSec = numItems / numSeconds;

    bsl::cout << "Processed " << numItems << " items in "
              << bmqu::PrintUtil::prettyTimeInterval(elapsedTime) << ". "
              << bmqu::PrintUtil::prettyNumber(itemsPerSec) << "/s"
              << bsl::endl;
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   Basic functionality.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Ignore default allocator check for now because instantiating the
    // 'MQTP::Config' object fails the default allocator check for a
    // reason yet to be identified (the source appears to be in
    // 'bmqc::MultiQueueThreadPoolUtil::defaultCreator<int>()').

    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // CONSTANTS
    const int k_NUM_QUEUES       = 3;
    const int k_FIXED_QUEUE_SIZE = 10;

    bsl::map<int, bsl::vector<int> > queueContextMap(
        bmqtst::TestHelperUtil::allocator());

    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        3,                                // minThreads
        3,                                // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);

    MQTP::Config config(
        k_NUM_QUEUES,
        &threadPool,
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &eventCb,
                              bdlf::PlaceHolders::_1,   // queueId
                              bdlf::PlaceHolders::_2,   // context
                              bdlf::PlaceHolders::_3),  // event
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &queueCreator,
                              bdlf::PlaceHolders::_1,  // ret
                              bdlf::PlaceHolders::_2,  // queueId
                              bdlf::PlaceHolders::_3,  // allocator
                              k_FIXED_QUEUE_SIZE,
                              &queueContextMap),
        bmqtst::TestHelperUtil::allocator());

    MQTP mfqtp(config, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(mfqtp.isStarted(), false);
    BMQTST_ASSERT_EQ(mfqtp.numQueues(), k_NUM_QUEUES);
    BMQTST_ASSERT_EQ(mfqtp.start(), 0);
    BMQTST_ASSERT_NE(mfqtp.start(), 0);  // MQTP has already been started
    BMQTST_ASSERT_EQ(mfqtp.isStarted(), true);

    MQTP::Event* event = mfqtp.getUnmanagedEvent();
    event->object().value() = 0;
    mfqtp.enqueueEvent(event, 0);

    event           = mfqtp.getUnmanagedEvent();
    event->object().value() = 1;
    mfqtp.enqueueEvent(event, 1);

    event           = mfqtp.getUnmanagedEvent();
    event->object().value() = 2;
    mfqtp.enqueueEvent(event, 2);

    event           = mfqtp.getUnmanagedEvent();
    event->object().value() = 3;
    mfqtp.enqueueEventOnAllQueues(event);

    mfqtp.stop();
    BMQTST_ASSERT_EQ(mfqtp.isStarted(), false);

    BMQTST_ASSERT_EQ(queueContextMap[0].size(), 2U);
    BMQTST_ASSERT_EQ(queueContextMap[0][0], 0);
    BMQTST_ASSERT_EQ(queueContextMap[0][1], 3);

    BMQTST_ASSERT_EQ(queueContextMap[0].size(), 2U);
    BMQTST_ASSERT_EQ(queueContextMap[1][0], 1);
    BMQTST_ASSERT_EQ(queueContextMap[1][1], 3);

    BMQTST_ASSERT_EQ(queueContextMap[0].size(), 2U);
    BMQTST_ASSERT_EQ(queueContextMap[2][0], 2);
    BMQTST_ASSERT_EQ(queueContextMap[2][1], 3);

    threadPool.stop();
}

BSLA_MAYBE_UNUSED
static void testN1_performance()
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MQTP over a bdlcc::FixedQueue
//
// Plan:
//  1) Create a MQTP with a single queue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//  2) Do the same with a bdlcc::FixedQueue
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_QUEUES       = 1;
    const int k_NUM_ITERATIONS   = 10 * 1000 * 1000;  // 10 M
    const int k_FIXED_QUEUE_SIZE = 250 * 1000;        // 250K

    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        3,                                // minThreads
        3,                                // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);

    // Test with MQTP
    PRINT("====");
    PRINT("MQTP");
    PRINT("====");

    MQTP::Config config(
        k_NUM_QUEUES,
        &threadPool,
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestEventCb),
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestQueueCreator,
                              bdlf::PlaceHolders::_3,
                              k_FIXED_QUEUE_SIZE),
        bmqtst::TestHelperUtil::allocator());

    MQTP mfqtp(config, bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(mfqtp.start() == 0);

    // 1.
    bsls::Types::Int64 startTime = bsls::TimeUtil::getTimer();
    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items.");
    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        MQTP::Event* event = mfqtp.getUnmanagedEvent();
        event->object().value() = 0;
        mfqtp.enqueueEvent(event, 0);
    }
    PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

    mfqtp.waitUntilEmpty();
    bsls::Types::Int64 endTime = bsls::TimeUtil::getTimer();

    printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);

    // 2. .. again
    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items ...");
    startTime = bsls::TimeUtil::getTimer();
    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        MQTP::Event* event = mfqtp.getUnmanagedEvent();
        event->object().value() = 0;
        mfqtp.enqueueEvent(event, 0);
    }
    PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

    mfqtp.waitUntilEmpty();
    endTime = bsls::TimeUtil::getTimer();

    printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);

    PRINT("Stopping mfqtp ...");
    mfqtp.stop();

    // Now test with fixedQueue
    PRINT("=================");
    PRINT("bdlcc::FixedQueue");
    PRINT("=================");

    PerformanceTestObjectPool                 objectPool(-1,
                                         bmqtst::TestHelperUtil::allocator());
    bdlcc::FixedQueue<PerformanceTestObject*> fixedQueue(
        k_FIXED_QUEUE_SIZE,
        bmqtst::TestHelperUtil::allocator());

    // #1
    threadPool.enqueueJob(
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestFixedQueuePopper,
                              &fixedQueue,
                              &objectPool));

    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items ...");
    startTime = bsls::TimeUtil::getTimer();
    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        PerformanceTestObject* obj = objectPool.getObject();
        obj->d_value               = 0;
        fixedQueue.pushBack(obj);
    }
    fixedQueue.pushBack(0);
    PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

    while (!fixedQueue.isEmpty()) {
        bslmt::ThreadUtil::yield();
    }
    endTime = bsls::TimeUtil::getTimer();

    printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);

    // #2 .. again
    threadPool.enqueueJob(
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestFixedQueuePopper,
                              &fixedQueue,
                              &objectPool));

    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items ...");
    startTime = bsls::TimeUtil::getTimer();
    for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
        PerformanceTestObject* obj = objectPool.getObject();
        obj->d_value               = 0;
        fixedQueue.pushBack(obj);
    }
    fixedQueue.pushBack(0);
    PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

    while (!fixedQueue.isEmpty()) {
        bslmt::ThreadUtil::yield();
    }
    endTime = bsls::TimeUtil::getTimer();

    printProcessedItems(k_NUM_ITERATIONS, endTime - startTime);
}

#ifdef BMQTST_BENCHMARK_ENABLED
static void testN1_performance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MQTP
//
// Plan:
//  1) Create a MQTP with a single queue and enqueue events as quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_QUEUES       = 1;
    const int k_NUM_ITERATIONS   = 10 * 1000 * 1000;  // 10 M
    const int k_FIXED_QUEUE_SIZE = 250 * 1000;        // 250K

    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        3,                                // minThreads
        3,                                // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);

    // Test with MQTP
    PRINT("====");
    PRINT("MQTP");
    PRINT("====");

    MQTP::Config config(
        k_NUM_QUEUES,
        &threadPool,
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestEventCb),
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestQueueCreator,
                              bdlf::PlaceHolders::_3,
                              k_FIXED_QUEUE_SIZE),
        bmqtst::TestHelperUtil::allocator());

    MQTP mfqtp(config, bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(mfqtp.start() == 0);

    // 1.

    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items.");
    for (auto _ : state) {
        for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
            MQTP::Event* event = mfqtp.getUnmanagedEvent();
            event->object().value() = 0;
            mfqtp.enqueueEvent(event, 0);
        }
        PRINT("Enqueued " << k_NUM_ITERATIONS << " items.");

        mfqtp.waitUntilEmpty();
        mfqtp.stop();
    }
}

static void testN1_fixedPerformance_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of a bdlcc::FixedQueue
//
// Plan:
//  1) Create a bdlcc::FixedQueue with a single queue and enqueue events as
//  quickly as
//     possible on it.  See how many we can process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("FIXED PERFORMANCE TEST");

    // CONSTANTS
    const int k_NUM_ITERATIONS   = 10 * 1000 * 1000;  // 10 M
    const int k_FIXED_QUEUE_SIZE = 250 * 1000;        // 250K

    bdlmt::ThreadPool threadPool(
        bslmt::ThreadAttributes(),        // default
        3,                                // minThreads
        3,                                // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(threadPool.start() == 0);
    // Now test with fixedQueue
    PRINT("=================");
    PRINT("bdlcc::FixedQueue");
    PRINT("=================");

    PerformanceTestObjectPool                 objectPool(-1,
                                         bmqtst::TestHelperUtil::allocator());
    bdlcc::FixedQueue<PerformanceTestObject*> fixedQueue(
        k_FIXED_QUEUE_SIZE,
        bmqtst::TestHelperUtil::allocator());

    // #1
    threadPool.enqueueJob(
        bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                              &performanceTestFixedQueuePopper,
                              &fixedQueue,
                              &objectPool));

    PRINT("Enqueuing " << k_NUM_ITERATIONS << " items ...");
    for (auto _ : state) {
        for (int i = 0; i < k_NUM_ITERATIONS; ++i) {
            PerformanceTestObject* obj = objectPool.getObject();
            obj->d_value               = 0;
            fixedQueue.pushBack(obj);
        }
        fixedQueue.pushBack(0);

        while (!fixedQueue.isEmpty()) {
            bslmt::ThreadUtil::yield();
        }
    }
}
#endif  // BMQTST_BENCHMARK_ENABLED

//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    case -1:
#ifdef BMQTST_BENCHMARK_ENABLED
        BENCHMARK(testN1_fixedPerformance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_performance_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        testN1_performance();
#endif
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
