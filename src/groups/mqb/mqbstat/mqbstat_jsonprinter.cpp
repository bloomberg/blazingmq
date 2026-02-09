// Copyright 2024 Bloomberg Finance L.P.
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

// mqbstat_jsonprinter.cpp                                            -*-C++-*-
#include <mqbstat_jsonprinter.h>

#include <mqbscm_version.h>

// MQB
#include <mqbstat_queuestats.h>

#include <bmqio_statchannel.h>
#include <bmqio_statchannelfactory.h>
#include <bmqu_memoutstream.h>

#include <bmqst_statutil.h>

// BDE
#include <ball_context.h>
#include <ball_log.h>
#include <ball_logfilecleanerutil.h>
#include <ball_recordstringformatter.h>
#include <bdljsn_json.h>
#include <bdljsn_jsonutil.h>
#include <bdls_processutil.h>
#include <bdlt_iso8601util.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

const char k_LOG_CATEGORY[] = "MQBSTAT.JSONPRINTER";

// ----------------------------------
// class JsonPrettyPrinter
// ----------------------------------

/// The implementation class for JsonPrettyPrinter which incapsulates
/// printing to a stream with specified write options (e.g. compact or pretty).

class JsonPrettyPrinter {
  private:
    // DATA

    /// Output stream to print JSON to
    bsl::ostream& os;

    /// Options for printing a compact JSON
    const bdljsn::WriteOptions& opts;

    /// Delimiter in multi-object output
    const bsl::string delimiter;

  public:
    // CREATORS

    /// Create a new `JsonPrinterImpl` object, using the specified
    /// stream and the specified write options.
    explicit JsonPrettyPrinter(bsl::ostream&               os,
                               const bdljsn::WriteOptions& opts,
                               const bsl::string           delimiter)
    : os(os)
    , opts(opts)
    , delimiter(delimiter)
    {
    }

    // ACCESSORS

    /// Print the specified `json` to the output stream.
    inline void printJson(const bdljsn::Json& json) const
    {
        const int rc = bdljsn::JsonUtil::write(os, json, opts);
        BSLS_ASSERT_SAFE(0 == rc);
        os << delimiter;
    }
};
struct DomainsStatsConversionUtils {
    // PUBLIC CLASS METHODS

    /// "domainQueues" stat context:
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.

    inline static void
    populateMetric(bdljsn::JsonObject*                   obj,
                   const bmqst::StatContext&             ctx,
                   mqbstat::QueueStatsDomain::Stat::Enum metric)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(obj);

        const bsls::Types::Int64 value =
            mqbstat::QueueStatsDomain::getValue(ctx, -1, metric);

