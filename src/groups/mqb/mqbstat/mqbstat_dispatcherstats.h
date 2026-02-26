// Copyright 2026 Bloomberg Finance L.P.
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

// mqbstat_dispatcherstats.h                                          -*-C++-*-
#ifndef INCLUDED_MQBSTAT_DISPATCHERSTATS
#define INCLUDED_MQBSTAT_DISPATCHERSTATS

//@PURPOSE: Provide mechanism to keep track of Dispatcher statistics.
//
//@CLASSES:
//  mqbstat::DispatcherStats: Mechanism to maintain stats of a dispatcher
//  mqbstat::DispatcherStatsUtil: Utilities to initialize statistics
//
//@DESCRIPTION: 'mqbstat::DispatcherStats' provides a mechanism to keep track
// of dispatcher level statistics.  'mqbstat::DispatcherStatsUtil' is a utility
// namespace exposing methods to initialize the stat contexts.

// BMQ
#include <bmqst_statcontext.h>

// BDE
#include <bsl_memory.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbstat {

// =====================
// class DispatcherStats
// =====================

/// Mechanism to keep track of individual overall statistics of a dispatcher
class DispatcherStats {
  public:
    // TYPES

    /// Enum representing the various type of stats that can be obtained
    /// from this object.
    struct Stat {
        // TYPES
        enum Enum {
            e_ENQUEUE_DELTA      = 0,
            e_DEQUEUE_DELTA      = 1,
            e_QUEUE_SIZE         = 2,
            e_QUEUE_SIZE_MAX     = 3,
            e_QUEUE_SIZE_ABS_MAX = 4,
            e_QUEUE_TIME_MIN     = 5,
            e_QUEUE_TIME_AVG     = 6,
            e_QUEUE_TIME_MAX     = 7,
            e_QUEUE_TIME_ABS_MAX = 8
        };
    };

    // CLASS METHODS

    /// Get the value of the specified `stat` reported to the dispatcher
    /// represented by its associated specified `context` as the difference
    /// between the latest snapshot-ed value (i.e., `snapshotId == 0`) and
    /// the value that was recorded at the specified `snapshotId` snapshots
    /// ago.
    ///
    /// THREAD: This method can only be invoked from the `snapshot` thread.
    static bsls::Types::Int64 getValue(const bmqst::StatContext& context,
                                       int                       snapshotId,
                                       Stat::Enum                stat);

    /// Update the `queued_count` field of the specified `queueStatContext`.
    static void onEnqueue(bmqst::StatContext* queueStatContext);

    /// Update the `queued_count` and `queued_time` fields of the specified
    /// `queueStatContext`.
    static void onDequeue(bmqst::StatContext* queueStatContext,
                          bsls::Types::Int64  queuedTime);

  private:
    // PRIVATE TYPES

    /// Namespace for the constants of stat values that applies to the
    /// dispatcher queues from the clients.
    struct DispatcherStatsIndex {
        enum Enum {
            e_STAT_QUEUE = 0,  // Queue/Dequeue
            e_STAT_TIME  = 1   // Event queued time
        };
    };

    // NOT IMPLEMENTED
    DispatcherStats(const DispatcherStats&) BSLS_CPP11_DELETED;

    /// Copy constructor and assignment operator are not implemented.
    DispatcherStats& operator=(const DispatcherStats&) BSLS_CPP11_DELETED;
};

// ==========================
// struct DispatcherStatsUtil
// ==========================

/// Utility namespace of methods to initialize dispatcher stats.
struct DispatcherStatsUtil {
    // CLASS METHODS

    /// Initialize the statistics for the dispatcher stat context, keeping the
    /// specified `historySize` of history.  Return the created top level
    /// stat context to use for all dispatcher level statistics.  Use the
    /// specified `allocator` for all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeStatContext(int historySize, bslma::Allocator* allocator);

    /// Initialize the statistics for the dispatcher client stat context,
    /// with the specified `parent` context and `name`.
    /// Return the created stat context to use for all dispatcher client
    /// level statistics.  Use the specified `allocator` for all
    /// stat context and stat values.
    static bslma::ManagedPtr<bmqst::StatContext>
    initializeClientStatContext(bmqst::StatContext* parent,
                                bsl::string_view    name,
                                bslma::Allocator*   allocator);

    /// Initialize the statistics for the dispatcher queue stat context,
    /// with the specified `parent` context `name`, `client`, and
    /// `processorId`. Return the created stat context to use for all
    /// dispatcher queue level statistics.  Use the specified `allocator` for
    /// all stat context and stat values.
    static bsl::shared_ptr<bmqst::StatContext>
    initializeQueueStatContext(bmqst::StatContext* parent,
                               bsl::string_view    name,
                               bsl::string_view    client,
                               unsigned int        processorId,
                               bslma::Allocator*   allocator);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class DispatcherStats
// ---------------------

inline void DispatcherStats::onEnqueue(bmqst::StatContext* queueStatContext)
{
    BSLS_ASSERT_SAFE(queueStatContext && "Stat context is not initialized");

    queueStatContext->adjustValue(DispatcherStatsIndex::e_STAT_QUEUE, 1);
}

inline void DispatcherStats::onDequeue(bmqst::StatContext* queueStatContext,
                                       bsls::Types::Int64  queuedTime)
{
    BSLS_ASSERT_SAFE(queueStatContext && "Stat context is not initialized");

    queueStatContext->adjustValue(DispatcherStatsIndex::e_STAT_QUEUE, -1);
    queueStatContext->reportValue(DispatcherStatsIndex::e_STAT_TIME,
                                  queuedTime);
}

}  // close package namespace
}  // close enterprise namespace

#endif
