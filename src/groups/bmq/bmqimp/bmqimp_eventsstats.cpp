// Copyright 2015-2023 Bloomberg Finance L.P.
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

// bmqimp_eventsstats.cpp                                             -*-C++-*-
#include <bmqimp_eventsstats.h>

#include <bmqscm_version.h>

#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqimp {

namespace {
/// Name of the stat context to create (holding all events statistics)
const char k_STAT_NAME[] = "events";

enum {
    k_STAT_EVENT = 0  // value = bytes ; increments = number of events
    ,
    k_STAT_MESSAGE = 1  // value = number of messages
};
}  // close unnamed namespace

// ---------------------------
// struct EventsStatsEventType
// ---------------------------

const char* EventsStatsEventType::toAscii(EventsStatsEventType::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(ACK)
        CASE(CONFIRM)
        CASE(CONTROL)
        CASE(PUSH)
        CASE(PUT)
        CASE(LAST)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -----------------
// class EventsStats
// -----------------

EventsStats::EventsStats(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_stat(allocator)
{
    // NOTHING
}

void EventsStats::initializeStats(
    bmqst::StatContext*                       rootStatContext,
    const bmqst::StatValue::SnapshotLocation& start,
    const bmqst::StatValue::SnapshotLocation& end)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    // Create the root stat context
    // ----------------------------
    bmqst::StatContextConfiguration config(k_STAT_NAME, &localAllocator);
    config.isTable(true);
    config.value("event").value("message");
    d_stat.d_statContext_mp = rootStatContext->addSubcontext(config);

    // Create the subContexts
    // ----------------------
    for (int type = 0; type < EventsStatsEventType::e_LAST; ++type) {
        const char* name = EventsStatsEventType::toAscii(
            static_cast<EventsStatsEventType::Enum>(type));
        d_statContexts_mp[type] = d_stat.d_statContext_mp->addSubcontext(
            bmqst::StatContextConfiguration(name, &localAllocator));
    }

    // Create table (with Delta stats)
    // -------------------------------
    bmqst::TableSchema& schema = d_stat.d_table.schema();
    schema.addDefaultIdColumn("id");

    schema.addColumn("messages",
                     k_STAT_MESSAGE,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("events",
                     k_STAT_EVENT,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("bytes", k_STAT_EVENT, bmqst::StatUtil::value, start);
    schema.addColumn("messages_delta",
                     k_STAT_MESSAGE,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("events_delta",
                     k_STAT_EVENT,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("bytes_delta",
                     k_STAT_EVENT,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);

    // Configure records
    bmqst::TableRecords& records = d_stat.d_table.records();
    records.setContext(d_stat.d_statContext_mp.get());
    records.setFilter(&StatUtil::filterDirectAndTopLevel);
    records.considerChildrenOfFilteredContexts(true);

    // Create the tip
    d_stat.d_tip.setTable(&d_stat.d_table);
    d_stat.d_tip.setColumnGroup("Type");
    d_stat.d_tip.addColumn("id", "").justifyLeft();

    d_stat.d_tip.setColumnGroup("delta");
    d_stat.d_tip.addColumn("messages_delta", "messages").zeroString("");
    d_stat.d_tip.addColumn("events_delta", "events").zeroString("");
    d_stat.d_tip.addColumn("bytes_delta", "bytes")
        .zeroString("")
        .printAsMemory();

    d_stat.d_tip.setColumnGroup("absolute");
    d_stat.d_tip.addColumn("messages", "messages").zeroString("");
    d_stat.d_tip.addColumn("events", "events").zeroString("");
    d_stat.d_tip.addColumn("bytes", "bytes").zeroString("").printAsMemory();

    // Create the table (without Delta stats)
    // --------------------------------------
    // We always use current snapshot for this
    bmqst::StatValue::SnapshotLocation loc(0, 0);

    bmqst::TableSchema& schemaNoDelta = d_stat.d_tableNoDelta.schema();
    schemaNoDelta.addDefaultIdColumn("id");

    schemaNoDelta.addColumn("messages",
                            k_STAT_MESSAGE,
                            bmqst::StatUtil::value,
                            loc);
    schemaNoDelta.addColumn("events",
                            k_STAT_EVENT,
                            bmqst::StatUtil::increments,
                            loc);
    schemaNoDelta.addColumn("bytes",
                            k_STAT_EVENT,
                            bmqst::StatUtil::value,
                            loc);

    // Configure records
    bmqst::TableRecords& recordsNoDelta = d_stat.d_tableNoDelta.records();
    recordsNoDelta.setContext(d_stat.d_statContext_mp.get());
    recordsNoDelta.setFilter(&StatUtil::filterDirectAndTopLevel);
    recordsNoDelta.considerChildrenOfFilteredContexts(true);

    // Create the tip
    d_stat.d_tipNoDelta.setTable(&d_stat.d_tableNoDelta);
    d_stat.d_tipNoDelta.addColumn("id", "").justifyLeft();

    d_stat.d_tipNoDelta.addColumn("messages", "messages").zeroString("");
    d_stat.d_tipNoDelta.addColumn("events", "events").zeroString("");
    d_stat.d_tipNoDelta.addColumn("bytes", "bytes")
        .zeroString("")
        .printAsMemory();
}

void EventsStats::resetStats()
{
    if (d_stat.d_statContext_mp) {
        d_stat.d_statContext_mp->clearValues();
    }
}

void EventsStats::onEvent(EventsStatsEventType::Enum type,
                          int                        eventSize,
                          int                        messageCount)
{
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!d_statContexts_mp[type])) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Stats are disabled (i.e. 'initializeStats' was not called).
        return;  // RETURN
    }

    d_statContexts_mp[type]->adjustValue(k_STAT_EVENT, eventSize);
    d_statContexts_mp[type]->adjustValue(k_STAT_MESSAGE, messageCount);
}

}  // close package namespace
}  // close enterprise namespace
