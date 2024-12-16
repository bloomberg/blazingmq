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

// bmqma_countingallocator.t.cpp                                      -*-C++-*-
#include <bmqma_countingallocator.h>

#include <bmqst_basictableinfoprovider.h>
#include <bmqst_statcontext.h>
#include <bmqst_statcontexttableinfoprovider.h>
#include <bmqst_statvalue.h>
#include <bmqst_table.h>
#include <bmqtst_scopedlogobserver.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_severity.h>
#include <bdlf_bind.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bslma_default.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

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

static const char* k_COLS1[] = {""  // Leftmost column is empty
                                ,
                                "Bytes Allocated",
                                "-delta-",
                                "Max Bytes Allocated",
                                "Allocations",
                                "-delta-",
                                "Deallocations",
                                "-delta-"};

const static size_t k_NUM_COLS1 = sizeof(k_COLS1) / sizeof(k_COLS1[0]);

static const char* k_COLS2[] = {"id",
                                "numAllocated",
                                "numAllocatedDelta",
                                "maxAllocated",
                                "numAllocations",
                                "numAllocationsDelta",
                                "numDeallocations",
                                "numDeallocationsDelta"};

const static size_t k_NUM_COLS2 = sizeof(k_COLS2) / sizeof(k_COLS2[0]);

}  // close unnamed namespace

//=============================================================================
//                              TEST CASES
//-----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise basic functionality before beginning testing in earnest.
//   Probe that functionality to discover basic errors.
//
// Testing:
//   CountingAllocator(const bslstl::StringRef&  name,
//                     bslma::Allocator         *allocator = 0);
//   CountingAllocator(const bslstl::StringRef&  name,
//                     bmqst::StatContext       *parentStatContext,
//                     bslma::Allocator         *allocator = 0);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // CONSTANTS
    const char* k_NAME = "Test";

    {
        PV("Constructor - no 'parentStatContext'");

        bmqma::CountingAllocator obj(k_NAME,
                                     bmqtst::TestHelperUtil::allocator());
        BMQTST_ASSERT(obj.context() == 0);
    }

    {
        PV("Constructor - with 'parentStatContext'");

        bmqst::StatContextConfiguration config(
            "test",
            bmqtst::TestHelperUtil::allocator());
        bmqst::StatContext parentStatContext(
            config,
            bmqtst::TestHelperUtil::allocator());
        bmqma::CountingAllocator obj(k_NAME,
                                     &parentStatContext,
                                     bmqtst::TestHelperUtil::allocator());
        parentStatContext.snapshot();

        BMQTST_ASSERT_EQ(parentStatContext.numSubcontexts(), 1);
        BMQTST_ASSERT_NE(obj.context(), &parentStatContext);
        BMQTST_ASSERT_EQ(obj.context(),
                         parentStatContext.getSubcontext("Test"));
    }
}

static void test2_allocate()
// ------------------------------------------------------------------------
// ALLOCATE
//
// Concerns:
//   Ensure allocation succeeds and the appropriate side effects are
//   observed under various edge cases.
//
// Plan:
//   1. Allocate with 'size' of 0 and verify the returned address is 0.
//   2. Allocate non-zero number of bytes and verify the allocation was
//      successful.
//
// Testing:
//   allocate
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ALLOCATE");

    // CONSTANTS
    const bsls::Types::size_type k_SIZE_ALLOC = 1024;

    char*                    buf = 0;
    bmqma::CountingAllocator obj("Test", bmqtst::TestHelperUtil::allocator());

    // 1. Allocate with 'size' of 0 and verify the returned address is 0.
    buf = static_cast<char*>(obj.allocate(0));
    BMQTST_ASSERT(buf == 0);

    // 2. Allocate non-zero number of bytes and verify the allocation was
    // successful.
    buf = static_cast<char*>(obj.allocate(k_SIZE_ALLOC));
    BMQTST_ASSERT(buf != 0);

    bsl::fill_n(buf, k_SIZE_ALLOC, 33);
    for (bsls::Types::size_type i = 0; i < k_SIZE_ALLOC; ++i) {
        BMQTST_ASSERT_EQ_D(i, buf[i], 33);
    }

    BMQTST_ASSERT_SAFE_PASS(obj.deallocate(buf));
}

