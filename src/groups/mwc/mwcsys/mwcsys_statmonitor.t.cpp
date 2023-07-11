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

// mwcsys_statmonitor.t.cpp                                           -*-C++-*-
#include <mwcsys_statmonitor.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <bdlf_bind.h>
#include <bdlt_timeunitratio.h>
#include <bsl_functional.h>
#include <bslmt_barrier.h>
#include <bslmt_condition.h>
#include <bslmt_qlock.h>
#include <bslmt_semaphore.h>
#include <bslmt_threadgroup.h>
#include <bslmt_threadutil.h>
#include <bslmt_turnstile.h>
#include <bsls_assert.h>
#include <bsls_asserttest.h>
#include <bsls_atomic.h>
#include <bsls_macrorepeat.h>
#include <bsls_platform.h>
#include <bsls_stopwatch.h>

// SYS
#include <stdint.h>

#if defined(BSLS_PLATFORM_OS_DARWIN)
#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/thread_act.h>
#endif  // BSLS_PLATFORM_OS_DARWIN

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

// TYPES
#if defined(BSLS_PLATFORM_OS_LINUX) || defined(BSLS_PLATFORM_OS_CYGWIN)
typedef bsls::Platform::OsLinux OsType;
#elif defined(BSLS_PLATFORM_OS_FREEBSD)
typedef bsls::Platform::OsFreeBsd OsType;
#elif defined(BSLS_PLATFORM_OS_DARWIN)
typedef bsls::Platform::OsDarwin OsType;
#elif defined(BSLS_PLATFORM_OS_WINDOWS)
typedef bsls::Platform::OsWindows OsType;
#else  // UNIX is the most reasonable default
typedef bsls::Platform::OsUnix OsType;
#endif

// CLASSES

// =========================
// struct TestCollectionUtil
// =========================

struct TestCollectionUtil {
#if defined(BSLS_PLATFORM_OS_DARWIN)
    static bslmt::QLock s_collectionLock;
#endif

    template <class PLATFORM>
    static void collectAndPrint()
    {
        PVVV_SAFE("Per-thread collection not enabled on this platform");
    }
};

#if defined(BSLS_PLATFORM_OS_DARWIN)

bslmt::QLock TestCollectionUtil::s_collectionLock = BSLMT_QLOCK_INITIALIZER;

template <>
void TestCollectionUtil::collectAndPrint<bsls::Platform::OsDarwin>()
{
    bslmt::QLockGuard lockGuard(&s_collectionLock);

    mach_port_t   thread;
    kern_return_t kr;
    uint64_t      tid = -1;

    thread = ::mach_thread_self();

    // Thread ID
    thread_identifier_info_data_t idInfo;
    mach_msg_type_number_t        idInfoCount = THREAD_IDENTIFIER_INFO_COUNT;
    kr                                        = ::thread_info(thread,
                       THREAD_IDENTIFIER_INFO,
                       reinterpret_cast<thread_info_t>(&idInfo),
                       &idInfoCount);
    if (kr == KERN_SUCCESS) {
        PVV_SAFE("TID: " << idInfo.thread_id);
        tid = idInfo.thread_id;
    }

    // Thread basic info
    thread_basic_info_data_t basicInfo;
    mach_msg_type_number_t   basicInfoCount = THREAD_BASIC_INFO_COUNT;
    kr                                      = ::thread_info(thread,
                       THREAD_BASIC_INFO,
                       reinterpret_cast<thread_info_t>(&basicInfo),
                       &basicInfoCount);

    if (kr == KERN_SUCCESS && (basicInfo.flags & TH_FLAGS_IDLE) == 0) {
        PRINT_SAFE("THREAD ID [" << tid << "]");
        PRINT_SAFE_("    ");
        P(basicInfo.user_time.seconds);
        PRINT_SAFE_("    ");
        P(basicInfo.user_time.microseconds);
        PRINT_SAFE_("    ");
        P(basicInfo.system_time.seconds);
        PRINT_SAFE_("    ");
        P(basicInfo.system_time.microseconds);
        PRINT_SAFE_("    ");
        P(basicInfo.cpu_usage);
        PRINT_SAFE_("    ");
        P(basicInfo.suspend_count);
    }
}