        (*obj)[mqbstat::QueueStatsDomain::Stat::toString(metric)]
            .makeNumber() = value;
    }

    inline static void populateMetrics(const JsonPrettyPrinter&  jsonPrinter,
                                       const bdljsn::Json&       parent,
                                       const bmqst::StatContext& ctx)
    {
        if (ctx.numValues() == 0) {
            // Prefer to omit an empty "values" object
            return;  // RETURN
        }

        bdljsn::Json        json(parent, parent.allocator());
        bdljsn::JsonObject& values = json.theObject();

        values.insert("type", "domain");

        typedef mqbstat::QueueStatsDomain::Stat Stat;

        populateMetric(&values, ctx, Stat::e_NB_PRODUCER);
        populateMetric(&values, ctx, Stat::e_NB_CONSUMER);

        populateMetric(&values, ctx, Stat::e_MESSAGES_CURRENT);
        populateMetric(&values, ctx, Stat::e_MESSAGES_MAX);
        populateMetric(&values, ctx, Stat::e_MESSAGES_UTILIZATION_MAX);
        populateMetric(&values, ctx, Stat::e_BYTES_CURRENT);
        populateMetric(&values, ctx, Stat::e_BYTES_MAX);
        populateMetric(&values, ctx, Stat::e_BYTES_UTILIZATION_MAX);

        populateMetric(&values, ctx, Stat::e_PUT_MESSAGES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUT_BYTES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUT_MESSAGES_ABS);
        populateMetric(&values, ctx, Stat::e_PUT_BYTES_ABS);

        populateMetric(&values, ctx, Stat::e_PUSH_MESSAGES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUSH_BYTES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUSH_MESSAGES_ABS);
        populateMetric(&values, ctx, Stat::e_PUSH_BYTES_ABS);

        populateMetric(&values, ctx, Stat::e_ACK_DELTA);
        populateMetric(&values, ctx, Stat::e_ACK_ABS);
        populateMetric(&values, ctx, Stat::e_ACK_TIME_AVG);
        populateMetric(&values, ctx, Stat::e_ACK_TIME_MAX);

        populateMetric(&values, ctx, Stat::e_NACK_DELTA);
        populateMetric(&values, ctx, Stat::e_NACK_ABS);

        populateMetric(&values, ctx, Stat::e_CONFIRM_DELTA);
        populateMetric(&values, ctx, Stat::e_CONFIRM_ABS);
        populateMetric(&values, ctx, Stat::e_CONFIRM_TIME_AVG);
        populateMetric(&values, ctx, Stat::e_CONFIRM_TIME_MAX);

        populateMetric(&values, ctx, Stat::e_REJECT_ABS);
        populateMetric(&values, ctx, Stat::e_REJECT_DELTA);

        populateMetric(&values, ctx, Stat::e_QUEUE_TIME_AVG);
        populateMetric(&values, ctx, Stat::e_QUEUE_TIME_MAX);

        populateMetric(&values, ctx, Stat::e_GC_MSGS_DELTA);
        populateMetric(&values, ctx, Stat::e_GC_MSGS_ABS);

        populateMetric(&values, ctx, Stat::e_ROLE);

        populateMetric(&values, ctx, Stat::e_CFG_MSGS);
        populateMetric(&values, ctx, Stat::e_CFG_BYTES);

        populateMetric(&values, ctx, Stat::e_NO_SC_MSGS_DELTA);
        populateMetric(&values, ctx, Stat::e_NO_SC_MSGS_ABS);

        populateMetric(&values, ctx, Stat::e_HISTORY_ABS);

        jsonPrinter.printJson(json);
    }

    inline static void populateOne(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator queueIt = ctx.subcontextIterator();
             queueIt;
             ++queueIt) {
            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("queue_name", queueIt->name());

            if (queueIt->numSubcontexts() > 0) {
                // Add metrics per appId, if any
                for (bmqst::StatContextIterator appIdIt =
                         queueIt->subcontextIterator();
                     appIdIt;
                     ++appIdIt) {
                    // Do not expect another nested StatContext within appId
                    BSLS_ASSERT_SAFE(0 == appIdIt->numSubcontexts());

                    bdljsn::Json jsonApp(parent, parent.allocator());
                    jsonApp.theObject().insert("app_id", appIdIt->name());

                    populateMetrics(jsonPrinter, jsonApp, *appIdIt);
                }
            }
            else {
                populateMetrics(jsonPrinter, json, *queueIt);
            }
        }
    }

    inline static void populateAll(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator domainIt = ctx.subcontextIterator();
             domainIt;
             ++domainIt) {
            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("domain_name", domainIt->name());

            populateOne(jsonPrinter, json, *domainIt);
        }
    }
};

struct ClientStatsConversionUtils {
    // PUBLIC CLASS METHODS

    /// "clients" stat context:
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.

    inline static void
    populateMetric(bdljsn::JsonObject*                   obj,
                   const bmqst::StatContext&             ctx,
                   mqbstat::QueueStatsClient::Stat::Enum metric)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(obj);

        const bsls::Types::Int64 value =
            mqbstat::QueueStatsClient::getValue(ctx, -1, metric);