static void test3_deallocate()
// ------------------------------------------------------------------------
// DEALLOCATE
//
// Concerns:
//   1. Ensure deallocation succeeds and the appropriate side effects are
//      observed under various edge cases.
//   2. Ensure that invalid deallocation attempts fail appropriately.
//
// Plan:
//   1. Deallocate null pointer and verify success and no effect .
//   2. Deallocate a previous allocation and verify success.
//   3. Deallocate previously deallocated memory and verify failure, as
//      well as ensure that an error is logged.
//
// Testing:
//   allocate
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // Logging infrastructure allocates using the default allocator and
    // that is beyond the control of this function
    bmqtst::TestHelper::printTestName("DEALLOCATE");

    // This test twice-deallocates the same block of memory, to verify that
    // such an operation fails. If we're running under MemorySanitizer,
    // AddressSanitizer or ThreadSanitizer, we must skip this test to avoid
    // detecting the issue and aborting.
    //
    // Under MSan, we would instead try to explicitly "unpoison" the memory,
    // but CountingAllocator keeps a hidden "header" block, which we have no
    // good way of accessing to unpoison.
    //
    // Under ASan, we might be able to use the `no_sanitize` attribute, but
    // GCC doesn't support it before version 8.0 - so for now, better just to
    // skip the testcase.
#if defined(__has_feature)  // Clang-supported method for checking sanitizers.
    const bool skipTestForSanitizers = __has_feature(address_sanitizer) ||
                                       __has_feature(memory_sanitizer) ||
                                       __has_feature(thread_sanitizer);
#elif defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_MEMORY__) ||        \
    defined(__SANITIZE_THREAD__)
    // GCC-supported macros for checking ASAN, MSAN and TSAN.
    const bool skipTestForSanitizers = true;
#else
    const bool skipTestForSanitizers = false;  // Default to running the test.
#endif

    if (skipTestForSanitizers) {
        bsl::cout << "Test skipped (running under sanitizer)" << bsl::endl;
        return;  // RETURN
    }

    // CONSTANTS
    const bsls::Types::size_type k_SIZE_ALLOC = 1024;

    bmqst::StatContextConfiguration config(
        "test",
        bmqtst::TestHelperUtil::allocator());
    bmqst::StatContext       parentStatContext(config,
                                         bmqtst::TestHelperUtil::allocator());
    bmqma::CountingAllocator obj("Test",
                                 &parentStatContext,
                                 bmqtst::TestHelperUtil::allocator());

    char* buf = 0;

    // 1. Deallocate null pointer and verify success and no effect.
    BMQTST_ASSERT_SAFE_PASS(obj.deallocate(0));

    // 2. Deallocate a previous allocation and verify success
    buf = static_cast<char*>(obj.allocate(k_SIZE_ALLOC));
    BSLS_ASSERT_OPT(buf != 0);

    BMQTST_ASSERT_SAFE_PASS(obj.deallocate(buf));

    // 3. Deallocate previously deallocated memory and verify failure, as
    //    well as ensure that an error is logged.
    bmqtst::ScopedLogObserver logObserver(ball::Severity::INFO,
                                          bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_SAFE_FAIL(obj.deallocate(buf));
    BMQTST_ASSERT_EQ(logObserver.records().size(), 1U);
}

