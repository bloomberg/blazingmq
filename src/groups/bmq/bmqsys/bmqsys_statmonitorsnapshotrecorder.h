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

// bmqsys_statmonitorsnapshotrecorder.h                               -*-C++-*-
#ifndef INCLUDED_BMQSYS_STATMONITORSNAPSHOTRECORDER
#define INCLUDED_BMQSYS_STATMONITORSNAPSHOTRECORDER

//@PURPOSE: Provide a mechanism to record snapshot of time, cpu, ctx switch.
//
//@CLASSES:
//  bmqsys::StatMonitorSnapshotRecorder: Mechanism to record snapshot of stats
//
//@DESCRIPTION: 'bmqsys::StatMonitorSnapshotRecorder' provides a mechanism to
// record and report a snapshot of wall time, average CPU utilization
// percentage (%) for user, system, and their sum, and voluntary context
// switches.  The measurements recorded and reported by this object are
// described below.
//
//..
// Measure                                Description
// -------                                -----------
// Wall time                              Amount of real time elapsed since
//                                        previous print.
//
// User CPU %                             Average percentage of elapsed CPU
//                                        time this process spent executing
//                                        instructions in user mode.
//
// System CPU %                           Average percentage of elapsed CPU
//                                        time this process spent executing
//                                        instructions in kernel mode.
//
// CPU %                                  Sum of the average User CPU % and
//                                        System CPU times.
//
// Voluntary Context Switches             Total number of voluntary context
//                                        switches incurred since beginning to
//                                        record.
//..
//
/// Thread Safety
///-------------
// NOT Thread-Safe.
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Example 1: Monitoring and logging stats for an expensive recovery operation
///  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following code illustrates how to monitor and log stats throughout an
// expensive recovery operation.
//
// In this example, the 'bmqsys::StatMonitorSnapshotRecorder' is created on the
// stack before beginning recovery, with a 'header' string that will be used as
// the top-level header in its output.  Then the first step of recovery is
// initiated, and when it is finished, a snapshot of the stats recorded is
// logged using the 'print' method.  Note that this also resets the recording
// of stats.  Next, the second step of recovery begins, and finally, when it
// completes, the stat recorder is again used to log a snapshot of the stats
// using the 'print' method.
//
//..
//  int
//  FileStore::recoverMessages(...)
//  {
//      ...
//      bmqsys::StatMonitorSnapshotRecorder statRecorder(
//                                                    "Cluster (testCluster)");
//      // Perform step 1 of recovery
//      ...
//
//      // Log stats for step 1 of recovery (also resets the recording of
//      // stats)
//      BALL_LOG_INFO_BLOCK {
//          statRecorder.print(BALL_LOG_OUTPUT_STREAM, "RECOVERY - STEP 1");
//      }
//
//      // Perform step 2 of recovery
//      ...
//
//      // Log stats for step 2 of recovery
//      BALL_LOG_INFO_BLOCK {
//          statRecorder.print(BALL_LOG_OUTPUT_STREAM, "RECOVERY - STEP 2");
//      }
//      ...
//  }
//..

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_types.h>

namespace BloombergLP {

namespace bmqsys {

// =================================
// class StatMonitorSnapshotRecorder
// =================================

/// Provide a mechanism to record and report a snapshot of wall time,
/// average cpu utilization percentage (%) for user, system, and their sum,
/// and voluntary context switches.
class StatMonitorSnapshotRecorder {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQSYS.STATMONITORSNAPSHOTRECORDER");

  private:
    // DATA
    struct Impl;
    mutable bsl::shared_ptr<Impl> d_impl_sp;  // pimpl (for efficiency and ease
                                              // of copying in async operations
                                              // where an object may be created
                                              // on the stack and needed later
                                              // in a callback)

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatMonitorSnapshotRecorder,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `bmqsys::StatMonitorSnapshotRecorder` using the
    /// specified `header`, used as the top-level header when printing
    /// statistics, and begin recording statistics.  Use the optionally
    /// specified `allocator` to supply memory.
    explicit StatMonitorSnapshotRecorder(const bsl::string& header,
                                         bslma::Allocator*  allocator = 0);

    /// Copy constructor, create a new `bmqsys::StatMonitorSnapshotRecorder`
    /// having the same values as the specified `other`, and using the
    /// optionally specified `allocator`.
    StatMonitorSnapshotRecorder(
        const bmqsys::StatMonitorSnapshotRecorder& other,
        bslma::Allocator*                          allocator = 0);

    // MANIPULATORS

    /// Print a snapshot of statistics, recorded since the object was
    /// instantiated or last printed (whichever is most recent), to the
    /// specified `os` using the `header` provided at construction as the
    /// header of the output and the specified `section` as the sub-header
    /// of the output.  The format of the recording is printed as follows:
    ///
    ///   <header><section> (Total elapsed: 50.83s):
    ///       WALL TIME ...................: 21.25 s
    ///       CPU USER AVG ................: 51.526 %
    ///       CPU SYSTEM AVG ..............: 18.351 %
    ///       CPU ALL AVG .................: 69.878 %
    ///       VOLUNTARY CONTEXT SWITCHES ..: 4,377
    void print(bsl::ostream& os, const bsl::string& section) const;

    // ACCESSORS

    /// Return the total time elapsed, in nanoseconds, since creation of
    /// this object.
    bsls::Types::Int64 totalElapsed() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
