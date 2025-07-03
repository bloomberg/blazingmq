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

// bmqsys_statmonitor.h                                               -*-C++-*-
#ifndef INCLUDED_BMQSYS_STATMONITOR
#define INCLUDED_BMQSYS_STATMONITOR

//@PURPOSE: Provide a mechanism to monitor cpu, memory, and os stats.
//
//@CLASSES:
//  bmqsys::StatMonitor    : Mechanism to monitor cpu, memory, os stats.
//  bmqsys::StatMonitorUtil: Utility to access monitor cpu, memory, os stats.
//
//@DESCRIPTION: 'bmqsys::StatMonitor' provides a mechanism that takes snapshots
// of the cpu, memory usage, and context switches of the current process and
// provides accessors to those values over a range of historical snapshots.
// The last 'maxSnapshots' (from the constructor parameter) values are kept,
// meaning that values could be inspected over a range of at most
// 'maxSnapshots': [1, maxSnapshots].  The system keeps track of the average
// cpu (user, system and all) and the maximum memory (resident and virtual)
// used as reported by the 'balb::PerformanceMonitor'.  It also tracks page
// faults (minor and major) and context switches (voluntary and involuntary) as
// reported by the 'getrusage' system call.  'bmqsys::StatMonitorUtil' provides
// a utility namespace for static accessors that allow access of system
// statistics from a stat context.
//..
// Measure                                Description
// -------                                -----------
// User CPU Time                          Average amount of time spent
//                                        executing instructions in user mode.
//
// System CPU Time                        Average amount of time spent
//                                        executing instructions in kernel
//                                        mode.
//
// Process Uptime                         Total amount of time (in seconds)
//                                        elapsed since the process was
//                                        started.
//
// All CPU Time                           Sum of the average User and System
//                                        CPU times.
//
// Resident Size                          Maximum value of resident memory
//                                        (in bytes) used by the process.
//
// Virtual Size                           The maximum value of virtual memory
//                                        (bytes on the heap) used by this
//                                        process.  This value does not include
//                                        the size of the address space mapped
//                                        to files (anonymous or otherwise.)
//
// Major Page Faults                      Total number of major page faults
//                                        incurred throughout the lifetime of
//                                        the process.
//
// Minor Page Faults                      Total number of minor page faults
//                                        incurred throughout the lifetime of
//                                        the process.
//
// Voluntary Context Switches             Total number of voluntary context
//                                        switches incurred throughout the
//                                        lifetime of the process.
//
// Involuntary Context Switches           Total number of involuntary context
//                                        switches incurred throughout the
//                                        lifetime of the process.
//
//..
//
/// StatContext
///-----------
// Statistics are kept in a 'bmqst::StatContext' having a root level named
// 'System', and three subcontexts 'cpu', 'mem', and 'os'.
//
/// Thread Safety
///-------------
// NOT Thread-Safe.

#include <bmqst_statcontext.h>

// BDE
#include <balb_performancemonitor.h>
#include <ball_log.h>
#include <bsl_ostream.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqsys {

// =================
// class StatMonitor
// =================

/// Provide a mechanism to monitor cpu, memory, and operating system stats.
/// Values of these data points are recorded at every `snapshot`, for a
/// maximum of `historySize` historical snapshots at any given time.  A new
/// snapshot is taken every time the `snapshot` method is called on an
/// object of this class.  The latest snapshot is always recorded at
/// `snapshotId == 0`, and at each call to `snapshot`, the id of the oldest
/// historical snapshot is incremented by one, starting at zero and
/// increasing monotonically until reaching the `historySize` provided at
/// construction.
class StatMonitor {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQSYS.STATMONITOR");

  private:
    // DATA
    balb::PerformanceMonitor d_performanceMonitor;
    // Performance Monitor

    int d_pid;
    // PID of the current process

    bmqst::StatContext d_systemStatContext;
    // The root statistic context

    bslma::ManagedPtr<bmqst::StatContext> d_cpuStatContext_mp;
    // StatContext for cpu stats

    bslma::ManagedPtr<bmqst::StatContext> d_memStatContext_mp;
    // StatContext for memory stats

    bslma::ManagedPtr<bmqst::StatContext> d_osStatContext_mp;
    // StatContext for operating system
    // stats ('os' == operating system)

    bool d_isStarted;
    // Flag indicating if this StatMonitor
    // object was successfully started via
    // a call to 'start'

  private:
    // NOT IMPLEMENTED
    StatMonitor(const StatMonitor& other) BSLS_KEYWORD_DELETED;
    StatMonitor& operator=(const StatMonitor& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatMonitor, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `StatMonitor` holding up the to specified `historySize`
    /// snapshots of historical values, and using the specified `allocator`.
    StatMonitor(int historySize, bslma::Allocator* allocator);

    /// Stop and destroy this object.
    ~StatMonitor();

    // MANIPULATORS

    /// Start the StatMonitor. Return 0 on success, or a non-zero return
    /// code on error and fill in the specified `errorDescription` stream
    /// with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop this component.
    void stop();

    /// Return the top-level stat context.
    bmqst::StatContext* statContext();

    /// Take a snapshot of the system stats.  The behavior is undefined if
    /// `start` was not called (or failed).
    void snapshot();

    // ACCESSORS

    /// Return true if this StatMonitor object was successfully started via
    /// a call to `start`, and false otherwise.
    bool isStarted() const;

    /// Return the total amount of time (in seconds) elapsed since the
    /// process was started.
    bsls::Types::Int64 uptime() const;

    /// Return the average cpu system value over the period starting from
    /// the specified `snapshotId` snapshots ago to the latest snapshot
    /// (i.e., at `snapshotId == 0`).  Note that passing in 'snapshotId ==
    /// 0' returns 0 (empty interval).  Returned value is the average of the
    /// individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    double cpuSystem(int snapshotId) const;

    /// Return the average cpu user value over the period starting from the
    /// specified `snapshotId` snapshots ago to the latest snapshot (i.e.,
    /// at `snapshotId == 0`).  Note that passing in `snapshotId == 0`
    /// returns 0 (empty interval).  Returned value is the average of the
    /// individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    double cpuUser(int snapshotId) const;

    /// Return the average cpu all (user + system) value over the period
    /// starting from the specified `snapshotId` snapshots ago to the latest
    /// snapshot (i.e., at `snapshotId == 0`).  Note that passing in
    /// `snapshotId == 0` returns 0 (empty interval).  Returned value is the
    /// average of the individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    double cpuAll(int snapshotId) const;

    /// Return the maximum value of resident memory (in bytes) snapshot-ed
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///  max(value[0, snapshotId])
    bsls::Types::Int64 memResident(int snapshotId) const;

    /// Return the maximum value of virtual memory (bytes on the heap)
    /// snapshot-ed between the specified `snapshotId` snapshots ago and the
    /// latest snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///   max(value[0, snapshotId])
    bsls::Types::Int64 memVirtual(int snapshotId) const;

    /// Return the number of minor page faults that occurred between the
    /// specified `snapshotId` snapshots ago and the latest snapshot (i.e.,
    /// at `snapshotId == 0`), inclusive.
    ///
    ///   difference(value[0], value[snapshotId])
    bsls::Types::Int64 minorPageFaults(int snapshotId) const;

    /// Return the number of major page faults that occurred between the
    /// specified `snapshotId` snapshots ago and the latest snapshot (i.e.,
    /// at `snapshotId == 0`), inclusive.
    ///
    ///   difference(value[0], value[snapshotId])
    bsls::Types::Int64 majorPageFaults(int snapshotId) const;

    /// Return the number of swaps that occurred between the specified
    /// `snapshotId` snapshots ago and the latest snapshot (i.e., at
    /// `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    bsls::Types::Int64 numSwaps(int snapshotId) const;

    /// Return the number of voluntary context switches that occurred
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    bsls::Types::Int64 voluntaryContextSwitches(int snapshotId) const;

    /// Return the number of involuntary context switches that occurred
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    bsls::Types::Int64 involuntaryContextSwitches(int snapshotId) const;
};

// ======================
// struct StatMonitorUtil
// ======================

/// Provide a utility object that allows access to system statistics stored
/// in a stat context.
struct StatMonitorUtil {
  private:
    // PRIVATE CLASS METHODS

    /// Utility function to get the top-level system stat for the specified
    /// `statContext` corresponding to the specified `statId` over the
    /// period starting from the specified `snapshotId` snapshots ago to the
    /// latest snapshot (i.e., at `snapshotId == 0`) snapshot-ed over [0,
    /// snapshotId]:
    ///
    ///   max(value[0, snapshotId])
    static bsls::Types::Int64
    getSystemStat(const bmqst::StatContext& statContext,
                  int                       snapshotId,
                  int                       statId);

    /// Utility function to get the cpu stat for the specified `statContext`
    /// corresponding to the specified `statId` over the period starting
    /// from the specified `snapshotId` snapshots ago to the latest snapshot
    /// (i.e., at `snapshotId == 0`).  Note that passing in 'snapshotId ==
    /// 0' returns 0 (empty interval).  Returned value is the average of the
    /// individual values snapshot-ed over [0, snapshotId].
    ///
    ///   avg(value[0, snapshotId])
    static double getCpuStat(const bmqst::StatContext& statContext,
                             int                       snapshotId,
                             int                       statId);

    /// Utility function to get the memory stat for the specified
    /// `statContext` corresponding to the specified `statId` over the
    /// period starting from the specified `snapshotId` snapshots ago to the
    /// latest snapshot (i.e., at `snapshotId == 0`) snapshot-ed over [0,
    /// snapshotId]:
    ///
    ///   max(value[0, snapshotId])
    static bsls::Types::Int64 getMemStat(const bmqst::StatContext& statContext,
                                         int                       snapshotId,
                                         int                       statId);

    /// Utility function to get the operating system stat for the specified
    /// `statContext` corresponding to the specified `statId` as the
    /// difference between the latest snapshot-ed value (i.e., 'snapshotId
    /// == 0') and the value that was recorded at the specified `snapshotId`
    /// snapshots ago.
    ///
    ///   difference(value[0], value[snapshotId]).
    static bsls::Types::Int64
    getOperatingSystemStat(const bmqst::StatContext& statContext,
                           int                       snapshotId,
                           int                       statId);

  public:
    // CLASS METHODS

    /// Return the amount of time (in seconds) the process has been running.
    /// Note that since the uptime increases monotonically, the snapshotId
    /// should be equal to zero.
    static bsls::Types::Int64 uptime(const bmqst::StatContext& statContext,
                                     int                       snapshotId = 0);

    /// Return the average cpu system value over the period starting from
    /// the specified `snapshotId` snapshots ago to the latest snapshot
    /// (i.e., at `snapshotId == 0`).  Note that passing in 'snapshotId ==
    /// 0' returns 0 (empty interval).  Returned value is the average of the
    /// individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    static double cpuSystem(const bmqst::StatContext& statContext,
                            int                       snapshotId);

    /// Return the average cpu user value over the period starting from the
    /// specified `snapshotId` snapshots ago to the latest snapshot (i.e.,
    /// at `snapshotId == 0`).  Note that passing in `snapshotId == 0`
    /// returns 0 (empty interval).  Returned value is the average of the
    /// individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    static double cpuUser(const bmqst::StatContext& statContext,
                          int                       snapshotId);

    /// Return the average cpu all (user + system) value over the period
    /// starting from the specified `snapshotId` snapshots ago to the latest
    /// snapshot (i.e., at `snapshotId == 0`).  Note that passing in
    /// `snapshotId == 0` returns 0 (empty interval).  Returned value is the
    /// average of the individual values snapshot-ed over [0, snapshotId]:
    ///
    ///   avg(value[0, snapshotId])
    static double cpuAll(const bmqst::StatContext& statContext,
                         int                       snapshotId);

    /// Return the maximum value of resident memory (in bytes) snapshot-ed
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///  max(value[0, snapshotId])
    static bsls::Types::Int64
    memResident(const bmqst::StatContext& statContext, int snapshotId);

    /// Return the maximum value of virtual memory (bytes on the heap)
    /// snapshot-ed between the specified `snapshotId` snapshots ago and the
    /// latest snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///   max(value[0, snapshotId])
    static bsls::Types::Int64 memVirtual(const bmqst::StatContext& statContext,
                                         int                       snapshotId);

    /// Return the number of minor page faults that occurred between the
    /// specified `snapshotId` snapshots ago and the latest snapshot (i.e.,
    /// at `snapshotId == 0`), inclusive.
    ///
    ///   difference(value[0], value[snapshotId])
    static bsls::Types::Int64
    minorPageFaults(const bmqst::StatContext& statContext, int snapshotId);

    /// Return the number of major page faults that occurred between the
    /// specified `snapshotId` snapshots ago and the latest snapshot (i.e.,
    /// at `snapshotId == 0`), inclusive.
    ///
    ///   difference(value[0], value[snapshotId])
    static bsls::Types::Int64
    majorPageFaults(const bmqst::StatContext& statContext, int snapshotId);

    /// Return the number of swaps that occurred between the specified
    /// `snapshotId` snapshots ago and the latest snapshot (i.e., at
    /// `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    static bsls::Types::Int64 numSwaps(const bmqst::StatContext& statContext,
                                       int                       snapshotId);

    /// Return the number of voluntary context switches that occurred
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    static bsls::Types::Int64
    voluntaryContextSwitches(const bmqst::StatContext& statContext,
                             int                       snapshotId);

    /// Return the number of involuntary context switches that occurred
    /// between the specified `snapshotId` snapshots ago and the latest
    /// snapshot (i.e., at `snapshotId == 0`), inclusive.
    ///
    ///    difference(value[0], value[snapshotId]).
    static bsls::Types::Int64
    involuntaryContextSwitches(const bmqst::StatContext& statContext,
                               int                       snapshotId);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class StatMonitor
// -----------------

// ACCESSORS
inline bool StatMonitor::isStarted() const
{
    return d_isStarted;
}

inline bsls::Types::Int64 StatMonitor::uptime() const
{
    return StatMonitorUtil::uptime(d_systemStatContext);
}

inline double StatMonitor::cpuSystem(int snapshotId) const
{
    return StatMonitorUtil::cpuSystem(d_systemStatContext, snapshotId);
}

inline double StatMonitor::cpuUser(int snapshotId) const
{
    return StatMonitorUtil::cpuUser(d_systemStatContext, snapshotId);
}

inline double StatMonitor::cpuAll(int snapshotId) const
{
    return StatMonitorUtil::cpuAll(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64 StatMonitor::memResident(int snapshotId) const
{
    return StatMonitorUtil::memResident(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64 StatMonitor::memVirtual(int snapshotId) const
{
    return StatMonitorUtil::memVirtual(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64 StatMonitor::minorPageFaults(int snapshotId) const
{
    return StatMonitorUtil::minorPageFaults(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64 StatMonitor::majorPageFaults(int snapshotId) const
{
    return StatMonitorUtil::majorPageFaults(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64 StatMonitor::numSwaps(int snapshotId) const
{
    return StatMonitorUtil::numSwaps(d_systemStatContext, snapshotId);
}

inline bsls::Types::Int64
StatMonitor::voluntaryContextSwitches(int snapshotId) const
{
    return StatMonitorUtil::voluntaryContextSwitches(d_systemStatContext,
                                                     snapshotId);
}

inline bsls::Types::Int64
StatMonitor::involuntaryContextSwitches(int snapshotId) const
{
    return StatMonitorUtil::involuntaryContextSwitches(d_systemStatContext,
                                                       snapshotId);
}

}  // close package namespace
}  // close enterprise namespace

#endif
