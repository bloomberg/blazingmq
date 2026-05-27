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

#include <mqbstat_tableprinter.h>

#include <mqbscm_version.h>
// MQB
#include <mqbstat_queuestats.h>

#include <bmqio_statchannelfactory.h>
#include <bmqma_countingallocator.h>
#include <bmqst_statvalue.h>
#include <bmqst_tableutil.h>
#include <bmqu_printutil.h>

// BDE
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_ctime.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

// Subcontext names
const char k_SUBCONTEXT_ALLOCATORS[] = "allocators";

}  // close unnamed namespace

// ------------------
// class TablePrinter
// ------------------

void TablePrinter::initializeTablesAndTips(int historySize)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(historySize > 0);

    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it != d_contexts.end()) {
        Context* context = it->second.get();
        bmqma::CountingAllocator::configureStatContextTableInfoProvider(
            &context->d_table,
            &context->d_tip,
            bmqst::StatValue::SnapshotLocation(0, 0),
            bmqst::StatValue::SnapshotLocation(0, 1));
        context->d_table.records().setContext(
            context->d_statContext_p->getSubcontext(k_SUBCONTEXT_ALLOCATORS));
    }

    Context* context = d_contexts["domainQueues"].get();
    QueueStatsUtil::initializeTableAndTipDomains(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["clients"].get();
    QueueStatsUtil::initializeTableAndTipClients(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["clusterNodes"].get();
    QueueStatsUtil::initializeTableAndTipClients(&context->d_table,
                                                 &context->d_tip,
                                                 historySize,
                                                 context->d_statContext_p);

    context = d_contexts["channels"].get();
    bmqst::StatValue::SnapshotLocation start(0, 0);
    bmqst::StatValue::SnapshotLocation end(0, historySize - 1);
    bmqio::StatChannelFactoryUtil::initializeStatsTable(
        &context->d_table,
        &context->d_tip,
        context->d_statContext_p,
        start,
        end);
}

TablePrinter::TablePrinter(const StatContextsMap& statContextsMap,
                           int                    historySize,
                           bslma::Allocator*      allocator)
: d_lastAllocatorSnapshot(0)
, d_contexts(allocator)
{
    // Insert all the required Contexts
    for (StatContextsMap::const_iterator it = statContextsMap.begin();
         it != statContextsMap.end();
         ++it) {
        bsl::shared_ptr<Context> contextSp;
        contextSp.createInplace(allocator);

        ContextsMap::iterator cit =
            d_contexts.insert(bsl::make_pair(it->first, contextSp)).first;
        cit->second->d_statContext_p = it->second;
    }

    if (historySize > 0) {
        initializeTablesAndTips(historySize);
    }
}

void TablePrinter::printStats(bsl::ostream& stream, int statId)
{
    if (statId >= 0) {
        bdlt::Datetime now;
        bdlt::EpochUtil::convertFromTimeT(&now, time(0));
        stream << "===== ===== ===== ===== ===== ===== ===== ===== ====="
               << " =====\n"
               << "Stats id: " << statId << " @ " << now << "\n"
               << "===== ===== ===== ===== ===== ===== ===== ===== ====="
               << " =====\n";
    }

    // DOMAINQUEUES
    stream << "\n"
           << ":::::::::: :::::::::: DOMAINQUEUES >>";
    Context* context = d_contexts["domainQueues"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CLIENTS
    stream << "\n"
           << ":::::::::: :::::::::: CLIENTS >>";
    context = d_contexts["clients"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CLUSTERS
    stream << "\n"
           << ":::::::::: :::::::::: CLUSTERS >>";
    context = d_contexts["clusterNodes"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // CHANNELS
    stream << "\n"
           << ":::::::::: :::::::::: TCP CHANNELS >>";
    context = d_contexts["channels"].get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);

    // ALLOCATORS
    stream << "\n"
           << ":::::::::: :::::::::: ALLOCATORS >>";
    ContextsMap::iterator it = d_contexts.find("allocators");
    if (it == d_contexts.end()) {
        stream << " Unavailable\n";
        return;  // RETURN
    }
    if (d_lastAllocatorSnapshot != 0) {
        stream << " Last snapshot was "
               << bmqu::PrintUtil::prettyTimeInterval(
                      bsls::TimeUtil::getTimer() - d_lastAllocatorSnapshot)
               << " ago.";
    }
    d_lastAllocatorSnapshot = bsls::TimeUtil::getTimer();

    context = it->second.get();
    context->d_table.records().update();
    bmqst::TableUtil::printTable(stream, context->d_tip);
}

}  // close package namespace
}  // close enterprise namespace
