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
#include <bdlma_localsequentialallocator.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all dispatcher's statistics)
static const char k_DISPATCHER_STAT_NAME[] = "dispatcher";

}  // close unnamed namespace

// -----------------
// class DomainStats
// -----------------

bsls::Types::Int64 DispatcherStats::getValue(const bmqst::StatContext& context,
                                         int                       snapshotId,
                                         const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const bmqst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const bmqst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

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
        oldestSnapshot)

    switch (stat) {
    case Stat::e_ENQUEUE_DELTA: {
        return STAT_RANGE(incrementsDifference, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_DEQUEUE_DELTA: {
        return STAT_RANGE(decrementsDifference, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_SIZE: {
        return STAT_SINGLE(value, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_SIZE_MAX: {
        return STAT_RANGE(rangeMax, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_SIZE_ABS_MAX: {
        return STAT_SINGLE_ABS(absoluteMax, DispatcherStatsIndex::e_STAT_QUEUE);
    }
    case Stat::e_TIME_MIN: {
        return STAT_RANGE(rangeMin, DispatcherStatsIndex::e_STAT_TIME);
    }
    case Stat::e_TIME_AVG: {
        return STAT_RANGE(averagePerEvent, DispatcherStatsIndex::e_STAT_TIME);
    }
    case Stat::e_TIME_MAX: {
        return STAT_RANGE(rangeMax, DispatcherStatsIndex::e_STAT_TIME);
    }
    case Stat::e_TIME_ABS_MAX: {
        return STAT_SINGLE_ABS(absoluteMax, DispatcherStatsIndex::e_STAT_TIME);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE_ABS
#undef STAT_SINGLE
}

// ---------------------
// class DomainStatsUtil
// ---------------------

bsl::shared_ptr<bmqst::StatContext>
DispatcherStatsUtil::initializeStatContext(int               historySize,
                                           bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<1024> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_DISPATCHER_STAT_NAME, &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("queued_count")
        .value("queued_time", bmqst::StatValue::e_DISCRETE);

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

bslma::ManagedPtr<bmqst::StatContext>
    DispatcherStatsUtil::initializeSubStatContext(bmqst::StatContext*      parent,
                                const bslstl::StringRef& name,
                                bslma::Allocator*        allocator)
{
    bdlma::LocalSequentialAllocator<512> localAllocator(allocator);

    bmqst::StatContextConfiguration statConfig(name, &localAllocator);
    // statConfig.value("queued_count")
    //           .value("queued_time", bmqst::StatValue::e_DISCRETE);

    // bslma::ManagedPtr<bmqst::StatContext> statContext = parent->addSubcontext(statConfig);

    // return statContext;
    return parent->addSubcontext(statConfig);
}                                

}  // close package namespace
}  // close enterprise namespace