static void test4_allocationLimit()
// ------------------------------------------------------------------------
// Verify that the 'allocationLimitCb' is invoked when crossing the
// allocation limit; and that it only gets invoked once.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("allocationLimit");

    /// Increment the integer at the specified `value`
    struct local {
        static void incrementInteger(int* value) { ++(*value); }
    };

    {
        PV("AllocationLimitCB is not enabled when no statContext");
        int                      cbInvocationCount = 0;
        bmqma::CountingAllocator obj("Test",
                                     bmqtst::TestHelperUtil::allocator());

        obj.setAllocationLimit(10,
                               bdlf::BindUtil::bind(local::incrementInteger,
                                                    &cbInvocationCount));
        void* alloc = obj.allocate(20);
        BMQTST_ASSERT_EQ(cbInvocationCount, 0);
        obj.deallocate(alloc);
    }

    {
        PV("AllocationLimitCB fires once when crossing limit");

        int                cbInvocationCount = 0;
        bmqst::StatContext statContext(
            bmqst::StatContextConfiguration("myAllocatorStatContext"),
            bmqtst::TestHelperUtil::allocator());
        ;
        bmqma::CountingAllocator obj("Test",
                                     &statContext,
                                     bmqtst::TestHelperUtil::allocator());

        // Note that when using a stat-context enabled counting allocator,
        // there is an allocation overhead for the Header struct, and the
        // allocation size is rounded to alignment; therefore what gets
        // allocated (and internally tracked is a bit bigger then requested, so
        // we don't here that the callback is invoked when exactly reaching the
        // allocation limit, but ensure that once reached, it doesn't get
        // invoked again).
        obj.setAllocationLimit(1024,
                               bdlf::BindUtil::bind(local::incrementInteger,
                                                    &cbInvocationCount));

        BMQTST_ASSERT_EQ(cbInvocationCount, 0);

        void* alloc1 = obj.allocate(128);
        BMQTST_ASSERT_EQ(cbInvocationCount, 0);

        void* alloc2 = obj.allocate(256);
        BMQTST_ASSERT_EQ(cbInvocationCount, 0);

        // Allocate to go beyond limit, callback should now fire
        void* alloc3 = obj.allocate(2048);
        BMQTST_ASSERT_EQ(cbInvocationCount, 1);

        // Allocate again, the callback should no longer be invoked
        void* alloc4 = obj.allocate(1);
        BMQTST_ASSERT_EQ(cbInvocationCount, 1);

        // Cleanup
        obj.deallocate(alloc1);
        obj.deallocate(alloc2);
        obj.deallocate(alloc3);
        obj.deallocate(alloc4);
    }

    {
        PV("Allocation limit correctly keeps track of deallocation");

        // 1. Create a CountingAllocator, and perform 3 allocations, such that
        //    the third one triggers crossing the limit
        {
            int                cbInvocationCount = 0;
            bmqst::StatContext statContext(
                bmqst::StatContextConfiguration("myAllocatorStatContext"),
                bmqtst::TestHelperUtil::allocator());
            ;
            bmqma::CountingAllocator obj("Test",
                                         &statContext,
                                         bmqtst::TestHelperUtil::allocator());

            obj.setAllocationLimit(
                1024,
                bdlf::BindUtil::bind(local::incrementInteger,
                                     &cbInvocationCount));

            void* alloc1 = obj.allocate(400);
            void* alloc2 = obj.allocate(400);
            BMQTST_ASSERT_EQ(cbInvocationCount, 0);

            void* alloc3 = obj.allocate(400);
            BMQTST_ASSERT_EQ(cbInvocationCount, 1);

            obj.deallocate(alloc3);
            obj.deallocate(alloc2);
            obj.deallocate(alloc1);
        }

        // 2. Now that we verified the third allocation crosses the limit, do a
        //    deallocation and reallocate twice: the first one should not
        //    trigger the limit, but the second should
        {
            int                cbInvocationCount = 0;
            bmqst::StatContext statContext(
                bmqst::StatContextConfiguration("myAllocatorStatContext"),
                bmqtst::TestHelperUtil::allocator());
            ;
            bmqma::CountingAllocator obj("Test",
                                         &statContext,
                                         bmqtst::TestHelperUtil::allocator());

            obj.setAllocationLimit(
                1024,
                bdlf::BindUtil::bind(local::incrementInteger,
                                     &cbInvocationCount));

            void* alloc1 = obj.allocate(400);
            void* alloc2 = obj.allocate(400);

            obj.deallocate(alloc2);

            void* alloc3 = obj.allocate(400);
            BMQTST_ASSERT_EQ(cbInvocationCount, 0);

            void* alloc4 = obj.allocate(400);
            BMQTST_ASSERT_EQ(cbInvocationCount, 1);

            obj.deallocate(alloc4);
            obj.deallocate(alloc3);
            obj.deallocate(alloc1);
        }
    }
}

