// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqsys_statmonitor.cpp                                             -*-C++-*-
#include <bmqsys_statmonitor.h>

#include <bmqscm_version.h>

#include <bmqst_statutil.h>
#include <bmqst_statvalue.h>

// BDE
#include <bdlf_bind.h>
#include <bdls_processutil.h>
#include <bsl_c_errno.h>
#include <bsl_cmath.h>
#include <bsl_cstring.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>

// SYS
#include <sys/resource.h>  // for getrusage()

namespace BloombergLP {
namespace bmqsys {

namespace {

// Subcontext names

/// StatContext name of the top level statContext
const char k_STATCONTEXT_NAME[] = "system";

/// StatContext name of the CPU top level statContext
const char k_SUBCONTEXT_CPU[] = "cpu";

/// StatContext name of the MEM top level statContext
const char k_SUBCONTEXT_MEM[] = "mem";

/// StatContext name of the OPERATING-SYSTEM top level statContext
const char k_SUBCONTEXT_OS[] = "os";

/// Multiplier to use for reporting CPU stats from PerformanceMonitor into
/// statContext
const int k_CPU_MULTIPLIER = 1000;

enum {
    // Constants for the index of each stat value in their respective context

    // SYSTEM (top-level)
    k_STAT_SYSTEM_UPTIME = 0

    // CPU (in percent times k_CPU_MULTIPLIER, for example, if value is 5.13%,
    // stat will hold '5.13 * k_CPU_MULTIPLIER')
    ,
    k_STAT_CPU_SYSTEM = 0,
    k_STAT_CPU_USER   = 1,
    k_STAT_CPU_ALL    = 2

    // MEM (in bytes)
    ,
    k_STAT_MEM_RESIDENT = 0,
    k_STAT_MEM_VIRTUAL  = 1

    // OPERATING-SYSTEM (in absolute number)
    ,
    k_STAT_OS_MINOR_PAGE_FAULTS = 0
    // Number of page faults serviced without any
    // I/O activity.  In this case, I/O activity is
    // avoided by reclaiming a page frame from the
    // list of pages awaiting reallocation.

    ,
    k_STAT_OS_MAJOR_PAGE_FAULTS = 1
    // Number of page faults serviced that required
    // I/O activity

    ,
    k_STAT_OS_NUM_SWAPS = 2
    // Number of times process was swapped out of
    // main memory

    ,
    k_STAT_OS_VOLUNTARY_CTX_SWITCHES = 3
    // Number of times a context switch resulted
    // because a process voluntarily gave up the
    // processor before its time slice was completed

    ,
    k_STAT_OS_INVOLUNTARY_CTX_SWITCHES = 4
    // Number of times a context switch resulted
    // because a higher priority process ran or
    // because the current process exceeded its time
    // slice
};

typedef bsl::function<
    double(const bmqst::StatContext& statCtx, int snapshotId, int statId)>
    AccessorDouble;
typedef bsl::function<bsls::Types::Int64(const bmqst::StatContext& statCtx,
                                         int                       snapshotId,
                                         int                       statId)>
    AccessorInt64;

/// Return the queried stat considering the values from the lastest snapshot
/// to its value at `snapshotId`. The stat is extracted from a subcontext of
/// the specified `statContext` and has the specified `subCtxName` name. Use
/// the specified `accessorMethod` to extract this value. The behaviour for
/// invalid stat contexts or invalid names is undefined.
double genericAccessor(const bmqst::StatContext& statContext,
                       int                       snapshotId,
                       const bslstl::StringRef&  subCtxName,
                       int                       statId,
                       const AccessorDouble&     accessorMethod)
{
    const bmqst::StatContext* subContext = statContext.getSubcontext(
        subCtxName);
    if (!subContext) {
        BSLS_ASSERT_SAFE(false && "Invalid stat context");
        return 0;  // RETURN
    }

    return (accessorMethod)(*subContext, snapshotId, statId);
}

/// Return the queried stat considering the values from the lastest snapshot
/// to its value at `snapshotId`. The stat is extracted from a subcontext of
/// the specified `statContext` and has the specified `subCtxName` name. Use
/// the specified `accessorMethod` to extract this value. The behaviour for
/// invalid stat contexts or invalid names is undefined.
bsls::Types::Int64 genericAccessor(const bmqst::StatContext& statContext,
                                   int                       snapshotId,
                                   const bslstl::StringRef&  subCtxName,
                                   int                       statId,
                                   const AccessorInt64&      accessorMethod)
{
    const bmqst::StatContext* subContext = statContext.getSubcontext(
        subCtxName);
    if (!subContext) {
        BSLS_ASSERT_SAFE(false && "Invalid stat context");
        return 0;  // RETURN
    }

    return (accessorMethod)(*subContext, snapshotId, statId);
}

/// Return the queried stat considering the values from the lastest snapshot
/// to its value at `snapshotId`. Use the specified `accessorMethod` to
/// extract this value.  The behaviour for invalid stat contexts is
/// undefined.
bsls::Types::Int64 genericAccessor(const bmqst::StatContext& statContext,
                                   int                       snapshotId,
                                   int                       statId,
                                   const AccessorInt64&      accessorMethod)
{
    return (accessorMethod)(statContext, snapshotId, statId);
}

}  // close unnamed namespace

// -----------------
// class StatMonitor
// -----------------

// CREATORS
StatMonitor::StatMonitor(int historySize, bslma::Allocator* allocator)
: d_performanceMonitor(allocator)
, d_pid(bdls::ProcessUtil::getProcessId())
, d_systemStatContext(bmqst::StatContextConfiguration(k_STATCONTEXT_NAME,
                                                      allocator)
                          .defaultHistorySize(historySize + 1)
                          .value("uptime", bmqst::StatValue::e_CONTINUOUS),
                      allocator)
, d_cpuStatContext_mp(0)
, d_memStatContext_mp(0)
, d_osStatContext_mp(0)
, d_isStarted(false)
{
    // Create the CPU, MEM and RESOURCE-USAGE subContexts
    d_cpuStatContext_mp = d_systemStatContext.addSubcontext(
        bmqst::StatContextConfiguration(k_SUBCONTEXT_CPU, allocator)
            .value("system", bmqst::StatValue::e_DISCRETE)
            .value("user", bmqst::StatValue::e_DISCRETE)
            .value("all", bmqst::StatValue::e_DISCRETE));
    d_memStatContext_mp = d_systemStatContext.addSubcontext(
        bmqst::StatContextConfiguration(k_SUBCONTEXT_MEM, allocator)
            .value("resident", bmqst::StatValue::e_DISCRETE)
            .value("virtual", bmqst::StatValue::e_DISCRETE));
    d_osStatContext_mp = d_systemStatContext.addSubcontext(
        bmqst::StatContextConfiguration(k_SUBCONTEXT_OS, allocator)
            .value("minflt", bmqst::StatValue::e_CONTINUOUS)
            .value("majflt", bmqst::StatValue::e_CONTINUOUS)
            .value("nswap", bmqst::StatValue::e_CONTINUOUS)
            .value("nvcsw", bmqst::StatValue::e_CONTINUOUS)
            .value("nivcsw", bmqst::StatValue::e_CONTINUOUS));
}

StatMonitor::~StatMonitor()
{
    stop();
}

// MANIPULATORS
int StatMonitor::start(bsl::ostream& errorDescription)
{
    // Register process pid in performance monitor
    const int rc = d_performanceMonitor.registerPid(d_pid, "Current Process");
    if (rc != 0) {
        errorDescription << "Failed registering pid " << d_pid << " to "
                         << "performance monitor; system stat monitor will be "
                         << "disabled.";
        return rc;  // RETURN
    }

    d_isStarted = true;

    d_performanceMonitor.resetStatistics();

    return 0;
}

void StatMonitor::stop()
{
    d_isStarted = false;
    d_performanceMonitor.unregisterPid(d_pid);
    statContext()->clearValues();
}

bmqst::StatContext* StatMonitor::statContext()
{
    return &d_systemStatContext;
}

void StatMonitor::snapshot()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isStarted() && "'start' was not called");

    d_performanceMonitor.collect();
    balb::PerformanceMonitor::ConstIterator it = d_performanceMonitor.find(
        d_pid);

    // 'snapshot' should only be called if 'start' returned success, implying
    // that the pid was successfully registered.
    BSLS_ASSERT_SAFE((it != d_performanceMonitor.end()) &&
                     "'start' was not called (or failed)");

    // System: report in seconds
    d_systemStatContext.setValue(
        k_STAT_SYSTEM_UPTIME,
        static_cast<bsls::Types::Int64>(it->elapsedTime()));

    // Convenient macro to retrieve the value 'T' applying a multiplier 'M'
#define PERFVAL(T, M)                                                         \
    (static_cast<bsls::Types::Int64>(                                         \
        it->latestValue(balb::PerformanceMonitor::T) * M))

    // CPU: report CPU values * k_STAT_MULTIPLIER because statContext only
    // handles Int64, not float, so multiply by k_STAT_MULTIPLIER to keep some
    // decimal digits, and we'll divide by k_STAT_MULTIPLIER before reporting
    // the stats
    d_cpuStatContext_mp->reportValue(k_STAT_CPU_SYSTEM,
                                     PERFVAL(e_CPU_UTIL_SYSTEM,
                                             k_CPU_MULTIPLIER));
    d_cpuStatContext_mp->reportValue(k_STAT_CPU_USER,
                                     PERFVAL(e_CPU_UTIL_USER,
                                             k_CPU_MULTIPLIER));
    d_cpuStatContext_mp->reportValue(k_STAT_CPU_ALL,
                                     PERFVAL(e_CPU_UTIL, k_CPU_MULTIPLIER));

    // Memory: report in bytes
    d_memStatContext_mp->reportValue(k_STAT_MEM_RESIDENT,
                                     PERFVAL(e_RESIDENT_SIZE, 1024 * 1024));
    d_memStatContext_mp->reportValue(k_STAT_MEM_VIRTUAL,
                                     PERFVAL(e_VIRTUAL_SIZE, 1024 * 1024));

#undef PERFVAL

    struct rusage ru;
    static bool   failureLogged = false;
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(::getrusage(RUSAGE_SELF, &ru) ==
                                            0)) {
        d_osStatContext_mp->setValue(k_STAT_OS_MINOR_PAGE_FAULTS,
                                     ru.ru_minflt);
        d_osStatContext_mp->setValue(k_STAT_OS_MAJOR_PAGE_FAULTS,
                                     ru.ru_majflt);
        d_osStatContext_mp->setValue(k_STAT_OS_NUM_SWAPS, ru.ru_nswap);
        d_osStatContext_mp->setValue(k_STAT_OS_VOLUNTARY_CTX_SWITCHES,
                                     ru.ru_nvcsw);
        d_osStatContext_mp->setValue(k_STAT_OS_INVOLUNTARY_CTX_SWITCHES,
                                     ru.ru_nivcsw);
    }
    else {
        // Log getrusage() failure only once during a process's lifetime
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(failureLogged == false)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BALL_LOG_WARN << "getrusage() failure with errno: " << errno
                          << " [" << bsl::strerror(errno) << "]";
            failureLogged = true;
        }
    }

    // Snapshot the stat context
    d_systemStatContext.snapshot();
}

// ----------------------
// struct StatMonitorUtil
// ----------------------

bsls::Types::Int64
StatMonitorUtil::getSystemStat(const bmqst::StatContext& statContext,
                               int                       snapshotId,
                               int                       statId)
{
    const bmqst::StatValue& statValue =
        statContext.value(bmqst::StatContext::e_TOTAL_VALUE, statId);

    const bsls::Types::Int64 value = bmqst::StatUtil::rangeMax(
        statValue,
        bmqst::StatValue::SnapshotLocation(0, 0),
        bmqst::StatValue::SnapshotLocation(0, snapshotId));
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            value == bsl::numeric_limits<bsls::Types::Int64>::min())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return static_cast<bsls::Types::Int64>(0);  // RETURN
    }

    return value;
}

double StatMonitorUtil::getCpuStat(const bmqst::StatContext& statContext,
                                   int                       snapshotId,
                                   int                       statId)
{
    const bmqst::StatValue::SnapshotLocation firstSnapshot(0, 0);
    const bmqst::StatValue::SnapshotLocation secondSnapshot(0, snapshotId);

    const bmqst::StatValue& statValue =
        statContext.value(bmqst::StatContext::e_TOTAL_VALUE, statId);

    double value = bmqst::StatUtil::averagePerEventReal(statValue,
                                                        firstSnapshot,
                                                        secondSnapshot);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(value != value)) {
        // In floating point arithmetic (IEEE 754), if, and only if, a value is
        // NaN, the comparison '<val> != <val>' returns true.
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Nothing was ever reported
        return 0.0;  // RETURN
    }

    return value / k_CPU_MULTIPLIER;
}

bsls::Types::Int64
StatMonitorUtil::getMemStat(const bmqst::StatContext& statContext,
                            int                       snapshotId,
                            int                       statId)
{
    const bmqst::StatValue& statValue =
        statContext.value(bmqst::StatContext::e_TOTAL_VALUE, statId);

    const bsls::Types::Int64 value = bmqst::StatUtil::rangeMax(
        statValue,
        bmqst::StatValue::SnapshotLocation(0, 0),
        bmqst::StatValue::SnapshotLocation(0, snapshotId));
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            value == bsl::numeric_limits<bsls::Types::Int64>::min())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return static_cast<bsls::Types::Int64>(0);  // RETURN
    }

    return value;
}

bsls::Types::Int64
StatMonitorUtil::getOperatingSystemStat(const bmqst::StatContext& statContext,
                                        int                       snapshotId,
                                        int                       statId)
{
    const bmqst::StatValue& statValue =
        statContext.value(bmqst::StatContext::e_TOTAL_VALUE, statId);

    return bmqst::StatUtil::valueDifference(
        statValue,
        bmqst::StatValue::SnapshotLocation(0, 0),
        bmqst::StatValue::SnapshotLocation(0, snapshotId));
}

#define ACCESSOR_METHOD_SYSTEM(NAME, ID)                                      \
    bsls::Types::Int64 StatMonitorUtil::NAME(                                 \
        const bmqst::StatContext& statContext,                                \
        int                       snapshotId)                                 \
    {                                                                         \
        const AccessorInt64 accessor = bdlf::BindUtil::bind(                  \
            &StatMonitorUtil::getSystemStat,                                  \
            bdlf::PlaceHolders::_1,                                           \
            bdlf::PlaceHolders::_2,                                           \
            bdlf::PlaceHolders::_3);                                          \
        return genericAccessor(statContext, snapshotId, ID, accessor);        \
    }

#define ACCESSOR_METHOD_CPU(NAME, ID)                                         \
    double StatMonitorUtil::NAME(const bmqst::StatContext& statContext,       \
                                 int                       snapshotId)        \
    {                                                                         \
        const AccessorDouble accessor = bdlf::BindUtil::bind(                 \
            &StatMonitorUtil::getCpuStat,                                     \
            bdlf::PlaceHolders::_1,                                           \
            bdlf::PlaceHolders::_2,                                           \
            bdlf::PlaceHolders::_3);                                          \
        return genericAccessor(statContext,                                   \
                               snapshotId,                                    \
                               k_SUBCONTEXT_CPU,                              \
                               ID,                                            \
                               accessor);                                     \
    }

#define ACCESSOR_METHOD_MEM(NAME, ID)                                         \
    bsls::Types::Int64 StatMonitorUtil::NAME(                                 \
        const bmqst::StatContext& statContext,                                \
        int                       snapshotId)                                 \
    {                                                                         \
        const AccessorInt64 accessor = bdlf::BindUtil::bind(                  \
            &StatMonitorUtil::getMemStat,                                     \
            bdlf::PlaceHolders::_1,                                           \
            bdlf::PlaceHolders::_2,                                           \
            bdlf::PlaceHolders::_3);                                          \
        return genericAccessor(statContext,                                   \
                               snapshotId,                                    \
                               k_SUBCONTEXT_MEM,                              \
                               ID,                                            \
                               accessor);                                     \
    }

#define ACCESSOR_METHOD_OS(NAME, ID)                                          \
    bsls::Types::Int64 StatMonitorUtil::NAME(                                 \
        const bmqst::StatContext& statContext,                                \
        int                       snapshotId)                                 \
    {                                                                         \
        const AccessorInt64 accessor = bdlf::BindUtil::bind(                  \
            &StatMonitorUtil::getOperatingSystemStat,                         \
            bdlf::PlaceHolders::_1,                                           \
            bdlf::PlaceHolders::_2,                                           \
            bdlf::PlaceHolders::_3);                                          \
        return genericAccessor(statContext,                                   \
                               snapshotId,                                    \
                               k_SUBCONTEXT_OS,                               \
                               ID,                                            \
                               accessor);                                     \
    }

ACCESSOR_METHOD_SYSTEM(uptime, k_STAT_SYSTEM_UPTIME)

ACCESSOR_METHOD_CPU(cpuUser, k_STAT_CPU_USER)
ACCESSOR_METHOD_CPU(cpuSystem, k_STAT_CPU_SYSTEM)
ACCESSOR_METHOD_CPU(cpuAll, k_STAT_CPU_ALL)

ACCESSOR_METHOD_MEM(memResident, k_STAT_MEM_RESIDENT)
ACCESSOR_METHOD_MEM(memVirtual, k_STAT_MEM_VIRTUAL)

ACCESSOR_METHOD_OS(minorPageFaults, k_STAT_OS_MINOR_PAGE_FAULTS)
ACCESSOR_METHOD_OS(majorPageFaults, k_STAT_OS_MAJOR_PAGE_FAULTS)
ACCESSOR_METHOD_OS(numSwaps, k_STAT_OS_NUM_SWAPS)
ACCESSOR_METHOD_OS(voluntaryContextSwitches, k_STAT_OS_VOLUNTARY_CTX_SWITCHES)
ACCESSOR_METHOD_OS(involuntaryContextSwitches,
                   k_STAT_OS_INVOLUNTARY_CTX_SWITCHES)

#undef ACCESSOR_METHOD_OS
#undef ACCESSOR_METHOD_MEM
#undef ACCESSOR_METHOD_CPU
#undef ACCESSOR_METHOD_SYSTEM

}  // close package namespace
}  // close enterprise namespace