#endif  // BSLS_PLATFORM_OS_DARWIN

// FUNCTIONS
int doWork_threadFunction_CpuIntensive()
{
    const int k_1K = 1000;

    int                   multiplier = 1;
    volatile unsigned int sum        = 0;
    for (int i = 0; i < (k_1K * multiplier); ++i) {
        volatile int a = 50 * 9873 / 3;
        sum += a;

        volatile int b = a * (a ^ (~0U));
        sum += b;

        volatile int c = (a * (b ^ (~0U))) % 999983;
        sum += c;
    }

    return sum;
}

void doWork_threadFunction(bslmt::Barrier* turnsFinishedBarrier,
                           double          rate,
                           double          duration)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(turnsFinishedBarrier);

    bsls::Stopwatch timer;
    timer.start();

    bslmt::Turnstile turnstile(rate);

    int sum = 0;
    while (true) {
        turnstile.waitTurn();
        if (timer.elapsedTime() >= duration) {
            break;  // BREAK
        }
        int tmpSum = doWork_threadFunction_CpuIntensive();
        sum        = tmpSum * sum;
    }

    // Print per-thread statistics (if supported)
    TestCollectionUtil::collectAndPrint<OsType>();

    turnsFinishedBarrier->wait();
}

void doWorkAndPostWhenDone(bslmt::Semaphore* workDoneSemaphore)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(workDoneSemaphore);

    const int k_NUM_THREADS = 3;

    // Multithreaded calculation
    bslmt::ThreadGroup threadGroup(s_allocator_p);
    bslmt::Barrier     workDoneBarrier(k_NUM_THREADS + 1);

    for (int i = 0; i < k_NUM_THREADS; ++i) {
        int rc = threadGroup.addThread(
            bdlf::BindUtil::bindS(s_allocator_p,
                                  &doWork_threadFunction,
                                  &workDoneBarrier,
                                  10000,  // rate
                                  3));    // duration
        BSLS_ASSERT_OPT(rc == 0);
    }

    // Wait for all threads to finish and then join on all
    workDoneBarrier.wait();
    threadGroup.joinAll();

    workDoneSemaphore->post();
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
//   Exercise the basic functionality of the component.
//
// Plan:
//   - Construct a StatsMonitor object and verify accessors and undefined
//     behavior invariants in the default state
//
// Testing:
//   - Basic functionality
//   - Constructor
// ------------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("BREATHING TEST");

    s_ignoreCheckDefAlloc = true;
    // 'snapshot' passes a string using default alloc

    PVV("Constructor");
    {
        const int k_HISTORY_SIZE = 5;

        mwcu::MemOutStream  errorDesc(s_allocator_p);
        mwcsys::StatMonitor obj(k_HISTORY_SIZE, s_allocator_p);

        ASSERT(obj.isStarted() == false);
        ASSERT(obj.statContext() != 0);
        ASSERT_SAFE_FAIL(obj.snapshot());

        ASSERT_SAFE_PASS(obj.uptime());
        ASSERT_SAFE_FAIL(obj.cpuSystem(0));
        ASSERT_SAFE_FAIL(obj.cpuUser(0));
        ASSERT_SAFE_FAIL(obj.cpuAll(0));
        ASSERT_SAFE_FAIL(obj.memResident(0));
        ASSERT_SAFE_FAIL(obj.memVirtual(0));
        ASSERT_SAFE_FAIL(obj.minorPageFaults(0));
        ASSERT_SAFE_FAIL(obj.majorPageFaults(0));
        ASSERT_SAFE_FAIL(obj.numSwaps(0));
        ASSERT_SAFE_FAIL(obj.voluntaryContextSwitches(0));
        ASSERT_SAFE_FAIL(obj.involuntaryContextSwitches(0));
    }
}