        (*obj)[mqbstat::QueueStatsClient::Stat::toString(metric)]
            .makeNumber() = value;
    }

    inline static void populateMetrics(const JsonPrettyPrinter&  jsonPrinter,
                                       const bdljsn::Json&       parent,
                                       const bmqst::StatContext& ctx)
    {
        if (ctx.numValues() == 0) {
            // Prefer to omit an empty "values" object
            return;  // RETURN
        }

        bdljsn::Json        json(parent, parent.allocator());
        bdljsn::JsonObject& values = json.theObject();

        values.insert("type", "client");

        typedef mqbstat::QueueStatsClient::Stat Stat;

        populateMetric(&values, ctx, Stat::e_PUT_MESSAGES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUT_BYTES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUT_MESSAGES_ABS);
        populateMetric(&values, ctx, Stat::e_PUT_BYTES_ABS);

        populateMetric(&values, ctx, Stat::e_PUSH_MESSAGES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUSH_BYTES_DELTA);
        populateMetric(&values, ctx, Stat::e_PUSH_MESSAGES_ABS);
        populateMetric(&values, ctx, Stat::e_PUSH_BYTES_ABS);

        populateMetric(&values, ctx, Stat::e_ACK_DELTA);
        populateMetric(&values, ctx, Stat::e_ACK_ABS);
        populateMetric(&values, ctx, Stat::e_CONFIRM_DELTA);
        populateMetric(&values, ctx, Stat::e_CONFIRM_ABS);

        jsonPrinter.printJson(json);
    }

    inline static void populateOne(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator queueIt = ctx.subcontextIterator();
             queueIt;
             ++queueIt) {
            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("queue_name", queueIt->name());

            if (queueIt->numSubcontexts() > 0) {
                // Add metrics per appId, if any
                for (bmqst::StatContextIterator appIdIt =
                         queueIt->subcontextIterator();
                     appIdIt;
                     ++appIdIt) {
                    // Do not expect another nested StatContext within appId
                    BSLS_ASSERT_SAFE(0 == appIdIt->numSubcontexts());

                    bdljsn::Json jsonApp(parent, parent.allocator());
                    jsonApp.theObject().insert("app_id", appIdIt->name());

                    populateMetrics(jsonPrinter, jsonApp, *appIdIt);
                }
            }
            else {
                populateMetrics(jsonPrinter, json, *queueIt);
            }
        }
    }

    inline static void populateAll(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator clientIt = ctx.subcontextIterator();
             clientIt;
             ++clientIt) {
            // Populate client metrics

            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("client_name", clientIt->name());

            populateOne(jsonPrinter, json, *clientIt);
        }
    }
};

struct ChannelStatsConversionUtils {
    // PUBLIC CLASS METHODS

    /// "channels" stat context:
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.

    inline static void
    populateMetric(bdljsn::JsonObject*                       obj,
                   const bmqst::StatContext&                 ctx,
                   bmqio::StatChannelFactoryUtil::Stat::Enum metric)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(obj);

        const bsls::Types::Int64 value =
            bmqio::StatChannelFactoryUtil::getValue(ctx, -1, metric);