static void test5_allocationLimitHierarchical()
// ------------------------------------------------------------------------
// Verify that allocations from a 'downstream' allocator are propagated to
// the 'upstream' one, eventually invoking its allocation limit.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("allocationLimitHierarchical");

    int cbInvocationCount = 0;

    /// Increment the integer at the specified `value`
    struct local {
        static void incrementInteger(int* value) { ++(*value); }
    };

    // Create the 'top' allocator, with a limit set
    bmqst::StatContext statContext(
        bmqst::StatContextConfiguration("myAllocatorStatContext"),
        bmqtst::TestHelperUtil::allocator());
    ;
    bmqma::CountingAllocator topAlloc("Top",
                                      &statContext,
                                      bmqtst::TestHelperUtil::allocator());

    // Note that when using a stat-context enabled counting allocator, there is
    // an allocation overhead for the Header struct, and the allocation size is
    // rounded to alignment; therefore what gets allocated (and internally
    // tracked) is a bit bigger than requested, so we don't check here that the
    // callback is invoked when exactly reaching the allocation limit, but
    // just ensure that once reached, it doesn't get invoked again.
    topAlloc.setAllocationLimit(1024,
                                bdlf::BindUtil::bind(local::incrementInteger,
                                                     &cbInvocationCount));

    // Create the 'bottom1' allocator, children of 'topAlloc'
    bmqma::CountingAllocator bottomAlloc1("bottom1", &topAlloc);
    bmqma::CountingAllocator bottomAlloc2("bottom2", &bottomAlloc1);

    BMQTST_ASSERT_EQ(cbInvocationCount, 0);

    void* alloc1 = bottomAlloc1.allocate(800);
    BMQTST_ASSERT_EQ(cbInvocationCount, 0);

    void* alloc2 = bottomAlloc2.allocate(800);
    BMQTST_ASSERT_EQ(cbInvocationCount, 1);

    // Allocate more from each allocators, and verify callback is not invoked
    void* alloc3 = bottomAlloc1.allocate(100);
    void* alloc4 = bottomAlloc2.allocate(100);
    void* alloc5 = topAlloc.allocate(100);

    BMQTST_ASSERT_EQ(cbInvocationCount, 1);

    // Cleanup
    topAlloc.deallocate(alloc5);
    bottomAlloc2.deallocate(alloc4);
    bottomAlloc1.deallocate(alloc3);
    bottomAlloc2.deallocate(alloc2);
    bottomAlloc1.deallocate(alloc1);
}

static void test6_configureStatContextTableInfoProvider_part1()
// ------------------------------------------------------------------------
// configureStatContextTableInfoProvider - part 1
//
// Concerns:
//   Ensure that configuring the 'StatContextTableInfoProvider' results in
//   a functional object  having the expected column layout.  Probe that
//   functionality to discover basic errors.
//
// Testing:
//   configureStatContextTableInfoProvider(
//                 bmqst::StatContextTableInfoProvider *tableInfoProvider);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    bmqtst::TestHelper::printTestName("configureStatContextTableInfoProvider"
                                      " - part 1");

    bmqst::StatContextTableInfoProvider tableInfoProvider(
        bmqtst::TestHelperUtil::allocator());

    bmqma::CountingAllocator::configureStatContextTableInfoProvider(
        &tableInfoProvider);

    BMQTST_ASSERT_EQ(tableInfoProvider.hasTitle(), false);
    BMQTST_ASSERT_EQ(tableInfoProvider.numHeaderLevels(), 1);
    BMQTST_ASSERT_EQ(tableInfoProvider.numRows(), 0);
    BMQTST_ASSERT_EQ(static_cast<size_t>(tableInfoProvider.numColumns(0)),
                     k_NUM_COLS1);

    for (int i = 0; i < tableInfoProvider.numColumns(0); ++i) {
        bsl::ostringstream out(bmqtst::TestHelperUtil::allocator());
        tableInfoProvider.printHeader(out, 0, i, 0);
        const bsl::string col(out.str().data(),
                              out.str().length(),
                              bmqtst::TestHelperUtil::allocator());
        PV(i << ": " << col);
        BMQTST_ASSERT_EQ_D(i, col, k_COLS1[i]);
    }
}

