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

#include <mqbstat_jsonprinter.h>

#include <mqbscm_version.h>

// MQB
#include <mqbstat_queuestats.h>

// BDE
#include <ball_log.h>
#include <bdljsn_json.h>
#include <bdljsn_jsonutil.h>
#include <bdlt_datetime.h>
#include <bdlt_epochutil.h>
#include <bsl_ctime.h>
#include <bsl_sstream.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

struct ConversionUtils {
    // PUBLIC CLASS METHODS

    /// "domainQueues" stat context:
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.

    inline static void
    populateMetric(bdljsn::JsonObject*                   metricsObject,
                   const bmqst::StatContext&             ctx,
                   mqbstat::QueueStatsDomain::Stat::Enum metric)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(metricsObject);

        const bsls::Types::Int64 value =
            mqbstat::QueueStatsDomain::getValue(ctx, -1, metric);

        (*metricsObject)[mqbstat::QueueStatsDomain::Stat::toString(metric)]
            .makeNumber() = value;
    }

    inline static void populateQueueStats(bdljsn::JsonObject* queueObject,
                                          const bmqst::StatContext& ctx)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(queueObject);

        if (ctx.numValues() == 0) {
            // Prefer to omit an empty "values" object
            return;  // RETURN
        }

        bdljsn::JsonObject& values = (*queueObject)["values"].makeObject();

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
    }

    inline static void populateOneDomainStats(bdljsn::JsonObject* domainObject,
                                              const bmqst::StatContext& ctx)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(domainObject);

        for (bmqst::StatContextIterator queueIt = ctx.subcontextIterator();
             queueIt;
             ++queueIt) {
            bdljsn::JsonObject& queueObj =
                (*domainObject)[queueIt->name()].makeObject();
            populateQueueStats(&queueObj, *queueIt);

            if (queueIt->numSubcontexts() > 0) {
                bdljsn::JsonObject& appIdsObject =
                    queueObj["appIds"].makeObject();

                // Add metrics per appId, if any
                for (bmqst::StatContextIterator appIdIt =
                         queueIt->subcontextIterator();
                     appIdIt;
                     ++appIdIt) {
                    // Do not expect another nested StatContext within appId
                    BSLS_ASSERT_SAFE(0 == appIdIt->numSubcontexts());

                    populateQueueStats(
                        &appIdsObject[appIdIt->name()].makeObject(),
                        *appIdIt);
                }
            }
        }
    }

    inline static void populateAllDomainsStats(bdljsn::JsonObject* parent,
                                               const bmqst::StatContext& ctx)
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(parent);

        bdljsn::JsonObject& nodes = (*parent)["domains"].makeObject();
        for (bmqst::StatContextIterator domainIt = ctx.subcontextIterator();
             domainIt;
             ++domainIt) {
            populateOneDomainStats(&nodes[domainIt->name()].makeObject(),
                                   *domainIt);
        }
    }
};

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
    JsonPrinterImpl(const JsonPrinterImpl& other) BSLS_KEYWORD_DELETED;
    JsonPrinterImpl&
    operator=(const JsonPrinterImpl& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(JsonPrinterImpl, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `JsonPrinterImpl` object, using the specified
    /// `statContextsMap` and the specified `allocator`.
    explicit JsonPrinterImpl(const StatContextsMap& statContextsMap,
                             bslma::Allocator*      allocator);

    // MANIPULATORS

    /// Print the JSON-encoded stats to the specified `stream`.
    /// If the specified `compact` flag is `true`, the JSON is printed in a
    /// compact form, otherwise the JSON is printed in a pretty form.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    void printStats(bsl::ostream& stream, bool compact);
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

inline void JsonPrinter::JsonPrinterImpl::printStats(bsl::ostream& stream,
                                                     bool          compact)
{
    // executed by the `snapshot` thread

    bdljsn::Json        json(d_allocator_p);
    bdljsn::JsonObject& obj = json.makeObject();

    {
        bdlt::Datetime now;
        bdlt::EpochUtil::convertFromTimeT(&now, time(0));
        bsl::ostringstream ts(d_allocator_p);
        ts << now;
        obj["_timestamp"].makeString(ts.str());
    }

    {
        const bmqst::StatContext& ctx =
            *d_contexts.find("domainQueues")->second;
        bdljsn::JsonObject& domainQueuesObj = obj["domainQueues"].makeObject();

        ConversionUtils::populateAllDomainsStats(&domainQueuesObj, ctx);
    }

    const bdljsn::WriteOptions& ops = compact ? d_opsCompact : d_opsPretty;

    const int rc = bdljsn::JsonUtil::write(stream, json, ops);
    if (0 != rc) {
        BALL_LOG_ERROR << "Failed to encode stats JSON, rc = " << rc;
    }
}

// -----------------
// class JsonPrinter
// -----------------

JsonPrinter::JsonPrinter(const StatContextsMap& statContextsMap,
                         bslma::Allocator*      allocator)
: d_impl_mp(
      bslma::ManagedPtrUtil::allocateManaged<JsonPrinterImpl>(allocator,
                                                              statContextsMap))
{
    // NOTHING
}

void JsonPrinter::printStats(bsl::ostream& stream, bool compact)
{
    // executed by the `snapshot` thread

    d_impl_mp->printStats(stream, compact);
}

}  // close package namespace
}  // close enterprise namespace
