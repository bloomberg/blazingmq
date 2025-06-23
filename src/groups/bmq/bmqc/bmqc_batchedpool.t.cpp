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

// bmqc_batchedpool.t.cpp                                             -*-C++-*-
#include <bmqc_batchedpool.h>

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
#include <bslmt_barrier.h>
#include <bslmt_latch.h>
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

/// The number of iterations per each thread in all benchmarks
static const size_t k_NUM_ITERATIONS = 1000000;

static bsls::AtomicInt64 g_NUM_CONSTRUCT = 0;
static bsls::AtomicInt64 g_NUM_DESTRUCT = 0;

struct TestItem {
  private:
    // PRIVATE DATA
    int d_value;

  public:
    // CREATORS
    TestItem()
    : d_value(0)
    {
        g_NUM_CONSTRUCT.addRelaxed(1);
    }

    ~TestItem() {
        g_NUM_DESTRUCT.addRelaxed(1);
    }

    // MANIPULATORS
    void reset() {
        d_value = 0;
    }

    int& value() { return d_value; }
};

void creatorFn(void* arena, bslma::Allocator* allocator) {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(arena);
        BSLS_ASSERT_SAFE(allocator);

        bslalg::ScalarPrimitives::construct(reinterpret_cast<TestItem*>(arena),
                                            allocator);
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
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    balst::StackTraceTestAllocator allocator(bmqtst::TestHelperUtil::allocator());

    {
        bmqc::BatchedObjectPool<TestItem> itemPool(128, &allocator);

        bsl::shared_ptr<bmqc::LocalBatcher<TestItem> > batcher = itemPool.getBatcher();

        bsl::vector<bmqc::BatchNode<TestItem>* > objects;
        objects.reserve(k_NUM_ITERATIONS);

        for (size_t i = 0; i < k_NUM_ITERATIONS; i++) {
            objects.push_back(batcher->getObject());
        }

        for (size_t i = 0; i < k_NUM_ITERATIONS; i++) {
            objects[i]->release();
        }
        objects.clear();
    }
    // All TestItem objects must be destructed

    BMQTST_ASSERT_EQ(g_NUM_CONSTRUCT, g_NUM_DESTRUCT);
    PRINT(g_NUM_CONSTRUCT);    PRINT(g_NUM_DESTRUCT);
}

template <size_t k_NUM_THREADS, size_t k_BATCH_SIZE>
static void testN1_batchedPool_benchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MQTP with multiple producer threads
//
// Plan:
//  1) Create a MQTP with a single queue and enqueue events as quickly as
//     possible on it from multiple producer threads.  See how many we can
//     process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("MQTP MULTI PRODUCER PERFORMANCE TEST");

    struct Local {
        static void benchFn(bmqc::BatchedObjectPool<TestItem>* pool_p,
                            bslmt::Latch*   initLatch_p,
                            bslmt::Barrier* startBarrier_p,
                            bslmt::Latch*   finishLatch_p)
        {
            // PRECONDITIONS
            BSLS_ASSERT_OPT(pool_p);
            BSLS_ASSERT_OPT(initLatch_p);
            BSLS_ASSERT_OPT(startBarrier_p);
            BSLS_ASSERT_OPT(finishLatch_p);

            bsl::shared_ptr<bmqc::LocalBatcher<TestItem> > batcher = pool_p->getBatcher();

            initLatch_p->arrive();
            startBarrier_p->wait();

            for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
                bmqc::BatchNode<TestItem>* item = batcher->getObject();
                item->d_object.object().value() = i;
                item->release();
            }

            finishLatch_p->arrive();
        }
    };

    bmqc::BatchedObjectPool<TestItem> itemPool(k_BATCH_SIZE, bmqtst::TestHelperUtil::allocator());

    bdlmt::ThreadPool workThreadPool(
        bslmt::ThreadAttributes(),        // default
        k_NUM_THREADS,                    // minThreads
        k_NUM_THREADS,                    // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(workThreadPool.start() == 0);

    bslmt::Latch   initThreadLatch(k_NUM_THREADS);
    bslmt::Barrier startBenchmarkBarrier(k_NUM_THREADS + 1);
    bslmt::Latch finishBenchmarkLatch(k_NUM_THREADS);

    for (size_t i = 0; i < k_NUM_THREADS; i++) {
        workThreadPool.enqueueJob(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &Local::benchFn,
                                  &itemPool,
                                  &initThreadLatch,
                                  &startBenchmarkBarrier,
                                  &finishBenchmarkLatch));
    }

    initThreadLatch.wait();

    size_t iter = 0;
    for (auto _ : state) {
        // Benchmark time start

        // We don't support running multi-iteration benchmarks because we
        // prepare and start complex tasks in separate threads.
        // Once these tasks are finished, we cannot simply re-run them without
        // reinitialization, and it goes against benchmark library design. 
        // Make sure we run this only once.
        BSLS_ASSERT_OPT(0 == iter++ && "Must be run only once");

        startBenchmarkBarrier.wait();
        finishBenchmarkLatch.wait();

        // Benchmark time end
    }

    workThreadPool.stop();
}

