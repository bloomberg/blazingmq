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
            e_ENQUEUE_DELTA = 0,
            e_DEQUEUE_DELTA,
            e_QUEUE_SIZE,
            e_QUEUE_SIZE_MAX,
            e_QUEUE_SIZE_ABS_MAX,
            e_QUEUE_TIME_MIN,
            e_QUEUE_TIME_AVG,
            e_QUEUE_TIME_MAX,
            e_QUEUE_TIME_ABS_MAX,
            e_PROCESSING_TIME_UNDEFINED_MAX,
            e_PROCESSING_TIME_UNDEFINED_AVG,
            e_PROCESSING_TIME_UNDEFINED_SUM,
            e_PROCESSED_COUNT_UNDEFINED,
            e_PROCESSING_TIME_DISPATCHER_MAX,
            e_PROCESSING_TIME_DISPATCHER_AVG,
            e_PROCESSING_TIME_DISPATCHER_SUM,
            e_PROCESSED_COUNT_DISPATCHER,
            e_PROCESSING_TIME_CALLBACK_MAX,
            e_PROCESSING_TIME_CALLBACK_AVG,
            e_PROCESSING_TIME_CALLBACK_SUM,
            e_PROCESSED_COUNT_CALLBACK,
            e_PROCESSING_TIME_CONTROL_MSG_MAX,
            e_PROCESSING_TIME_CONTROL_MSG_AVG,
            e_PROCESSING_TIME_CONTROL_MSG_SUM,
            e_PROCESSED_COUNT_CONTROL_MSG,
            e_PROCESSING_TIME_CONFIRM_MAX,
            e_PROCESSING_TIME_CONFIRM_AVG,
            e_PROCESSING_TIME_CONFIRM_SUM,
            e_PROCESSED_COUNT_CONFIRM,
            e_PROCESSING_TIME_REJECT_MAX,
            e_PROCESSING_TIME_REJECT_AVG,
            e_PROCESSING_TIME_REJECT_SUM,
            e_PROCESSED_COUNT_REJECT,
            e_PROCESSING_TIME_PUSH_MAX,
            e_PROCESSING_TIME_PUSH_AVG,
            e_PROCESSING_TIME_PUSH_SUM,
            e_PROCESSED_COUNT_PUSH,
            e_PROCESSING_TIME_PUT_MAX,
            e_PROCESSING_TIME_PUT_AVG,
            e_PROCESSING_TIME_PUT_SUM,
            e_PROCESSED_COUNT_PUT,
            e_PROCESSING_TIME_ACK_MAX,
            e_PROCESSING_TIME_ACK_AVG,
            e_PROCESSING_TIME_ACK_SUM,
            e_PROCESSED_COUNT_ACK,
            e_PROCESSING_TIME_CLUSTER_STATE_MAX,
            e_PROCESSING_TIME_CLUSTER_STATE_AVG,
            e_PROCESSING_TIME_CLUSTER_STATE_SUM,
            e_PROCESSED_COUNT_CLUSTER_STATE,
            e_PROCESSING_TIME_STORAGE_MAX,
            e_PROCESSING_TIME_STORAGE_AVG,
            e_PROCESSING_TIME_STORAGE_SUM,
            e_PROCESSED_COUNT_STORAGE,
            e_PROCESSING_TIME_RECOVERY_MAX,
            e_PROCESSING_TIME_RECOVERY_AVG,
            e_PROCESSING_TIME_RECOVERY_SUM,
            e_PROCESSED_COUNT_RECOVERY,
            e_PROCESSING_TIME_REPLICATION_RECEIPT_MAX,
            e_PROCESSING_TIME_REPLICATION_RECEIPT_AVG,
            e_PROCESSING_TIME_REPLICATION_RECEIPT_SUM,
            e_PROCESSED_COUNT_REPLICATION_RECEIPT,
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
                                       const Stat::Enum&         stat);

    /// Update the `queued_count` field of the specified `queueStatContext`.
    static void onEnqueue(bmqst::StatContext* queueStatContext);

    /// Update the `queued_count` and `queued_time` fields of the specified
    /// `queueStatContext`.
    static void onDequeue(bmqst::StatContext* queueStatContext,
                          bsls::Types::Int64  queuedTime);

    /// Update the `processing_time` fields of the specified `eventType`
    /// and the specified `queueStatContext`
    static void onProcess(bmqst::StatContext* queueStatContext,
                          int                 eventType,
                          bsls::Types::Int64  processedTime);

  private:
    // PRIVATE TYPES

    /// Namespace for the constants of stat values that applies to the
    /// dispatcher queues from the clients.
    struct DispatcherStatsIndex {
        enum Enum {
            e_STAT_QUEUE                 = 0,  // Queue/Dequeue
            e_STAT_QUEUED_TIME           = 1,  // Event queued time
            e_STAT_PROCESSING_TIME_START = 2,
            /// Processing time for each event type. MUST be in the same order
            /// as mqbi::DispatcherEventType
            e_STAT_PROCESSING_TIME_UNDEFINED = e_STAT_PROCESSING_TIME_START,
            e_STAT_PROCESSING_TIME_DISPATCHER,
            e_STAT_PROCESSING_TIME_CALLBACK,
            e_STAT_PROCESSING_TIME_CONTROL_MSG,
            e_STAT_PROCESSING_TIME_CONFIRM,
            e_STAT_PROCESSING_TIME_REJECT,
            e_STAT_PROCESSING_TIME_PUSH,
            e_STAT_PROCESSING_TIME_PUT,
            e_STAT_PROCESSING_TIME_ACK,
            e_STAT_PROCESSING_TIME_CLUSTER_STATE,
            e_STAT_PROCESSING_TIME_STORAGE,
            e_STAT_PROCESSING_TIME_RECOVERY,
            e_STAT_PROCESSING_TIME_REPLICATION_RECEIPT,
            e_STAT_PROCESSING_TIME_END =
                e_STAT_PROCESSING_TIME_REPLICATION_RECEIPT
        };
    };

    // PRIVATE CONSTANTS

    static const int k_EVENT_TYPES_NUMBER =
        DispatcherStatsIndex::e_STAT_PROCESSING_TIME_END -
        DispatcherStatsIndex::e_STAT_PROCESSING_TIME_START;

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
    static bslma::ManagedPtr<bmqst::StatContext>
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
    queueStatContext->reportValue(DispatcherStatsIndex::e_STAT_QUEUED_TIME,
                                  queuedTime);
}

inline void DispatcherStats::onProcess(bmqst::StatContext* queueStatContext,
                                       int                 eventType,
                                       bsls::Types::Int64  processedTime)
{
    BSLS_ASSERT_SAFE(queueStatContext && "Stat context is not initialized");
    BSLS_ASSERT_SAFE(eventType >= 0 && eventType <= k_EVENT_TYPES_NUMBER);

    queueStatContext->reportValue(
        DispatcherStatsIndex::e_STAT_PROCESSING_TIME_START + eventType,
        processedTime);
}

}  // close package namespace
}  // close enterprise namespace

#endif