        (*obj)[bmqio::StatChannelFactoryUtil::Stat::toString(metric)]
            .makeNumber() = value;
    }

    inline static void
    populatePortMetrics(const JsonPrettyPrinter&  jsonPrinter,
                        const bdljsn::Json&       parent,
                        const bmqst::StatContext& ctx)
    {
        if (ctx.numValues() == 0) {
            // Prefer to omit an empty "values" object
            return;  // RETURN
        }

        bdljsn::Json        json(parent, parent.allocator());
        bdljsn::JsonObject& values = json.theObject();

        values.insert("type", "channel");

        typedef bmqio::StatChannelFactoryUtil::Stat Stat;

        populateMetric(&values, ctx, Stat::e_BYTES_IN_DELTA);
        populateMetric(&values, ctx, Stat::e_BYTES_IN_ABS);
        populateMetric(&values, ctx, Stat::e_BYTES_OUT_DELTA);
        populateMetric(&values, ctx, Stat::e_BYTES_OUT_ABS);
        populateMetric(&values, ctx, Stat::e_CONNECTIONS_DELTA);
        populateMetric(&values, ctx, Stat::e_CONNECTIONS_ABS);

        jsonPrinter.printJson(json);
    }

    inline static void populatePort(const JsonPrettyPrinter&  jsonPrinter,
                                    const bdljsn::Json&       parent,
                                    const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator xIt = ctx.subcontextIterator(); xIt;
             ++xIt) {
            // Populate channel metrics (e.g. 127.0.0.1~localhost:36160)

            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("channel_name", xIt->name());

            populatePortMetrics(jsonPrinter, json, *xIt);
        }
    }

    inline static void populateOne(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator portIt = ctx.subcontextIterator();
             portIt;
             ++portIt) {
            // Populate port metrics (e.g. 36160).
            // Port identifier is integer, thus need to convert to string.
            char portItName[64];
            sprintf(portItName, "%lld", portIt->id());

            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("port_id", portItName);
            populatePort(jsonPrinter, json, *portIt);
        }
    }

    inline static void populateAll(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx)
    {
        for (bmqst::StatContextIterator channelIt = ctx.subcontextIterator();
             channelIt;
             ++channelIt) {
            // Populate channel metrics type (e.g. remote/local)

            bdljsn::Json json(parent, parent.allocator());
            json.theObject().insert("channel_type", channelIt->name());

            populateOne(jsonPrinter, json, *channelIt);
        }
    }
};

struct AllocatorStatsConversionUtils {
    // PUBLIC CLASS METHODS

    /// "allocators" stat context:
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.

    struct Stat {
        enum Enum {
            e_NUM_ALLOCATED,
            e_NUM_ALLOCATED_DELTA,
            e_MAX_ALLOCATED,
            e_NUM_ALLOCATIONS,
            e_NUM_ALLOCATIONS_DELTA,
            e_NUM_DEALLOCATIONS,
            e_NUM_DEALLOCATIONS_DELTA
        };

        static const char* toString(Enum stat);
    };

    inline static bsls::Types::Int64
    getValue(const bmqst::StatContext& context,
             int                       snapshotId,
             Stat::Enum                stat)
    {
        const int statValueIndex = 0;

        const bmqst::StatValue::SnapshotLocation latestSnapshot(0, 0);

#define OLDEST_SNAPSHOT(STAT)                                                 \
    (bmqst::StatValue::SnapshotLocation(                                      \
        0,                                                                    \
        (snapshotId >= 0)                                                     \
            ? snapshotId                                                      \
            : (context.value(bmqst::StatContext::e_DIRECT_VALUE, (STAT))      \
                   .historySize(0) -                                          \
               1)))

#define STAT_AGGREGATED(OPERATION, STAT)                                      \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_TOTAL_VALUE, STAT))

#define STAT_SINGLE(OPERATION, STAT)                                          \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_TOTAL_VALUE, STAT),               \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_TOTAL_VALUE, STAT),               \
        latestSnapshot,                                                       \
        OLDEST_SNAPSHOT(STAT))

        switch (stat) {
        case Stat::e_NUM_ALLOCATED: {
            return STAT_SINGLE(value, statValueIndex);
        }
        case Stat::e_NUM_ALLOCATED_DELTA: {
            return STAT_RANGE(valueDifference, statValueIndex);
        }
        case Stat::e_MAX_ALLOCATED: {
            return STAT_AGGREGATED(absoluteMax, statValueIndex);
        }
        case Stat::e_NUM_ALLOCATIONS: {
            return STAT_SINGLE(increments, statValueIndex);
        }
        case Stat::e_NUM_ALLOCATIONS_DELTA: {
            return STAT_RANGE(incrementsDifference, statValueIndex);
        }
        case Stat::e_NUM_DEALLOCATIONS: {
            return STAT_SINGLE(decrements, statValueIndex);
        }
        case Stat::e_NUM_DEALLOCATIONS_DELTA: {
            return STAT_RANGE(decrementsDifference, statValueIndex);
        }
        default: {
            BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
        }
        }

