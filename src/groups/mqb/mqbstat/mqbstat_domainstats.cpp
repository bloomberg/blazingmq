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

// mqbstat_domainstats.cpp                                            -*-C++-*-
#include <mqbstat_domainstats.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqt_uri.h>

// MQB
#include <mqbi_cluster.h>
#include <mqbi_domain.h>

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

/// Name of the stat context to create (holding all domain's statistics)
static const char k_DOMAIN_STAT_NAME[] = "domains";

//------------------------
// struct DomainStatsIndex
//------------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients
struct DomainStatsIndex {
    enum Enum { e_STAT_CFG_MSGS, e_STAT_CFG_BYTES, e_STAT_QUEUE_COUNT };
};

}  // close unnamed namespace

// ------------------
// class DomainStats
// ------------------

bsls::Types::Int64 DomainStats::getValue(const mwcst::StatContext& context,
                                         int                       snapshotId,
                                         const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const mwcst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const mwcst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot,                                                       \
        oldestSnapshot)

    switch (stat) {
    case Stat::e_CFG_MSGS: {
        return STAT_SINGLE(value, DomainStatsIndex::e_STAT_CFG_MSGS);
    }
    case Stat::e_CFG_BYTES: {
        return STAT_SINGLE(value, DomainStatsIndex::e_STAT_CFG_BYTES);
    }
    case Stat::e_QUEUE_COUNT: {
        return STAT_RANGE(rangeMax, DomainStatsIndex::e_STAT_QUEUE_COUNT);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown Stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

DomainStats::DomainStats()
: d_statContext_mp(0)
{
    // NOTHING
}

void DomainStats::initialize(mqbi::Domain*       domain,
                             mwcst::StatContext* domainStatContext,
                             bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize was already called");

    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);
    d_statContext_mp = domainStatContext->addSubcontext(
        mwcst::StatContextConfiguration(domain->name(), &localAllocator));

    // Build a dummy queue URI so we can use 'bmqt::Uri' to extract the
    // qualified domain without resorting on implementation details.
    bmqt::Uri uri;
    int       rc = bmqt::UriBuilder(&localAllocator)
                 .setQualifiedDomain(domain->name())
                 .setQueue("dummy")
                 .uri(&uri);
    BSLS_ASSERT_OPT(rc == 0);

    bslma::Allocator* alloc = d_statContext_mp->datumAllocator();

    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bdld::DatumMapBuilder                 builder(alloc);

    builder.pushBack("cluster",
                     bdld::Datum::copyString(domain->cluster()->name(),
                                             alloc));
    builder.pushBack("domain", bdld::Datum::copyString(uri.domain(), alloc));
    builder.pushBack("tier",
                     bdld::Datum::copyString(uri.tier().isEmpty() ? ""
                                                                  : uri.tier(),
                                             alloc));

    datum->adopt(builder.commit());
}

void DomainStats::onEvent(EventType::Enum type, bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    switch (type) {
    case EventType::e_CFG_MSGS: {
        d_statContext_mp->setValue(DomainStatsIndex::e_STAT_CFG_MSGS, value);
    } break;
    case EventType::e_CFG_BYTES: {
        d_statContext_mp->setValue(DomainStatsIndex::e_STAT_CFG_BYTES, value);
    } break;
    case EventType::e_QUEUE_COUNT: {
        d_statContext_mp->setValue(DomainStatsIndex::e_STAT_QUEUE_COUNT,
                                   value);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

// ---------------------
// class DomainStatsUtil
// ---------------------

bsl::shared_ptr<mwcst::StatContext>
DomainStatsUtil::initializeStatContext(int               historySize,
                                       bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_DOMAIN_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("cfg_msgs")
        .value("cfg_bytes")
        .value("queue_count");

    return bsl::shared_ptr<mwcst::StatContext>(
        new (*allocator) mwcst::StatContext(config, allocator),
        allocator);
}

}  // close package namespace
}  // close enterprise namespace