static void test7_configureStatContextTableInfoProvider_part2()
// ------------------------------------------------------------------------
// configureStatContextTableInfoProvider - part 2
//
// Concerns:
//   Ensure that configuring the 'BasicTableInfoProvider' associated with
//   with the statContext results in a functional binding between the
//   'BasicTableInfoProvider' and 'Table' having the expected column
//   layout.  Probe that functionality to discover basic errors.
//
// Testing:
//   configureStatContextTableInfoProvider(
//       bmqst::Table                              *table,
//       bmqst::BasicTableInfoProvider              *basicTableInfoProvider,
//       const bmqst::StatValue::SnapshotLocation&  startSnapshot,
//       const bmqst::StatValue::SnapshotLocation&  endSnapshot);
// ------------------------------------------------------------------------
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // The method
    // 'CountingAllocator::configureStatContextTableInfoProvider' allocates
    // using the default allocator (specifically, because of calls to
    // 'bmqst::TableSchema::addColumn', and that allocation is outside of
    // the control of this function.

    bmqtst::TestHelper::printTestName("configureStatContextTableInfoProvider"
                                      " - part 2");

    bmqst::Table                  table(bmqtst::TestHelperUtil::allocator());
    bmqst::BasicTableInfoProvider basicTableInfoProvider(
        bmqtst::TestHelperUtil::allocator());
    bmqst::StatValue::SnapshotLocation start;
    bmqst::StatValue::SnapshotLocation end;

    bmqma::CountingAllocator::configureStatContextTableInfoProvider(
        &table,
        &basicTableInfoProvider,
        start,
        end);

    BMQTST_ASSERT_EQ(basicTableInfoProvider.hasTitle(), false);
    BMQTST_ASSERT_EQ(basicTableInfoProvider.numHeaderLevels(), 1);
    BMQTST_ASSERT_EQ(basicTableInfoProvider.numRows(), 0);
    BMQTST_ASSERT_EQ(static_cast<size_t>(basicTableInfoProvider.numColumns(0)),
                     k_NUM_COLS2);

    BMQTST_ASSERT_EQ(table.numRows(), 0);
    BMQTST_ASSERT_EQ(static_cast<size_t>(table.numColumns()), k_NUM_COLS2);
    for (int i = 0; i < table.numColumns() - 1; ++i) {
        PV(i << ": " << table.columnName(i));
        BMQTST_ASSERT_EQ_D(i, table.columnName(i), k_COLS2[i]);
    }
}

