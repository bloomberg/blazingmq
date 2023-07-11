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

// mwcsys_statmonitorsnapshotrecorder.cpp                             -*-C++-*-
#include <mwcsys_statmonitorsnapshotrecorder.h>

#include <mwcscm_version.h>
// MWC
#include <mwcsys_statmonitor.h>
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bslma_default.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mwcsys {

namespace {

/// Constant for the number of snapshots holding historical values in
/// `mwcsys::StatMonitor`.  Only need to keep one snapshot back.
const bsls::Types::Int64 k_HISTORY_SIZE = 1;

}  // close anonymous namespace

// ========================================
// struct StatMonitorSnapshotRecorder::Impl
// ========================================

/// Impl class for the `StatMonitorSnapshotRecorder` data members, some of
/// which cannot be copied.  Needed to allow users of
/// `StatMonitorSnapshotRecorder` to copy an object safely by relying on a
/// copy of an internal shared pointer to the private implementation.
struct StatMonitorSnapshotRecorder::Impl {
  public:
    // DATA
    bsls::Types::Int64 d_startTimeNs;  // Start time of beginning to record,
                                       // in nanoseconds

    bsls::Types::Int64 d_lastSnapshotTimeNs;
    // Start time of this portion of
    // recording, in nanoseconds

    bsl::string d_header;  // Top-level header when printing
                           // statistics

    mwcsys::StatMonitor d_statMonitor;  // Statistics monitor (for CPU,
                                        // voluntary context switches)

    bslma::Allocator* d_allocator_p;  // Allocator used to supply memory

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(StatMonitorSnapshotRecorder::Impl,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    Impl(bsls::Types::Int64 startTimeNs,
         bsls::Types::Int64 lastSnapshotTimeNs,
         const bsl::string& header,
         bslma::Allocator*  allocator)
    : d_startTimeNs(startTimeNs)
    , d_lastSnapshotTimeNs(lastSnapshotTimeNs)
    , d_header(header, allocator)
    , d_statMonitor(k_HISTORY_SIZE, allocator)
    , d_allocator_p(allocator)
    {
        // NOTHING
    }
};

// ---------------------------------
// class StatMonitorSnapshotRecorder
// ---------------------------------

// CREATORS
StatMonitorSnapshotRecorder::StatMonitorSnapshotRecorder(
    const bsl::string& header,
    bslma::Allocator*  allocator)
: d_impl_sp()
{
    // Set the pimpl
    const bsls::Types::Int64 now   = mwcsys::Time::highResolutionTimer();
    bslma::Allocator*        alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, now, now, header, alloc);

    // Start the StatMonitor to track system stats
    bdlma::LocalSequentialAllocator<256> localAllocator(
        d_impl_sp->d_allocator_p);
    mwcu::MemOutStream errorDesc(&localAllocator);
    if (d_impl_sp->d_statMonitor.start(errorDesc) != 0) {
        // Failed to start the StatMonitor.  We log and simply continue because
        // this is not fatal.
        BALL_LOG_ERROR << "Starting the StatMonitor for '" << header
                       << "' failed.  Reason: " << errorDesc.str();
        return;  // RETURN
    }

    // snapshot now to initialize the stats
    d_impl_sp->d_statMonitor.snapshot();
}

StatMonitorSnapshotRecorder::StatMonitorSnapshotRecorder(
    const mwcsys::StatMonitorSnapshotRecorder& other,
    BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
: d_impl_sp(other.d_impl_sp)
{
    // NOTHING
}

// MANIPULATORS
void StatMonitorSnapshotRecorder::print(bsl::ostream&      os,
                                        const bsl::string& section) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    const bsls::Types::Int64 now = mwcsys::Time::highResolutionTimer();
    const bsls::Types::Int64 totalWallTimeNs = now - d_impl_sp->d_startTimeNs;
    const bsls::Types::Int64 wallTimeNs      = now -
                                          d_impl_sp->d_lastSnapshotTimeNs;

    d_impl_sp->d_lastSnapshotTimeNs = now;
    os << d_impl_sp->d_header << "'" << section << "' (Total elapsed: "
       << mwcu::PrintUtil::prettyTimeInterval(totalWallTimeNs) << ", "
       << totalWallTimeNs << " nanoseconds):\n"
       << "    WALL TIME .....................: "
       << mwcu::PrintUtil::prettyTimeInterval(wallTimeNs);

    if (!d_impl_sp->d_statMonitor.isStarted()) {
        os << "    ** SYSTEM STATS N/A ** [Reason: 'StatMonitor' not started]";
        return;  // RETURN
    }

    const int snapId = 1;
    d_impl_sp->d_statMonitor.snapshot();

    os << '\n'
       << "    CPU USER AVG ..................: "
       << d_impl_sp->d_statMonitor.cpuUser(snapId) << " %\n"
       << "    CPU SYSTEM AVG ................: "
       << d_impl_sp->d_statMonitor.cpuSystem(snapId) << " %\n"
       << "    CPU ALL AVG ...................: "
       << d_impl_sp->d_statMonitor.cpuAll(snapId) << " %\n"
       << "    VOLUNTARY CONTEXT SWITCHES ....: "
       << mwcu::PrintUtil::prettyNumber(
              d_impl_sp->d_statMonitor.voluntaryContextSwitches(snapId))
       << " \n"
       << "    INVOLUNTARY CONTEXT SWITCHES ..: "
       << mwcu::PrintUtil::prettyNumber(
              d_impl_sp->d_statMonitor.involuntaryContextSwitches(snapId))
       << " \n"
       << "    MINOR PAGE FAULTS .............: "
       << mwcu::PrintUtil::prettyNumber(
              d_impl_sp->d_statMonitor.minorPageFaults(snapId))
       << " \n"
       << "    MAJOR PAGE FAULTS .............: "
       << mwcu::PrintUtil::prettyNumber(
              d_impl_sp->d_statMonitor.majorPageFaults(snapId))
       << " \n"
       << "    NUM SWAPS .....................: "
       << mwcu::PrintUtil::prettyNumber(
              d_impl_sp->d_statMonitor.numSwaps(snapId));
}

// ACCESSORS
bsls::Types::Int64 StatMonitorSnapshotRecorder::totalElapsed() const
{
    return mwcsys::Time::highResolutionTimer() - d_impl_sp->d_startTimeNs;
}

}  // close package namespace
}  // close enterprise namespace
