// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbstat_brokerstats.cpp                                            -*-C++-*-
#include <mqbstat_brokerstats.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>

// MWC
#include <mwcst_statcontext.h>
#include <mwcst_statutil.h>
#include <mwcst_statvalue.h>

// BDE
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all broker's statistics)
static const char k_BROKER_STAT_NAME[] = "broker";

//------------------------
// struct BrokerStatsIndex
//------------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients
struct BrokerStatsIndex {
    enum Enum { e_STAT_QUEUE_COUNT, e_STAT_CLIENT_COUNT };
};

}  // close unnamed namespace

// -----------------
// class BrokerStats
// -----------------

BrokerStats BrokerStats::s_instance;

BrokerStats& BrokerStats::instance()
{
    return s_instance;
}

bsls::Types::Int64 BrokerStats::getValue(const mwcst::StatContext& context,
                                         int                       snapshotId,
                                         const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const mwcst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const mwcst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_RANGE(OPERATION, STAT)                                           \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::DMCST_DIRECT_VALUE, STAT),          \
        latestSnapshot,                                                       \
        oldestSnapshot)

    switch (stat) {
    case Stat::e_QUEUE_COUNT: {
        return STAT_RANGE(rangeMax, BrokerStatsIndex::e_STAT_QUEUE_COUNT);
    }
    case Stat::e_CLIENT_COUNT: {
        return STAT_RANGE(rangeMax, BrokerStatsIndex::e_STAT_CLIENT_COUNT);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
}

BrokerStats::BrokerStats()
: d_statContext_p(0)
{
    // NOTHING
}

void BrokerStats::initialize(mwcst::StatContext* brokerStatContext)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_statContext_p && "initialize was already called");

    d_statContext_p = brokerStatContext;
}

void BrokerStats::onEvent(EventType::Enum type)
{
    BSLS_ASSERT_SAFE(d_statContext_p && "initialize was not called");

    switch (type) {
    case EventType::e_CLIENT_CREATED: {
        d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_CLIENT_COUNT, 1);
    } break;
    case EventType::e_CLIENT_DESTROYED: {
        d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_CLIENT_COUNT,
                                     -1);
    } break;
    case EventType::e_QUEUE_CREATED: {
        d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_QUEUE_COUNT, 1);
    } break;
    case EventType::e_QUEUE_DESTROYED: {
        d_statContext_p->adjustValue(BrokerStatsIndex::e_STAT_QUEUE_COUNT, -1);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

// ---------------------
// class BrokerStatsUtil
// ---------------------

bsl::shared_ptr<mwcst::StatContext>
BrokerStatsUtil::initializeStatContext(int               historySize,
                                       bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_BROKER_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("queue_count")
        .value("client_count");

    bsl::shared_ptr<mwcst::StatContext> statContext =
        bsl::shared_ptr<mwcst::StatContext>(
            new (*allocator) mwcst::StatContext(config, allocator),
            allocator);

    BrokerStats::instance().initialize(statContext.get());

    return statContext;
}

}  // close package namespace
}  // close enterprise namespace