BSLA_MAYBE_UNUSED
static void testN1_performance_allocation()
// ------------------------------------------------------------------------
// PERFORMANCE - allocation (microbenchmark)
//
// Concerns:
//   Make sure the extra book-keeping in bmqma::CountingAllocator for each
//   allocation are does not impact performance.
//
// Plan:
//   1. Run a microbenchmark, making about 1 million allocations for sizes
//      starting at 1 up to (and including) 8192, repeatedly, for
//      bmqma::CountingAllocator and bslma::Default::defaultAllocator.
//
// Testing:
//   Compare allocation performance of bmqma::CountingAllocator vs
//   bslma::Default::defaultAllocator.
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // We're microbenching against the default allocator.

    bmqtst::TestHelper::printTestName("PERFORMANCE - allocation"
                                      " (microbenchmark)");

    const size_t k_MILLION = 1000000;

    const size_t k_MAX_ALLOC_SIZE  = 8192;  // 8 KiB == 2^13 B
    const size_t k_NUM_ALLOCATIONS = k_MAX_ALLOC_SIZE *
                                     (k_MILLION / k_MAX_ALLOC_SIZE);

    bsl::vector<void*> buffers(bmqtst::TestHelperUtil::allocator());
    buffers.resize(k_NUM_ALLOCATIONS);

    bsls::Types::Int64 timeCountingAlloc = 0;
    {
        PV("--------------------------------");
        PV("bmqma::CountingAllocator");
        PV("--------------------------------");

        bmqst::StatContextConfiguration config(
            "test",
            bmqtst::TestHelperUtil::allocator());
        bmqst::StatContext parentStatContext(
            config,
            bmqtst::TestHelperUtil::allocator());
        bmqma::CountingAllocator countingAlloc("CountingAlloc",
                                               &parentStatContext,
                                               bslma::Default::allocator());

        bslma::Allocator* alloc = &countingAlloc;

        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (size_t step = 0; step < k_NUM_ALLOCATIONS;
             step += k_MAX_ALLOC_SIZE) {
            for (size_t currSize = 0; currSize < k_MAX_ALLOC_SIZE;
                 ++currSize) {
                buffers[step + currSize] = alloc->allocate(currSize);
            }
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        timeCountingAlloc = end - start;
        bsl::cout << "    " << k_NUM_ALLOCATIONS << " allocations in "
                  << bmqu::PrintUtil::prettyTimeInterval(end - start)
                  << " (average of " << timeCountingAlloc / k_NUM_ALLOCATIONS
                  << " nano seconds per call).\n";

        for (size_t i = 0; i < k_NUM_ALLOCATIONS; ++i) {
            alloc->deallocate(buffers[i]);
        }
    }

    bsls::Types::Int64 timeDefaultAlloc = 0;
    {
        PV("--------------------------------");
        PV("bslma::Default::defaultAllocator");
        PV("--------------------------------");
        bslma::Allocator* alloc = bslma::Default::defaultAllocator();

        bsls::Types::Int64 start = bsls::TimeUtil::getTimer();
        for (size_t step = 0; step < k_NUM_ALLOCATIONS;
             step += k_MAX_ALLOC_SIZE) {
            for (size_t currSize = 0; currSize < k_MAX_ALLOC_SIZE;
                 ++currSize) {
                buffers[step + currSize] = alloc->allocate(currSize);
            }
        }
        bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        timeDefaultAlloc = end - start;
        bsl::cout << "    " << k_NUM_ALLOCATIONS << " allocations in "
                  << bmqu::PrintUtil::prettyTimeInterval(end - start)
                  << " (average of " << timeDefaultAlloc / k_NUM_ALLOCATIONS
                  << " nano seconds per call).\n";

        for (size_t i = 0; i < k_NUM_ALLOCATIONS; ++i) {
            alloc->deallocate(buffers[i]);
        }
    }

    PV("");
    bsl::cout << "TIME(bmqma::CountingAllocator) "
              << "/ TIME(bslma::Default::defaultALlocator)" << ": "
              << (static_cast<double>(timeCountingAlloc) /
                  static_cast<double>(timeDefaultAlloc))
              << '\n';
}

#ifdef BSLS_PLATFORM_OS_LINUX
static void
testN1_bslmaperformance_allocation_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE - allocation (microbenchmark)
//
// Concerns:
//   Make sure the extra book-keeping in bmqma::CountingAllocator for each
//   allocation are does not impact performance.
//
// Plan:
//   1. Run a microbenchmark, making about 1 million allocations for sizes
//      starting at 1 up to (and including) 8192, repeatedly, for
//      bmqma::CountingAllocator.
//
// Testing:
//   Allocation performance of bmqma::CountingAllocator
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // We're microbenching against the default allocator.

    bmqtst::TestHelper::printTestName("PERFORMANCE - allocation"
                                      " (microbenchmark)");

    const size_t k_MILLION = 1000000;

    const size_t k_MAX_ALLOC_SIZE  = 8192;  // 8 KiB == 2^13 B
    const size_t k_NUM_ALLOCATIONS = k_MAX_ALLOC_SIZE *
                                     (k_MILLION / k_MAX_ALLOC_SIZE);

    bsl::vector<void*> buffers(bmqtst::TestHelperUtil::allocator());
    buffers.resize(k_NUM_ALLOCATIONS);
    {
        PV("--------------------------------");
        PV("bslma::Default::defaultAllocator");
        PV("--------------------------------");
        bslma::Allocator* alloc = bslma::Default::defaultAllocator();

        for (auto _ : state) {
            for (size_t step = 0; step < k_NUM_ALLOCATIONS;
                 step += k_MAX_ALLOC_SIZE) {
                for (size_t currSize = 0; currSize < k_MAX_ALLOC_SIZE;
                     ++currSize) {
                    buffers[step + currSize] = alloc->allocate(currSize);
                }
            }
        }
        for (size_t i = 0; i < k_NUM_ALLOCATIONS; ++i) {
            alloc->deallocate(buffers[i]);
        }
    }
}