static void test2_start()
// ------------------------------------------------------------------------
// START
//
// Concerns:
//   - Taking snapshots is disabled before starting the monitor and
//     enabled after it.
//   - Merely starting the monitor preserves the stats at baseline values.
//   - Calls to 'snapshot' should always succeed after 'start' was called
//     (without a following 'stop')
//
// Plan:
//   1. Construct a StatsMonitor object and ensure verify that cannot take
//      snapshot.
//   2. Start the monitor and verify that the stats remain at baseline
//      values.
//   3. Verify ability to take snapshots, repeatedly.
//
// Testing:
//   - start
//   - Constructor
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // 'start' passes a string using default alloc

    mwctst::TestHelper::printTestName("START");

    const int k_HISTORY_SIZE = 5;

    mwcu::MemOutStream  errorDesc(s_allocator_p);
    mwcsys::StatMonitor obj(k_HISTORY_SIZE, s_allocator_p);

    // Given
    BSLS_ASSERT_OPT(obj.isStarted() == false);
    BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(obj.snapshot());

    ASSERT_EQ(obj.start(errorDesc), 0);
    ASSERT_EQ(obj.isStarted(), true);
    ASSERT_SAFE_PASS(obj.snapshot());

    ASSERT_GE(obj.uptime(), 0);
    ASSERT_GE(obj.cpuSystem(0), 0.0);
    ASSERT_GE(obj.cpuUser(0), 0.0);
    ASSERT_GE(obj.cpuAll(0), 0.0);
    ASSERT_GE(obj.memResident(0), 0LL);
    ASSERT_GE(obj.memVirtual(0), 0LL);
    ASSERT_GE(obj.minorPageFaults(0), 0LL);
    ASSERT_GE(obj.majorPageFaults(0), 0LL);
    ASSERT_GE(obj.numSwaps(0), 0LL);
    ASSERT_GE(obj.voluntaryContextSwitches(0), 0LL);
    ASSERT_GE(obj.involuntaryContextSwitches(0), 0LL);

    // After 'start' was called, calls to 'snapshot' should always succeed
    ASSERT_SAFE_PASS(obj.snapshot());
}

static void test3_stop()
// ------------------------------------------------------------------------
// STOP
//
// Concerns:
//   - Stopping the monitor clears the stats to their baseline values and
//     disables the capacity to take snapshots.
//   - Should be able to start the monitor again after having stopped it.
//
// Plan:
//   1. Start a StatMonitor object and then stop it. Verify that stats are
//      are at baseline values and capacity to take snapshots was disabled.
//      snapshot.
//   2. Start the monitor again and verify that it succeeded.
//
// Testing:
//   - stop
// ------------------------------------------------------------------------
{
    s_ignoreCheckDefAlloc = true;
    // 'start' passes a string using default alloc

    mwctst::TestHelper::printTestName("STOP");

    const int k_HISTORY_SIZE = 5;

    mwcu::MemOutStream  errorDesc(s_allocator_p);
    mwcsys::StatMonitor obj(k_HISTORY_SIZE, s_allocator_p);

    // Given
    BSLS_ASSERT_OPT(obj.start(errorDesc) == 0);
    BSLS_ASSERT_OPT(obj.isStarted() == true);
    // Ensure accessors are created
    ASSERT_SAFE_PASS(obj.snapshot());

    ASSERT_GE(obj.uptime(), 0);
    ASSERT_GE(obj.cpuSystem(0), 0.0);
    ASSERT_GE(obj.cpuUser(0), 0.0);
    ASSERT_GE(obj.cpuAll(0), 0.0);
    ASSERT_GE(obj.memResident(0), 0LL);
    ASSERT_GE(obj.memVirtual(0), 0LL);
    ASSERT_GE(obj.minorPageFaults(0), 0LL);
    ASSERT_GE(obj.majorPageFaults(0), 0LL);
    ASSERT_GE(obj.numSwaps(0), 0LL);
    ASSERT_GE(obj.voluntaryContextSwitches(0), 0LL);
    ASSERT_GE(obj.involuntaryContextSwitches(0), 0LL);

    // Stop and verify that the stats are at baseline values
    obj.stop();

    ASSERT_EQ(obj.isStarted(), false);
    ASSERT_GE(obj.uptime(), 0.0);
    ASSERT_GE(obj.cpuSystem(0), 0.0);
    ASSERT_GE(obj.cpuUser(0), 0.0);
    ASSERT_GE(obj.cpuAll(0), 0.0);
    ASSERT_GE(obj.memResident(0), 0LL);
    ASSERT_GE(obj.memVirtual(0), 0LL);
    ASSERT_GE(obj.minorPageFaults(0), 0LL);
    ASSERT_GE(obj.majorPageFaults(0), 0LL);
    ASSERT_GE(obj.numSwaps(0), 0LL);
    ASSERT_GE(obj.voluntaryContextSwitches(0), 0LL);
    ASSERT_GE(obj.involuntaryContextSwitches(0), 0LL);

    // Verify that capacity to take snapshots is disabled
    ASSERT_SAFE_FAIL(obj.snapshot());

    // Should be able to start after stopping
    ASSERT_EQ(obj.start(errorDesc), 0);
    ASSERT_EQ(obj.isStarted(), true);
}

static void test4_snapshot()
// ------------------------------------------------------------------------
// SNAPSHOT
//
// Concerns:
//   - When snapshots are taken, stat values change in accordance with
//     their contract specifications.
//
// Testing:
//   - snapshot
// ------------------------------------------------------------------------
{
    // MACROS
#define PVV_CPU_USER_N(ID) PVV("cpuUser(" << ID << "): " << obj.cpuUser(ID));
#define PVV_CPU_USER(HEADER)                                                  \
    PVV(HEADER);                                                              \
    PVV_CPU_USER_N(0);                                                        \
    BSLS_MACROREPEAT(3, PVV_CPU_USER_N);
#define PVV_CPU_SYSTEM_N(ID)                                                  \
    PVV("cpuSystem(" << ID << "): " << obj.cpuSystem(ID));
#define PVV_CPU_SYSTEM(HEADER)                                                \
    PVV(HEADER);                                                              \
    PVV_CPU_SYSTEM_N(0);                                                      \
    BSLS_MACROREPEAT(3, PVV_CPU_SYSTEM_N);
#define PVV_MEM_VIRTUAL_N(ID)                                                 \
    PVV("memVirtual(" << ID << "): " << obj.memVirtual(ID));
#define PVV_MEM_VIRTUAL(HEADER)                                               \
    PVV(HEADER);                                                              \
    PVV_MEM_VIRTUAL_N(0);                                                     \
    BSLS_MACROREPEAT(3, PVV_MEM_VIRTUAL_N);
#define PVV_INV_CONTEXT_SWITCHES_N(ID)                                        \
    PVV("involuntaryContextSwitches("                                         \
        << ID << "): " << obj.involuntaryContextSwitches(ID));
#define PVV_INV_CONTEXT_SWITCHES(HEADER)                                      \
    PVV(HEADER);                                                              \
    PVV_INV_CONTEXT_SWITCHES_N(0);                                            \
    BSLS_MACROREPEAT(3, PVV_INV_CONTEXT_SWITCHES_N);

    s_ignoreCheckDefAlloc = true;
    // 'start' passes a string using default alloc

    mwctst::TestHelper::printTestName("SNAPSHOT");

    const int k_HISTORY_SIZE = 5;

    mwcu::MemOutStream  errorDesc(s_allocator_p);
    mwcsys::StatMonitor obj(k_HISTORY_SIZE, s_allocator_p);

    // Given
    int rc = obj.start(errorDesc);
    BSLS_ASSERT_OPT(rc == 0);
    BSLS_ASSERT_OPT(obj.isStarted() == true);

    bslmt::Semaphore          workDoneSemaphore;
    bslmt::ThreadUtil::Handle handle;
    bslmt::ThreadAttributes   attributes;
    attributes.setDetachedState(bslmt::ThreadAttributes::e_CREATE_DETACHED);

    // Take snapshots and verify expected values
    PVV("----------------");
    PVV("  1ST SNAPSHOT  ");
    PVV("----------------");
    bslmt::ThreadUtil::createWithAllocator(
        &handle,
        attributes,
        bdlf::BindUtil::bindS(s_allocator_p,
                              &doWorkAndPostWhenDone,
                              &workDoneSemaphore),
        s_allocator_p);

    bslmt::ThreadUtil::sleep(
        bsls::TimeInterval(0, 1 * bdlt::TimeUnitRatio::k_NS_PER_MS));
    // Sleep for 1 ms to force a voluntary context switch to happen (we
    // could just 'yield' but when there are no other running processes on
    // the machine, 'yield' may be no-op).

    workDoneSemaphore.wait();
    obj.snapshot();
    PVV_CPU_USER("-");
    PVV_CPU_SYSTEM("-");
    PVV_MEM_VIRTUAL("-");
    PVV_INV_CONTEXT_SWITCHES("-");

    // Broker Uptime
    bsls::Types::Int64 uptime1 = obj.uptime();
    ASSERT_GE(uptime1, 0);

    // Average of CPU over last snapshot yields non-zero values
    ASSERT_GE(obj.cpuSystem(1), 0);
    ASSERT_GE(obj.cpuUser(1), 0);
    ASSERT_GE(obj.cpuAll(1), 0);

    // latest snapshot's memory
    ASSERT_GT(obj.memResident(0), 0LL);
    ASSERT_GT(obj.memVirtual(0), 0LL);

    // No relation between value(1) and value(0)
    ASSERT_GE(obj.minorPageFaults(1), 0);
    ASSERT_GE(obj.majorPageFaults(1), 0);
    ASSERT_GE(obj.numSwaps(1), 0);
    ASSERT_GE(obj.voluntaryContextSwitches(1), 0);
    ASSERT_GE(obj.involuntaryContextSwitches(1), 0);

    PVV("----------------");
    PVV("  2ND SNAPSHOT  ");
    PVV("----------------");
    bslmt::ThreadUtil::createWithAllocator(
        &handle,
        attributes,
        bdlf::BindUtil::bindS(s_allocator_p,
                              &doWorkAndPostWhenDone,
                              &workDoneSemaphore),
        s_allocator_p);
    workDoneSemaphore.wait();
    BSLS_ASSERTTEST_ASSERT_OPT_PASS(obj.snapshot());

    // Make sure the broker uptime is monotonically increasing
    bsls::Types::Int64 uptime2 = obj.uptime();
    ASSERT_GE(uptime2, uptime1);

    PVV_CPU_USER("-");
    PVV_CPU_SYSTEM("-");
    PVV_MEM_VIRTUAL("-");
    PVV_INV_CONTEXT_SWITCHES("-");

    PVV("----------------");
    PVV("  3RD SNAPSHOT  ");
    PVV("----------------");
    bslmt::ThreadUtil::createWithAllocator(
        &handle,
        attributes,
        bdlf::BindUtil::bindS(s_allocator_p,
                              &doWorkAndPostWhenDone,
                              &workDoneSemaphore),
        s_allocator_p);
    workDoneSemaphore.wait();
    BSLS_ASSERTTEST_ASSERT_OPT_PASS(obj.snapshot());

    // Make sure the broker uptime is monotonically increasing
    bsls::Types::Int64 uptime3 = obj.uptime();
    ASSERT_GE(uptime3, uptime2);

    PVV_CPU_USER("-");
    PVV_CPU_SYSTEM("-");
    PVV_MEM_VIRTUAL("-");
    PVV_INV_CONTEXT_SWITCHES("-");

#undef PVV_CPU_USER_N
#undef PVV_CPU_USER
#undef PVV_CPU_SYSTEM
#undef PVV_CPU_SYSTEM_N
#undef PVV_MEM_VIRTUAL_N
#undef PVV_MEM_VIRTUAL
#undef PVV_INV_CONTEXT_SWITCHES_N
#undef PVV_INV_CONTEXT_SWITCHES
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 4: test4_snapshot(); break;
    case 3: test3_stop(); break;
    case 2: test2_start(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