        return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
#undef OLDEST_SNAPSHOT

        return 0;
    }

    inline static void populateMetric(bdljsn::JsonObject*       obj,
                                      const bmqst::StatContext& ctx,
                                      Stat::Enum                metric)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(obj);

        const bsls::Types::Int64 value = getValue(ctx, -1, metric);

        (*obj)[Stat::toString(metric)].makeNumber() = value;
    }

    inline static void populateMetrics(const JsonPrettyPrinter&  jsonPrinter,
                                       const bdljsn::Json&       parent,
                                       const bmqst::StatContext& ctx,
                                       const bsl::string&        key

    )
    {
        if (ctx.numValues() == 0) {
            // Prefer to omit an empty "values" object
            return;  // RETURN
        }

        bdljsn::Json        json(parent, parent.allocator());
        bdljsn::JsonObject& values = json.theObject();
        values.insert("type", "allocator");
        values.insert("allocator_name", key);

        populateMetric(&values, ctx, Stat::e_NUM_ALLOCATED);
        populateMetric(&values, ctx, Stat::e_NUM_ALLOCATED_DELTA);
        populateMetric(&values, ctx, Stat::e_MAX_ALLOCATED);
        populateMetric(&values, ctx, Stat::e_NUM_ALLOCATIONS);
        populateMetric(&values, ctx, Stat::e_NUM_ALLOCATIONS_DELTA);
        populateMetric(&values, ctx, Stat::e_NUM_DEALLOCATIONS);
        populateMetric(&values, ctx, Stat::e_NUM_DEALLOCATIONS_DELTA);

        jsonPrinter.printJson(json);
    }

    inline static void populateAll(const JsonPrettyPrinter&  jsonPrinter,
                                   const bdljsn::Json&       parent,
                                   const bmqst::StatContext& ctx,
                                   const bsl::string&        key)
    {
        populateMetrics(jsonPrinter, parent, ctx, key);

        for (bmqst::StatContextIterator subIt = ctx.subcontextIterator();
             subIt;
             ++subIt) {
            bdljsn::Json json(parent, parent.allocator());

            bsl::string subKey = key.empty() ? subIt->name()
                                             : key + "." + subIt->name();
            populateAll(jsonPrinter, json, *subIt, subKey);
        }
    }
};

const char* AllocatorStatsConversionUtils::Stat::toString(Stat::Enum value)
{
#define MQBSTAT_CASE(VAL, DESC)                                               \
    case (VAL): {                                                             \
        return (DESC);                                                        \
    } break;

    switch (value) {
        MQBSTAT_CASE(Stat::e_NUM_ALLOCATED, "num_allocated")
        MQBSTAT_CASE(Stat::e_NUM_ALLOCATED_DELTA, "num_allocated_delta")
        MQBSTAT_CASE(Stat::e_MAX_ALLOCATED, "max_allocated")
        MQBSTAT_CASE(Stat::e_NUM_ALLOCATIONS, "num_allocations")
        MQBSTAT_CASE(Stat::e_NUM_ALLOCATIONS_DELTA, "num_allocations_delta")
        MQBSTAT_CASE(Stat::e_NUM_DEALLOCATIONS, "num_deallocations")
        MQBSTAT_CASE(Stat::e_NUM_DEALLOCATIONS_DELTA,
                     "num_deallocations_delta")
    default: BSLS_ASSERT_INVOKE_NORETURN("");
    }
#undef MQBSTAT_CASE
}

}  // close unnamed namespace

// ----------------------------------
// class JsonPrinter::JsonPrinterImpl
// ----------------------------------

/// The implementation class for JsonPrinter, containing all the cached options
/// for printing statistics as JSON.  This implementation exists and is hidden
/// from the package include for the following reasons:
/// - Don't want to expose `bdljsn` names and symbols to the outer scope.
/// - Member fields and functions defined for this implementation are used only
///   locally, so there is no reason to make it visible.
class JsonPrinter::JsonPrinterImpl {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.JSONPRINTERIMPL");