template <size_t k_NUM_THREADS>
static void testN1_legacyPool_benchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE TEST
//
// Concerns:
//  a) Check the overhead of the MQTP with multiple producer threads
//
// Plan:
//  1) Create a MQTP with a single queue and enqueue events as quickly as
//     possible on it from multiple producer threads.  See how many we can
//     process in a few seconds.
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;

    bmqtst::TestHelper::printTestName("MQTP MULTI PRODUCER PERFORMANCE TEST");

    typedef bsl::function<void(void* arena, bslma::Allocator* allocator)>
        CreatorFn;

    typedef bdlcc::ObjectPool<TestItem, CreatorFn, bdlcc::ObjectPoolFunctors::Reset<TestItem> > TestItemPool;

    struct Local {
        static void benchFn(TestItemPool*   pool_p,
                            bslmt::Latch*   initLatch_p,
                            bslmt::Barrier* startBarrier_p,
                            bslmt::Latch*   finishLatch_p)
        {
            // PRECONDITIONS
            BSLS_ASSERT_OPT(pool_p);
            BSLS_ASSERT_OPT(initLatch_p);
            BSLS_ASSERT_OPT(startBarrier_p);
            BSLS_ASSERT_OPT(finishLatch_p);

            initLatch_p->arrive();
            startBarrier_p->wait();

            for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
                TestItem* item = pool_p->getObject();
                item->value() = i;
                pool_p->releaseObject(item);
            }

            finishLatch_p->arrive();
        }
    };

    TestItemPool itemPool(&creatorFn, 128, bmqtst::TestHelperUtil::allocator());

    bdlmt::ThreadPool workThreadPool(
        bslmt::ThreadAttributes(),        // default
        k_NUM_THREADS,                    // minThreads
        k_NUM_THREADS,                    // maxThreads
        bsl::numeric_limits<int>::max(),  // maxIdleTime
        bmqtst::TestHelperUtil::allocator());
    BSLS_ASSERT_OPT(workThreadPool.start() == 0);

    bslmt::Latch   initThreadLatch(k_NUM_THREADS);
    bslmt::Barrier startBenchmarkBarrier(k_NUM_THREADS + 1);
    bslmt::Latch finishBenchmarkLatch(k_NUM_THREADS);

    for (size_t i = 0; i < k_NUM_THREADS; i++) {
        workThreadPool.enqueueJob(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &Local::benchFn,
                                  &itemPool,
                                  &initThreadLatch,
                                  &startBenchmarkBarrier,
                                  &finishBenchmarkLatch));
    }

    initThreadLatch.wait();

    size_t iter = 0;
    for (auto _ : state) {
        // Benchmark time start

        // We don't support running multi-iteration benchmarks because we
        // prepare and start complex tasks in separate threads.
        // Once these tasks are finished, we cannot simply re-run them without
        // reinitialization, and it goes against benchmark library design. 
        // Make sure we run this only once.
        BSLS_ASSERT_OPT(0 == iter++ && "Must be run only once");

        startBenchmarkBarrier.wait();
        finishBenchmarkLatch.wait();

        // Benchmark time end
    }

    workThreadPool.stop();
}

/// Function alias to simplify benchmark table output.
template <size_t k_NUM_THREADS>
static void testN1_batchedPool_batch_1_benchmark(benchmark::State& state) {
    testN1_batchedPool_benchmark<k_NUM_THREADS, 1>(state);
}

/// Function alias to simplify benchmark table output.
template <size_t k_NUM_THREADS>
static void testN1_batchedPool_batch_32_benchmark(benchmark::State& state) {
    testN1_batchedPool_benchmark<k_NUM_THREADS, 32>(state);
}

/// Function alias to simplify benchmark table output.
template <size_t k_NUM_THREADS>
static void testN1_batchedPool_batch_128_benchmark(benchmark::State& state) {
    testN1_batchedPool_benchmark<k_NUM_THREADS, 128>(state);
}

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
        /// Basic `bdlcc::ObjectPool` usage benchmarks.
        /// Test objects are acquired and released one by one.
        BENCHMARK(testN1_legacyPool_benchmark<1>)
            ->Name("bdlcc::ObjectPool threads=1             ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_legacyPool_benchmark<4>)
            ->Name("bdlcc::ObjectPool threads=4             ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_legacyPool_benchmark<10>)
            ->Name("bdlcc::ObjectPool threads=10            ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        
        /// Using `bmqc::BatchedPool` with batch size 1.
        /// Expecting similar or worse performance than the basic
        /// `bdlcc::ObjectPool`, because batches contain only one test object
        /// and we still pay the full price for batch management.
        BENCHMARK(testN1_batchedPool_batch_1_benchmark<1>)
            ->Name("bmqc::BatchedPool threads=1   batch=1   ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_1_benchmark<4>)
            ->Name("bmqc::BatchedPool threads=4   batch=1   ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_1_benchmark<10>)
            ->Name("bmqc::BatchedPool threads=10  batch=1   ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);

        /// Using `bmqc::BatchedPool` with batch size 32.
        BENCHMARK(testN1_batchedPool_batch_32_benchmark<1>)
            ->Name("bmqc::BatchedPool threads=1   batch=32  ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_32_benchmark<4>)
            ->Name("bmqc::BatchedPool threads=4   batch=32  ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_32_benchmark<10>)
            ->Name("bmqc::BatchedPool threads=10  batch=32  ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);

        /// Using `bmqc::BatchedPool` with batch size 128.
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<1>)
            ->Name("bmqc::BatchedPool threads=1   batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<4>)
            ->Name("bmqc::BatchedPool threads=4   batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<10>)
            ->Name("bmqc::BatchedPool threads=10  batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);

        /// Using `bmqc::BatchedPool` with batch size 128.
        /// Spawning so many threads that desktop CPU cannot process at the
        /// same time.
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<64>)
            ->Name("bmqc::BatchedPool threads=64  batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<128>)
            ->Name("bmqc::BatchedPool threads=128 batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_batchedPool_batch_128_benchmark<256>)
            ->Name("bmqc::BatchedPool threads=256 batch=128 ")
            ->Iterations(1)
            ->Unit(benchmark::kMillisecond);
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#endif
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
