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

// mqbstat_dispatcherstats.cpp                                        -*-C++-*-
#include <mqbstat_dispatcherstats.h>

// BMQ
#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>

// BDE
#include <ball_log.h>
#include <bdld_datummapbuilder.h>
#include <bdlma_localsequentialallocator.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all dispatcher's statistics)
static const char k_DISPATCHER_STAT_NAME[] = "dispatcher";

}  // close unnamed namespace

// ---------------------
// class DispatcherStats
// ---------------------

bsls::Types::Int64 DispatcherStats::getValue(const bmqst::StatContext& context,
                                             int               snapshotId,
                                             const Stat::Enum& stat)

{
    // invoked from the SNAPSHOT thread

    const bmqst::StatValue::SnapshotLocation latestSnapshot(0, 0);

#define OLDEST_SNAPSHOT(STAT)                                                 \
    (bmqst::StatValue::SnapshotLocation(                                      \
        0,                                                                    \
        (snapshotId >= 0)                                                     \
            ? snapshotId                                                      \
            : (context.value(bmqst::StatContext::e_DIRECT_VALUE, (STAT))      \
                   .historySize(0) -                                          \
               1)))

#define STAT_SINGLE(OPERATION, STAT)                                          \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot)

#define STAT_SINGLE_ABS(OPERATION, STAT)                                      \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE, STAT))

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot,                                                       \
        OLDEST_SNAPSHOT(STAT))

#define CASE_PROCESSING(EVENT_NAME)                                           \
    case Stat::e_PROCESSING_TIME_##EVENT_NAME##_MAX: {                        \
        const bsls::Types::Int64 max = STAT_RANGE(                            \
            rangeMax,                                                         \
            DispatcherStatsIndex::e_STAT_PROCESSING_TIME_##EVENT_NAME);       \
        return max == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0      \
                                                                     : max;   \
    }                                                                         \
    case Stat::e_PROCESSING_TIME_##EVENT_NAME##_AVG: {                        \
        const bsls::Types::Int64 avg = STAT_RANGE(                            \
            averagePerEvent,                                                  \
            DispatcherStatsIndex::e_STAT_PROCESSING_TIME_##EVENT_NAME);       \
        return avg == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0      \
                                                                     : avg;   \
    }                                                                         \
    case Stat::e_PROCESSING_TIME_##EVENT_NAME##_SUM: {                        \
        return STAT_RANGE(                                                    \
            sumDifference,                                                    \
            DispatcherStatsIndex::e_STAT_PROCESSING_TIME_##EVENT_NAME);       \
    }                                                                         \
    case Stat::e_PROCESSED_COUNT_##EVENT_NAME: {                              \
        return STAT_RANGE(                                                    \
            eventsDifference,                                                 \
            DispatcherStatsIndex::e_STAT_PROCESSING_TIME_##EVENT_NAME);       \
    }

    switch (stat) {
    case Stat::e_ENQUEUE_DELTA: {
        return STAT_RANGE(incrementsDifference,
                          DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_DEQUEUE_DELTA: {
        return STAT_RANGE(decrementsDifference,
                          DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_QUEUE_SIZE: {
        return STAT_SINGLE(value, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_QUEUE_SIZE_MAX: {
        return STAT_RANGE(rangeMax, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_QUEUE_SIZE_ABS_MAX: {
        return STAT_SINGLE_ABS(absoluteMax,
                               DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_QUEUE_TIME_MIN: {
        const bsls::Types::Int64 min =
            STAT_RANGE(rangeMin, DispatcherStatsIndex::e_STAT_QUEUED_TIME);
        return min == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0 : min;
    }
    case Stat::e_QUEUE_TIME_AVG: {
        const bsls::Types::Int64 avg = STAT_RANGE(
            averagePerEvent,
            DispatcherStatsIndex::e_STAT_QUEUED_TIME);
        return avg == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0 : avg;
    }
    case Stat::e_QUEUE_TIME_MAX: {
        const bsls::Types::Int64 max =
            STAT_RANGE(rangeMax, DispatcherStatsIndex::e_STAT_QUEUED_TIME);
        return max == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0 : max;
    }
    case Stat::e_QUEUE_TIME_ABS_MAX: {
        return STAT_SINGLE_ABS(absoluteMax,
                               DispatcherStatsIndex::e_STAT_QUEUED_TIME);
    }
        CASE_PROCESSING(UNDEFINED)
        CASE_PROCESSING(DISPATCHER)
        CASE_PROCESSING(CALLBACK)
        CASE_PROCESSING(CONTROL_MSG)
        CASE_PROCESSING(CONFIRM)
        CASE_PROCESSING(REJECT)
        CASE_PROCESSING(PUSH)
        CASE_PROCESSING(PUT)
        CASE_PROCESSING(ACK)
        CASE_PROCESSING(CLUSTER_STATE)
        CASE_PROCESSING(STORAGE)
        CASE_PROCESSING(RECOVERY)
        CASE_PROCESSING(REPLICATION_RECEIPT)
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef CASE_PROCESSING
#undef STAT_RANGE
#undef STAT_SINGLE_ABS
#undef STAT_SINGLE
#undef OLDEST_SNAPSHOT
}

// -------------------------
// class DispatcherStatsUtil
// -------------------------

bsl::shared_ptr<bmqst::StatContext>
DispatcherStatsUtil::initializeStatContext(int               historySize,
                                           bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_DISPATCHER_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("queued_count")
        .value("queued_time", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_undefined", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_dispatcher", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_callback", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_control_msg", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_confirm", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_reject", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_push", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_put", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_ack", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_cluster_state", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_storage", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_recovery", bmqst::StatValue::e_DISCRETE)
        .value("processing_time_replication_receipt",
               bmqst::StatValue::e_DISCRETE);

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

bslma::ManagedPtr<bmqst::StatContext>
DispatcherStatsUtil::initializeClientStatContext(bmqst::StatContext* parent,
                                                 bsl::string_view    name,
                                                 bslma::Allocator*   allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration statConfig(name, &localAllocator);
    return parent->addSubcontext(statConfig);
}

bslma::ManagedPtr<bmqst::StatContext>
DispatcherStatsUtil::initializeQueueStatContext(bmqst::StatContext* parent,
                                                bsl::string_view    name,
                                                bsl::string_view    client,
                                                unsigned int      processorId,
                                                bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration statConfig(name, &localAllocator);

    bslma::ManagedPtr<bmqst::StatContext> statContext_mp =
        parent->addSubcontext(statConfig);

    // Build a datum map containing the following values:
    //: o client: the name of client associated with the queue
    //: o processorId: the processor Id associated with the queue
    bslma::ManagedPtr<bdld::ManagedDatum> datum = statContext_mp->datum();
    bslma::Allocator*     alloc = statContext_mp->datumAllocator();
    bdld::DatumMapBuilder builder(alloc);
    builder.pushBack("client", bdld::Datum::copyString(client, alloc));
    builder.pushBack("processorId", bdld::Datum::createInteger(processorId));
    datum->adopt(builder.commit());

    return statContext_mp;
}

}  // close package namespace
}  // close enterprise namespace