  private:
    // PRIVATE TYPES
    typedef JsonPrinter::StatContextsMap StatContextsMap;

  private:
    // DATA
    /// Options for printing a compact JSON
    const bdljsn::WriteOptions d_opsCompact;

    /// Options for printing a pretty JSON
    const bdljsn::WriteOptions d_opsPretty;

    /// StatContext-s map
    const StatContextsMap d_contexts;

    /// Allocator
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    JsonPrinterImpl(const JsonPrinterImpl& other) BSLS_CPP11_DELETED;
    JsonPrinterImpl&
    operator=(const JsonPrinterImpl& other) BSLS_CPP11_DELETED;

  public:
    // CREATORS

    /// Create a new `JsonPrinterImpl` object, using the specified
    /// `statContextsMap` and the specified `allocator`.
    explicit JsonPrinterImpl(const StatContextsMap& statContextsMap,
                             bslma::Allocator*      allocator);

    // ACCESSORS

    /// Print the JSON-encoded stats to the specified `out`.
    /// If the specified `compact` flag is `true`, the JSON is printed in a
    /// compact form, otherwise the JSON is printed in a pretty form.
    /// Return `0` on success, and non-zero return code on failure.
    ///
    /// THREAD: This method is called in the *StatController scheduler* thread.
    int printStats(bsl::ostream&         os,
                   bool                  compact,
                   int                   statId,
                   const bdlt::Datetime& now,
                   const bsl::string     delimiter) const;
};

inline JsonPrinter::JsonPrinterImpl::JsonPrinterImpl(
    const StatContextsMap& statContextsMap,
    bslma::Allocator*      allocator)
: d_opsCompact(bdljsn::WriteOptions()
                   .setSpacesPerLevel(0)
                   .setStyle(bdljsn::WriteStyle::e_COMPACT)
                   .setSortMembers(true))
, d_opsPretty(bdljsn::WriteOptions()
                  .setSpacesPerLevel(4)
                  .setStyle(bdljsn::WriteStyle::e_PRETTY)
                  .setSortMembers(true))
, d_contexts(statContextsMap, allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

// -----------------
// class JsonPrinter
// -----------------

inline int
JsonPrinter::JsonPrinterImpl::printStats(bsl::ostream&         os,
                                         bool                  compact,
                                         int                   statsId,
                                         const bdlt::Datetime& datetime,
                                         const bsl::string     delimiter) const
{
    // executed by *StatController scheduler* thread

    JsonPrettyPrinter jpp(os, compact ? d_opsCompact : d_opsPretty, delimiter);

    bdljsn::Json json(d_allocator_p);
    json.makeObject();

    {
        // Output stats_id
        json.theObject().insert("stats_id", bdljsn::JsonNumber(statsId));
    }
    {
        // Output datetime in de-facto standard ISO-8601 format:
        char buffer[64];
        bdlt::Iso8601Util::generate(buffer, sizeof(buffer), datetime);
        json.theObject().insert("timestamp", buffer);
    }
    // Populate DOMAIN QUEUES stats
    {
        const bmqst::StatContext& ctx =
            *d_contexts.find("domainQueues")->second;

        DomainsStatsConversionUtils::populateAll(jpp, json, ctx);
    }
    // Populate CLIENTS stats
    {
        const bmqst::StatContext& ctx = *d_contexts.find("clients")->second;

        ClientStatsConversionUtils::populateAll(jpp, json, ctx);
    }
    // Populate CLUSTERS stats
    {
        const bmqst::StatContext& ctx =
            *d_contexts.find("clusterNodes")->second;

        ClientStatsConversionUtils::populateAll(jpp, json, ctx);
    }

    // Populate TCP CHANNELS stats
    {
        const bmqst::StatContext& ctx = *d_contexts.find("channels")->second;

        ChannelStatsConversionUtils::populateAll(jpp, json, ctx);
    }

    // Populate Allocators stats
    {
        StatContextsMap::const_iterator it = d_contexts.find("allocators");

        if (it != d_contexts.end()) {
            const bmqst::StatContext& ctx = *it->second;

            AllocatorStatsConversionUtils::populateAll(jpp, json, ctx, "");
        }
    }

    return 0;
}

JsonPrinter::JsonPrinter(const mqbcfg::StatsConfig& config,
                         bdlmt::EventScheduler*     eventScheduler,
                         const StatContextsMap&     statContextsMap,
                         bslma::Allocator*          allocator)
: d_statContextsMap(statContextsMap)
, d_statsLogFile(allocator)
, d_config(config)
, d_logfile_pattern(config.printer().file() + ".json")
, d_statLogCleaner(eventScheduler, allocator)
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_mp.load(new (*alloc) JsonPrinterImpl(statContextsMap, alloc),
                   alloc);
}

int JsonPrinter::printStats(bsl::ostream&         os,
                            bool                  compact,
                            int                   statsId,
                            const bdlt::Datetime& datetime,
                            const bsl::string     delimiter)
{
    // executed by *StatController scheduler* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_mp);

    return d_impl_mp->printStats(os, compact, statsId, datetime, delimiter);
}

void JsonPrinter::logStats(int lastStatId)
{
    // Dump to statslog file
    // Prepare the log record and associated attributes
    ball::Record            record;
    ball::RecordAttributes& attributes = record.fixedFields();
    bdlt::Datetime          now;
    bdlt::EpochUtil::convertFromTimeT(&now, time(0));
    attributes.setTimestamp(now);
    attributes.setProcessID(bdls::ProcessUtil::getProcessId());
    attributes.setThreadID(bslmt::ThreadUtil::selfIdAsUint64());
    attributes.setFileName(__FILE__);
    attributes.setLineNumber(__LINE__);
    attributes.setCategory(k_LOG_CATEGORY);
    attributes.setSeverity(ball::Severity::e_INFO);

    // Dump stats into bmqbrkr.stats.log
    attributes.clearMessage();
    bsl::ostream os(&attributes.messageStreamBuf());
    const bool   isCompact = false;
    printStats(os, isCompact, lastStatId, now);

    d_statsLogFile.publish(
        record,
        ball::Context(ball::Transmission::e_MANUAL_PUBLISH, 0, 1));
}

int JsonPrinter::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    // Setup the print of stats if configured for it
    if (!isEnabled()) {
        return 0;  // RETURN
    }

    // Configure the stats dump log file
    d_statsLogFile.enableFileLogging(d_logfile_pattern.c_str());
    d_statsLogFile.rotateOnSize(d_config.printer().rotateBytes() / 1024);
    d_statsLogFile.rotateOnTimeInterval(
        bdlt::DatetimeInterval(d_config.printer().rotateDays()));
    d_statsLogFile.setLogFileFunctor(ball::RecordStringFormatter("%m\n"));
    // Record's time is printed not through the record, but part of the
    // 'id banner' (see 'onSnapshot').

    // LogCleanup
    if (d_config.printer().maxAgeDays() <= 0 ||
        d_config.printer().file().empty()) {
        BALL_LOG_INFO << "StatLogCleaning is *disabled* "
                      << "[reason: either 'maxAgeDays' is set to 0 in config "
                      << "or file pattern is empty]";
        return 0;  // RETURN
    }

    bsl::string filePattern;
    ball::LogFileCleanerUtil::logPatternToFilePattern(
        &filePattern,
        d_config.printer().file());
    bsls::TimeInterval maxAge(0, 0);
    maxAge.addDays(d_config.printer().maxAgeDays());

    int rc = d_statLogCleaner.start(filePattern, maxAge);
    if (rc != 0) {
        BALL_LOG_ERROR << "#STATLOG_CLEANING "
                       << "Failed to start log cleaning of '" << filePattern
                       << "' [rc: " << rc << "]";
    }

    return 0;
}

void JsonPrinter::stop()
{
    // Stop the log cleaner
    d_statLogCleaner.stop();
}

}  // close package namespace
}  // close enterprise namespace