static void
testN1_defaultperformance_allocation_GoogleBenchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// PERFORMANCE - allocation (microbenchmark)
//
// Concerns:
//   Make sure the extra book-keeping in bmqma::CountingAllocator for each
//   allocation are does not impact performance.
//
// Plan:
//   1. Run a microbenchmark, making about 1 million allocations for sizes
//      starting at 1 up to (and including) 8192, repeatedly, for
//      bslma::Default::defaultAllocator.
//
// Testing:
//   Allocation performance of bslma::Default::defaultAllocator.
{
    bmqtst::TestHelperUtil::ignoreCheckDefAlloc() = true;
    // We're microbenching against the default allocator.

    bmqtst::TestHelper::printTestName("PERFORMANCE - allocation"
                                      " (microbenchmark)");

    const size_t k_MILLION = 1000000;

    const size_t k_MAX_ALLOC_SIZE  = 8192;  // 8 KiB == 2^13 B
    const size_t k_NUM_ALLOCATIONS = k_MAX_ALLOC_SIZE *
                                     (k_MILLION / k_MAX_ALLOC_SIZE);

    bsl::vector<void*> buffers(bmqtst::TestHelperUtil::allocator());
    buffers.resize(k_NUM_ALLOCATIONS);

    {
        PV("--------------------------------");
        PV("bmqma::CountingAllocator");
        PV("--------------------------------");

        bmqst::StatContextConfiguration config(
            "test",
            bmqtst::TestHelperUtil::allocator());
        bmqst::StatContext parentStatContext(
            config,
            bmqtst::TestHelperUtil::allocator());
        bmqma::CountingAllocator countingAlloc("CountingAlloc",
                                               &parentStatContext,
                                               bslma::Default::allocator());

        bslma::Allocator* alloc = &countingAlloc;

        for (auto _ : state) {
            for (size_t step = 0; step < k_NUM_ALLOCATIONS;
                 step += k_MAX_ALLOC_SIZE) {
                for (size_t currSize = 0; currSize < k_MAX_ALLOC_SIZE;
                     ++currSize) {
                    buffers[step + currSize] = alloc->allocate(currSize);
                }
            }
            for (size_t i = 0; i < k_NUM_ALLOCATIONS; ++i) {
                alloc->deallocate(buffers[i]);
            }
        }
    }
}
#endif
//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_configureStatContextTableInfoProvider_part2(); break;
    case 6: test6_configureStatContextTableInfoProvider_part1(); break;
    case 5: test5_allocationLimitHierarchical(); break;
    case 4: test4_allocationLimit(); break;
    case 3: test3_deallocate(); break;
    case 2: test2_allocate(); break;
    case 1: test1_breathingTest(); break;
    case -1:
#ifdef BSLS_PLATFORM_OS_LINUX
        BENCHMARK(testN1_defaultperformance_allocation_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        BENCHMARK(testN1_bslmaperformance_allocation_GoogleBenchmark)
            ->Unit(benchmark::kMillisecond);
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        testN1_performance_allocation();
#endif
        break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}

// ---------------------------------------------------------------------------
// NOTICE:
