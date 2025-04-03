// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbstat_queuestats.cpp                                             -*-C++-*-
#include <mqbstat_queuestats.h>

#include <mqbscm_version.h>
// BMQ
#include <bmqt_queueflags.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbi_cluster.h>
#include <mqbi_domain.h>
#include <mqbi_queue.h>

#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>
#include <bmqst_statvalue.h>

// BDE
#include <ball_log.h>
#include <ball_logthrottle.h>
#include <bdlb_print.h>
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

const char k_LOG_CATEGORY[] = "MQBSTAT.QUEUESTATS";

/// Name of the stat context to create (holding all domain's queues
/// statistics)
const char k_DOMAIN_STAT_NAME[] = "domain";

/// Name of the stat context to create (holding all client's queues
/// statistics)
const char k_CLIENT_STAT_NAME[] = "client";

/// Maximum messages logged with throttling in a short period of time.
const int k_MAX_INSTANT_MESSAGES = 10;

/// Time interval between messages logged with throttling.
const bsls::Types::Int64 k_NS_PER_MESSAGE =
    bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MINUTE / k_MAX_INSTANT_MESSAGES;

// ------------------
// struct ClientStats
// ------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients
struct ClientStats {
    enum Enum {
        /// Value:      Number of ack messages delivered to the client
        e_STAT_ACK

        ,
        /// Value:      Number of confirm messages delivered to the client
        e_STAT_CONFIRM

        ,
        /// Value:      Accumulated bytes of all messages ever pushed to
        ///             the client
        /// Increments: Number of messages ever pushed to the client
        e_STAT_PUSH

        ,
        /// Value:      Accumulated bytes of all messages ever received from
        ///             the client
        /// Increments: Number of messages ever received from the client
        e_STAT_PUT
    };
};

/// Functor method returning `true`, i.e., filter out, if the specified
/// `record` represents a `*direct*` stat (used when printing to filter them
/// out).
bool filterDirect(const bmqst::TableRecords::Record& record)
{
    return record.type() == bmqst::StatContext::e_TOTAL_VALUE;
}

/// Helper method to calculate queue utilization (for messages/bytes), in
/// percents. First, it calculates the average value of messages/bytes within
/// the publish interval (in range (oldestSnapshot, latestSnapshot)) and then
/// calculates queue utilization as the average value divided by the limit
/// value.
bsls::Types::Int64
queueUtilization(const bmqst::StatContext&                 context,
                 const bmqst::StatValue::SnapshotLocation& latestSnapshot,
                 const bmqst::StatValue::SnapshotLocation& oldestSnapshot,
                 DomainQueueStats::Enum                    currentValue,
                 DomainQueueStats::Enum                    limitValue)
{
    double avg = 0;
    if (latestSnapshot > oldestSnapshot) {
        avg = bmqst::StatUtil::average(
            context.value(bmqst::StatContext::e_DIRECT_VALUE, currentValue),
            latestSnapshot,
            oldestSnapshot);
    }
    else {
        // If no oldest snapshot present, use current value (from the latest
        // snapshot).
        avg = static_cast<double>(bmqst::StatUtil::value(
            context.value(bmqst::StatContext::e_DIRECT_VALUE, currentValue),
            latestSnapshot));
    }
    bsls::Types::Int64 limit = bmqst::StatUtil::value(
        context.value(bmqst::StatContext::e_DIRECT_VALUE, limitValue),
        latestSnapshot);

    bsls::Types::Int64 result = 0;
    if (limit != 0) {
        result = static_cast<bsls::Types::Int64>(
            avg / static_cast<double>(limit) * 100.0);
    }

    return result;
}

}  // close unnamed namespace

// -----------------------------
// struct QueueStatsDomain::Stat
// -----------------------------

const char* QueueStatsDomain::Stat::toString(Stat::Enum value)
{
#define MQBSTAT_CASE(VAL, DESC)                                               \
    case (VAL): {                                                             \
        return (DESC);                                                        \
    } break;

    switch (value) {
        MQBSTAT_CASE(e_NB_PRODUCER, "queue_producers_count")
        MQBSTAT_CASE(e_NB_CONSUMER, "queue_consumers_count")
        MQBSTAT_CASE(e_MESSAGES_CURRENT, "queue_msgs_current")
        MQBSTAT_CASE(e_MESSAGES_MAX, "queue_content_msgs")
        MQBSTAT_CASE(e_MESSAGES_UTILIZATION, "queue_msgs_utilization")
        MQBSTAT_CASE(e_BYTES_CURRENT, "queue_bytes_current")
        MQBSTAT_CASE(e_BYTES_MAX, "queue_content_bytes")
        MQBSTAT_CASE(e_BYTES_UTILIZATION, "queue_bytes_utilization")
        MQBSTAT_CASE(e_PUT_MESSAGES_DELTA, "queue_put_msgs")
        MQBSTAT_CASE(e_PUT_BYTES_DELTA, "queue_put_bytes")
        MQBSTAT_CASE(e_PUT_MESSAGES_ABS, "queue_put_msgs_abs")
        MQBSTAT_CASE(e_PUT_BYTES_ABS, "queue_put_bytes_abs")
        MQBSTAT_CASE(e_PUSH_MESSAGES_DELTA, "queue_push_msgs")
        MQBSTAT_CASE(e_PUSH_BYTES_DELTA, "queue_push_bytes")
        MQBSTAT_CASE(e_PUSH_MESSAGES_ABS, "queue_push_msgs_abs")
        MQBSTAT_CASE(e_PUSH_BYTES_ABS, "queue_push_bytes_abs")
        MQBSTAT_CASE(e_ACK_DELTA, "queue_ack_msgs")
        MQBSTAT_CASE(e_ACK_ABS, "queue_ack_msgs_abs")
        MQBSTAT_CASE(e_ACK_TIME_AVG, "queue_ack_time_avg")
        MQBSTAT_CASE(e_ACK_TIME_MAX, "queue_ack_time_max")
        MQBSTAT_CASE(e_NACK_DELTA, "queue_nack_msgs")
        MQBSTAT_CASE(e_NACK_ABS, "queue_nack_msgs_abs")
        MQBSTAT_CASE(e_CONFIRM_DELTA, "queue_confirm_msgs")
        MQBSTAT_CASE(e_CONFIRM_ABS, "queue_confirm_msgs_abs")
        MQBSTAT_CASE(e_CONFIRM_TIME_AVG, "queue_confirm_time_avg")
        MQBSTAT_CASE(e_CONFIRM_TIME_MAX, "queue_confirm_time_max")
        MQBSTAT_CASE(e_REJECT_ABS, "queue_reject_msgs_abs")
        MQBSTAT_CASE(e_REJECT_DELTA, "queue_reject_msgs")
        MQBSTAT_CASE(e_QUEUE_TIME_AVG, "queue_queue_time_avg")
        MQBSTAT_CASE(e_QUEUE_TIME_MAX, "queue_queue_time_max")
        MQBSTAT_CASE(e_GC_MSGS_DELTA, "queue_gc_msgs")
        MQBSTAT_CASE(e_GC_MSGS_ABS, "queue_gc_msgs_abs")
        MQBSTAT_CASE(e_ROLE, "queue_role")
        MQBSTAT_CASE(e_CFG_MSGS, "queue_cfg_msgs")
        MQBSTAT_CASE(e_CFG_BYTES, "queue_cfg_bytes")
        MQBSTAT_CASE(e_NO_SC_MSGS_DELTA, "queue_nack_noquorum_msgs")
        MQBSTAT_CASE(e_NO_SC_MSGS_ABS, "queue_nack_noquorum_msgs_abs")
        MQBSTAT_CASE(e_HISTORY_ABS, "queue_history_abs")
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;

#undef MQBSTAT_CASE
}

// ----------------------
// class QueueStatsDomain
// ----------------------

bsls::Types::Int64
QueueStatsDomain::getValue(const bmqst::StatContext& context,
                           int                       snapshotId,
                           const Stat::Enum&         stat)
{
    // invoked from the SNAPSHOT thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(snapshotId >= -1);  // do not support other negatives yet

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

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot,                                                       \
        OLDEST_SNAPSHOT(STAT))

    switch (stat) {
    case QueueStatsDomain::Stat::e_NB_PRODUCER: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_NB_PRODUCER);
    }
    case QueueStatsDomain::Stat::e_NB_CONSUMER: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_NB_CONSUMER);
    }
    case QueueStatsDomain::Stat::e_MESSAGES_CURRENT: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_MESSAGES);
    }
    case QueueStatsDomain::Stat::e_MESSAGES_MAX: {
        return STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_MESSAGES);
    }
    case QueueStatsDomain::Stat::e_MESSAGES_UTILIZATION: {
        // Calculate queue utilization (in precents) as the average value of
        // the current number of messages within publish interval divided by
        // the limit number of messages.
        return queueUtilization(
            context,
            latestSnapshot,
            OLDEST_SNAPSHOT(DomainQueueStats::e_STAT_MESSAGES),
            DomainQueueStats::e_STAT_MESSAGES,
            DomainQueueStats::e_CFG_MSGS);
    }
    case QueueStatsDomain::Stat::e_BYTES_CURRENT: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_BYTES);
    }
    case QueueStatsDomain::Stat::e_BYTES_MAX: {
        return STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_BYTES);
    }
    case QueueStatsDomain::Stat::e_BYTES_UTILIZATION: {
        // Calculate queue utilization (in precents) as the average value of
        // the current number of bytes within publish interval divided by the
        // limit number of bytes.
        return queueUtilization(
            context,
            latestSnapshot,
            OLDEST_SNAPSHOT(DomainQueueStats::e_STAT_BYTES),
            DomainQueueStats::e_STAT_BYTES,
            DomainQueueStats::e_CFG_BYTES);
    }
    case QueueStatsDomain::Stat::e_PUT_BYTES_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_PUT);
    }
    case QueueStatsDomain::Stat::e_PUSH_BYTES_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_PUSH);
    }
    case QueueStatsDomain::Stat::e_ACK_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_ACK);
    }
    case QueueStatsDomain::Stat::e_ACK_TIME_AVG: {
        const bsls::Types::Int64 avg =
            STAT_RANGE(averagePerEvent, DomainQueueStats::e_STAT_ACK_TIME);
        return avg == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0 : avg;
    }
    case QueueStatsDomain::Stat::e_ACK_TIME_MAX: {
        const bsls::Types::Int64 max =
            STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_ACK_TIME);
        return max == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0 : max;
    }
    case QueueStatsDomain::Stat::e_NACK_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_NACK);
    }
    case QueueStatsDomain::Stat::e_CONFIRM_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_CONFIRM);
    }
    case QueueStatsDomain::Stat::e_REJECT_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_REJECT);
    }
    case QueueStatsDomain::Stat::e_CONFIRM_TIME_AVG: {
        const bsls::Types::Int64 avg =
            STAT_RANGE(averagePerEvent, DomainQueueStats::e_STAT_CONFIRM_TIME);
        return avg == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0 : avg;
    }
    case QueueStatsDomain::Stat::e_CONFIRM_TIME_MAX: {
        const bsls::Types::Int64 max =
            STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_CONFIRM_TIME);
        return max == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0 : max;
    }
    case QueueStatsDomain::Stat::e_QUEUE_TIME_AVG: {
        const bsls::Types::Int64 avg =
            STAT_RANGE(averagePerEvent, DomainQueueStats::e_STAT_QUEUE_TIME);
        return avg == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0 : avg;
    }
    case QueueStatsDomain::Stat::e_QUEUE_TIME_MAX: {
        const bsls::Types::Int64 max =
            STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_QUEUE_TIME);
        return max == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0 : max;
    }
    case QueueStatsDomain::Stat::e_GC_MSGS_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_GC_MSGS);
    }
    case QueueStatsDomain::Stat::e_PUT_MESSAGES_ABS: {
        return STAT_SINGLE(increments, DomainQueueStats::e_STAT_PUT);
    }
    case QueueStatsDomain::Stat::e_PUSH_MESSAGES_ABS: {
        return STAT_SINGLE(increments, DomainQueueStats::e_STAT_PUSH);
    }
    case QueueStatsDomain::Stat::e_PUT_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference, DomainQueueStats::e_STAT_PUT);
    }
    case QueueStatsDomain::Stat::e_PUSH_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference, DomainQueueStats::e_STAT_PUSH);
    }
    case QueueStatsDomain::Stat::e_PUT_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_PUT);
    }
    case QueueStatsDomain::Stat::e_PUSH_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_PUSH);
    }
    case QueueStatsDomain::Stat::e_ACK_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_ACK);
    }
    case QueueStatsDomain::Stat::e_NACK_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_NACK);
    }
    case QueueStatsDomain::Stat::e_CONFIRM_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_CONFIRM);
    }
    case QueueStatsDomain::Stat::e_REJECT_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_REJECT);
    }
    case QueueStatsDomain::Stat::e_GC_MSGS_DELTA: {
        return STAT_RANGE(valueDifference, DomainQueueStats::e_STAT_GC_MSGS);
    }
    case QueueStatsDomain::Stat::e_ROLE: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_ROLE);
    }
    case QueueStatsDomain::Stat::e_CFG_MSGS: {
        return STAT_SINGLE(value, DomainQueueStats::e_CFG_MSGS);
    }
    case QueueStatsDomain::Stat::e_CFG_BYTES: {
        return STAT_SINGLE(value, DomainQueueStats::e_CFG_BYTES);
    }
    case QueueStatsDomain::Stat::e_NO_SC_MSGS_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_NO_SC_MSGS);
    }
    case QueueStatsDomain::Stat::e_NO_SC_MSGS_DELTA: {
        return STAT_RANGE(valueDifference,
                          DomainQueueStats::e_STAT_NO_SC_MSGS);
    }
    case QueueStatsDomain::Stat::e_HISTORY_ABS: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_HISTORY);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

QueueStatsDomain::QueueStatsDomain(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_statContext_mp(0)
, d_subContextsHolder(d_allocator_p)
, d_subContextsLookup(d_allocator_p)
{
    // NOTHING
}

void QueueStatsDomain::initialize(const bmqt::Uri& uri, mqbi::Domain* domain)
{
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize was already called");

    // Create subContext
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    d_statContext_mp = domain->queueStatContext()->addSubcontext(
        bmqst::StatContextConfiguration(uri.canonical(), &localAllocator));

    // Initialize the role to 'unknown'; once the 'mqbblp::Queue' is
    // configured, the role will be accordingly set
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_ROLE, Role::e_UNKNOWN);

    // Build a datum map containing the following values:
    //: o bmqQueue: the name of the queue
    //: o bmqCluster: the name of the cluster this queue lives on.  Note that
    //:   for local cluster, the 'hostname' is appended to the cluster name in
    //:   order to guarantee global uniqueness
    //: o bmqDomain: the name of the domain this queue belongs to.  Note that
    //:   due to limitation in the character space of the metrics framework,
    //:   '~' is an invalid character and is being replaced by an '_'

    bslma::Allocator* alloc = d_statContext_mp->datumAllocator();

    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bdld::DatumMapBuilder                 builder(alloc);

    builder.pushBack("queue", bdld::Datum::copyString(uri.queue(), alloc));
    builder.pushBack("cluster",
                     bdld::Datum::copyString(domain->cluster()->name(),
                                             alloc));
    builder.pushBack("domain", bdld::Datum::copyString(uri.domain(), alloc));
    builder.pushBack("tier", bdld::Datum::copyString(uri.tier(), alloc));

    datum->adopt(builder.commit());

    // Create subcontexts for each AppId to store per-AppId metrics, such as
    // `e_CONFIRM_TIME_MAX` or `e_QUEUE_TIME_MAX`, so the metrics can be
    // inspected separately for each application.
    if (!domain->cluster()->isRemote() &&
        domain->config().mode().isFanoutValue() &&
        domain->config().mode().fanout().publishAppIdMetrics()) {
        const bsl::vector<bsl::string>& appIDs =
            domain->config().mode().fanout().appIDs();
        for (bsl::vector<bsl::string>::const_iterator cit = appIDs.begin();
             cit != appIDs.end();
             ++cit) {
            StatSubContextMp subContext = d_statContext_mp->addSubcontext(
                bmqst::StatContextConfiguration(*cit, &localAllocator));

            d_subContextsLookup.insert(bsl::make_pair(*cit, subContext.get()));
            d_subContextsHolder.emplace_back(
                bslmf::MovableRefUtil::move(subContext));
        }
    }
}

QueueStatsDomain& QueueStatsDomain::setReaderCount(int readerCount)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    d_statContext_mp->setValue(DomainQueueStats::e_STAT_NB_CONSUMER,
                               readerCount);

    return *this;
}

QueueStatsDomain& QueueStatsDomain::setWriterCount(int writerCount)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    d_statContext_mp->setValue(DomainQueueStats::e_STAT_NB_PRODUCER,
                               writerCount);

    return *this;
}

void QueueStatsDomain::onEvent(EventType::Enum    type,
                               bsls::Types::Int64 value,
                               const bsl::string& appId)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    if (d_subContextsLookup.empty()) {
        // Not an error if the domain is not configured for Apps stats.
        return;  // RETURN
    }

    bsl::unordered_map<bsl::string, bmqst::StatContext*>::iterator it =
        d_subContextsLookup.find(appId);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(it ==
                                              d_subContextsLookup.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

        BALL_LOGTHROTTLE_WARN(k_MAX_INSTANT_MESSAGES, k_NS_PER_MESSAGE)
            << "[THROTTLED] No matching StatContext for domain: "
            << d_statContext_mp->name() << ", appId: " << appId;
        return;  // RETURN
    }

    bmqst::StatContext* appIdContext = it->second;
    BSLS_ASSERT_SAFE(appIdContext);

    switch (type) {
    case EventType::e_CONFIRM_TIME: {
        appIdContext->reportValue(DomainQueueStats::e_STAT_CONFIRM_TIME,
                                  value);
    } break;
    case EventType::e_QUEUE_TIME: {
        appIdContext->reportValue(DomainQueueStats::e_STAT_QUEUE_TIME, value);
    } break;
    case EventType::e_ADD_MESSAGE: {
        appIdContext->adjustValue(DomainQueueStats::e_STAT_BYTES, value);
        appIdContext->adjustValue(DomainQueueStats::e_STAT_MESSAGES, 1);
    } break;
    case EventType::e_DEL_MESSAGE: {
        appIdContext->adjustValue(DomainQueueStats::e_STAT_BYTES, -value);
        appIdContext->adjustValue(DomainQueueStats::e_STAT_MESSAGES, -1);
    } break;
    case EventType::e_PURGE: {
        // NOTE: Setting the value like that will cause weird results if using
        //       the stat to get rates
        appIdContext->setValue(DomainQueueStats::e_STAT_BYTES, 0);
        appIdContext->setValue(DomainQueueStats::e_STAT_MESSAGES, 0);
    } break;

    // Some of these event types make no sense per appId and should be reported
    // per entire queue instead
    case EventType::e_ACK: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_ACK_TIME: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_NACK: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_CONFIRM: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_REJECT: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_PUSH: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_PUT: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_GC_MESSAGE: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_CHANGE_ROLE: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_CFG_MSGS: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_CFG_BYTES: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_NO_SC_MESSAGE: BSLS_ANNOTATION_FALLTHROUGH;
    case EventType::e_UPDATE_HISTORY: {
        BSLS_ASSERT_SAFE(false && "Unexpected event type for appId metric");
    } break;

    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

void QueueStatsDomain::updateDomainAppIds(
    const bsl::vector<bsl::string>& appIds)
{
    if (appIds.empty()) {
        d_subContextsLookup.clear();
        d_subContextsHolder.clear();
        return;  // RETURN
    }

    bsl::unordered_set<bsl::string> remainingAppIds(appIds.begin(),
                                                    appIds.end(),
                                                    d_allocator_p);

    // 1. Remove subcontexts for unneeded appIds
    bsl::list<StatSubContextMp>::iterator it = d_subContextsHolder.begin();
    while (it != d_subContextsHolder.end()) {
        const bsl::string& ctxAppId = it->get()->name();
        bsl::unordered_set<bsl::string>::const_iterator sIt =
            remainingAppIds.find(ctxAppId);
        if (sIt == remainingAppIds.end()) {
            // Subcontext for this appId is no longer needed, remove it from
            // the holder and lookup table
            d_subContextsLookup.erase(ctxAppId);
            it = d_subContextsHolder.erase(it);
        }
        else {
            // This appId is needed, but the stat context is already built for
            // it
            remainingAppIds.erase(sIt);
            ++it;
        }
    }

    if (remainingAppIds.empty()) {
        return;  // RETURN
    }

    // 2. Add the remaining appIds
    bdlma::LocalSequentialAllocator<2048> localAllocator(d_allocator_p);

    for (bsl::unordered_set<bsl::string>::const_iterator sIt =
             remainingAppIds.begin();
         sIt != remainingAppIds.end();
         sIt++) {
        StatSubContextMp subContext = d_statContext_mp->addSubcontext(
            bmqst::StatContextConfiguration(*sIt, &localAllocator));

        d_subContextsLookup.insert(bsl::make_pair(*sIt, subContext.get()));
        d_subContextsHolder.emplace_back(
            bslmf::MovableRefUtil::move(subContext));
    }
}

// -----------------------------
// struct QueueStatsDomain::Role
// -----------------------------

bsl::ostream& QueueStatsDomain::Role::print(bsl::ostream& stream,
                                            QueueStatsDomain::Role::Enum value,
                                            int                          level,
                                            int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << Role::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueStatsDomain::Role::toAscii(Role::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNKNOWN)
        CASE(PRIMARY)
        CASE(REPLICA)
        CASE(PROXY)
    default: return "(* UNKNOWN *)";
    }

#undef case
}

// ----------------------
// class QueueStatsClient
// ----------------------

bsls::Types::Int64
QueueStatsClient::getValue(const bmqst::StatContext& context,
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

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE, STAT),              \
        latestSnapshot,                                                       \
        oldestSnapshot)

    switch (stat) {
    case QueueStatsClient::Stat::e_PUSH_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference, ClientStats::e_STAT_PUSH);
    }
    case QueueStatsClient::Stat::e_PUT_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference, ClientStats::e_STAT_PUT);
    }
    case QueueStatsClient::Stat::e_ACK_DELTA: {
        return STAT_RANGE(incrementsDifference, ClientStats::e_STAT_ACK);
    }
    case QueueStatsClient::Stat::e_CONFIRM_DELTA: {
        return STAT_RANGE(incrementsDifference, ClientStats::e_STAT_CONFIRM);
    }
    case QueueStatsClient::Stat::e_PUSH_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, ClientStats::e_STAT_PUSH);
    }
    case QueueStatsClient::Stat::e_PUT_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, ClientStats::e_STAT_PUT);
    }
    case QueueStatsClient::Stat::e_PUSH_MESSAGES_ABS: {
        return STAT_SINGLE(increments, ClientStats::e_STAT_PUSH);
    }
    case QueueStatsClient::Stat::e_PUT_MESSAGES_ABS: {
        return STAT_SINGLE(increments, ClientStats::e_STAT_PUT);
    }
    case QueueStatsClient::Stat::e_ACK_ABS: {
        return STAT_SINGLE(increments, ClientStats::e_STAT_ACK);
    }
    case QueueStatsClient::Stat::e_CONFIRM_ABS: {
        return STAT_SINGLE(increments, ClientStats::e_STAT_CONFIRM);
    }
    case QueueStatsClient::Stat::e_PUSH_BYTES_ABS: {
        return STAT_SINGLE(value, ClientStats::e_STAT_PUSH);
    }
    case QueueStatsClient::Stat::e_PUT_BYTES_ABS: {
        return STAT_SINGLE(value, ClientStats::e_STAT_PUT);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

QueueStatsClient::QueueStatsClient()
: d_statContext_mp(0)
{
    // NOTHING
}

void QueueStatsClient::initialize(const bmqt::Uri&    uri,
                                  bmqst::StatContext* clientStatContext,
                                  bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize called twice");

    // Create subContext
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    d_statContext_mp = clientStatContext->addSubcontext(
        bmqst::StatContextConfiguration(uri.asString(), &localAllocator));
}

void QueueStatsClient::onEvent(EventType::Enum type, bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    switch (type) {
    case EventType::e_ACK: {
        // For ACK, we don't have any bytes value, but we also wouldn't care ..
        d_statContext_mp->adjustValue(ClientStats::e_STAT_ACK, 1);
    } break;
    case EventType::e_CONFIRM: {
        // For CONFIRM, we don't care about the bytes value ..
        d_statContext_mp->adjustValue(ClientStats::e_STAT_CONFIRM, 1);
    } break;
    case EventType::e_PUSH: {
        d_statContext_mp->adjustValue(ClientStats::e_STAT_PUSH, value);
    } break;
    case EventType::e_PUT: {
        d_statContext_mp->adjustValue(ClientStats::e_STAT_PUT, value);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

// ---------------------
// struct QueueStatsUtil
// ---------------------

bsl::shared_ptr<bmqst::StatContext>
QueueStatsUtil::initializeStatContextDomains(int               historySize,
                                             bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_DOMAIN_STAT_NAME,
                                           &localAllocator);

    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("nb_producer")
        .value("nb_consumer")
        .value("messages")
        .value("bytes")
        .value("ack")
        .value("ack_time", bmqst::StatValue::e_DISCRETE)
        .value("nack")
        .value("confirm")
        .value("confirm_time", bmqst::StatValue::e_DISCRETE)
        .value("reject")
        .value("queue_time", bmqst::StatValue::e_DISCRETE)
        .value("gc")
        .value("push")
        .value("put")
        .value("role")
        .value("cfg_msgs")
        .value("cfg_bytes")
        .value("content_msgs")
        .value("content_bytes")
        .value("history_size");
    // NOTE: If the stats are using too much memory, we could reconsider
    //       nb_producer, nb_consumer, messages and bytes to be using atomic
    //       int and not stat value.

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

bsl::shared_ptr<bmqst::StatContext>
QueueStatsUtil::initializeStatContextClients(int               historySize,
                                             bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_CLIENT_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("ack")
        .value("confirm")
        .value("push")
        .value("put");
    // NOTE: If the stats are using too much memory, we could reconsider
    //       in_event and out_event to be using atomic int and not stat value.

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

void QueueStatsUtil::initializeTableAndTipDomains(
    bmqst::Table*                  table,
    bmqst::BasicTableInfoProvider* tip,
    int                            historySize,
    bmqst::StatContext*            statContext)
{
    // Use only one level for now ...
    bmqst::StatValue::SnapshotLocation start(0, 0);
    bmqst::StatValue::SnapshotLocation end(0, historySize - 1);

    // Create table
    bmqst::TableSchema& schema = table->schema();

    schema.addDefaultIdColumn("id");
    schema.addColumn("nb_producer",
                     DomainQueueStats::e_STAT_NB_PRODUCER,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("nb_consumer",
                     DomainQueueStats::e_STAT_NB_CONSUMER,
                     bmqst::StatUtil::value,
                     start);

    schema.addColumn("messages",
                     DomainQueueStats::e_STAT_MESSAGES,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("bytes",
                     DomainQueueStats::e_STAT_BYTES,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("history_size",
                     DomainQueueStats::e_STAT_HISTORY,
                     bmqst::StatUtil::value,
                     start);

    schema.addColumn("put_msgs_delta",
                     DomainQueueStats::e_STAT_PUT,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("put_bytes_delta",
                     DomainQueueStats::e_STAT_PUT,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("put_msgs_abs",
                     DomainQueueStats::e_STAT_PUT,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("put_bytes_abs",
                     DomainQueueStats::e_STAT_PUT,
                     bmqst::StatUtil::value,
                     start);

    schema.addColumn("push_msgs_delta",
                     DomainQueueStats::e_STAT_PUSH,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("push_bytes_delta",
                     DomainQueueStats::e_STAT_PUSH,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("push_msgs_abs",
                     DomainQueueStats::e_STAT_PUSH,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("push_bytes_abs",
                     DomainQueueStats::e_STAT_PUSH,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("ack_delta",
                     DomainQueueStats::e_STAT_ACK,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("ack_abs",
                     DomainQueueStats::e_STAT_ACK,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("ack_time_avg",
                     DomainQueueStats::e_STAT_ACK_TIME,
                     bmqst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("ack_time_max",
                     DomainQueueStats::e_STAT_ACK_TIME,
                     bmqst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("nack_delta",
                     DomainQueueStats::e_STAT_NACK,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("nack_abs",
                     DomainQueueStats::e_STAT_NACK,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("confirm_delta",
                     DomainQueueStats::e_STAT_CONFIRM,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("confirm_abs",
                     DomainQueueStats::e_STAT_CONFIRM,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("confirm_time_avg",
                     DomainQueueStats::e_STAT_CONFIRM_TIME,
                     bmqst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("confirm_time_max",
                     DomainQueueStats::e_STAT_CONFIRM_TIME,
                     bmqst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("reject_delta",
                     DomainQueueStats::e_STAT_REJECT,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("reject_abs",
                     DomainQueueStats::e_STAT_REJECT,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("queue_time_avg",
                     DomainQueueStats::e_STAT_QUEUE_TIME,
                     bmqst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("queue_time_max",
                     DomainQueueStats::e_STAT_QUEUE_TIME,
                     bmqst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("gc_msgs_delta",
                     DomainQueueStats::e_STAT_GC_MSGS,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("gc_msgs_abs",
                     DomainQueueStats::e_STAT_GC_MSGS,
                     bmqst::StatUtil::value,
                     start);
    schema.addColumn("no_sc_msgs_delta",
                     DomainQueueStats::e_STAT_NO_SC_MSGS,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("no_sc_msgs_abs",
                     DomainQueueStats::e_STAT_NO_SC_MSGS,
                     bmqst::StatUtil::value,
                     start);

    // Configure records
    bmqst::TableRecords& records = table->records();
    records.setContext(statContext);
    records.setFilter(&filterDirect);

    // Create the tip
    tip->setTable(table);
    tip->setColumnGroup("");
    tip->addColumn("id", "").justifyLeft();

    tip->setColumnGroup("Info");
    tip->addColumn("nb_producer", "producer").zeroString("");
    tip->addColumn("nb_consumer", "consumer").zeroString("");

    tip->setColumnGroup("Content");
    tip->addColumn("messages", "msgs").zeroString("");
    tip->addColumn("bytes", "bytes").zeroString("").printAsMemory();

    tip->setColumnGroup("Put");
    tip->addColumn("put_msgs_delta", "msgs (d)").zeroString("");
    tip->addColumn("put_bytes_delta", "bytes (d)")
        .zeroString("")
        .printAsMemory();
    tip->addColumn("put_msgs_abs", "msgs").zeroString("");
    tip->addColumn("put_bytes_abs", "bytes").zeroString("").printAsMemory();

    tip->setColumnGroup("Push");
    tip->addColumn("push_msgs_delta", "msgs (d)").zeroString("");
    tip->addColumn("push_bytes_delta", "bytes (d)")
        .zeroString("")
        .printAsMemory();
    tip->addColumn("push_msgs_abs", "msgs").zeroString("");
    tip->addColumn("push_bytes_abs", "bytes").zeroString("").printAsMemory();

    tip->setColumnGroup("Queue Time");
    tip->addColumn("queue_time_avg", "avg")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();
    tip->addColumn("queue_time_max", "max")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();

    tip->setColumnGroup("Ack");
    tip->addColumn("ack_delta", "delta").zeroString("");
    tip->addColumn("ack_abs", "abs").zeroString("");
    tip->addColumn("ack_time_avg", "time avg")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();
    tip->addColumn("ack_time_max", "time max")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();
    tip->setColumnGroup("Nack");
    tip->addColumn("nack_delta", "delta").zeroString("");
    tip->addColumn("nack_abs", "abs").zeroString("");

    tip->setColumnGroup("Confirm");
    tip->addColumn("confirm_delta", "delta").zeroString("");
    tip->addColumn("confirm_abs", "abs").zeroString("");
    tip->addColumn("confirm_time_avg", "time avg")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();
    tip->addColumn("confirm_time_max", "time max")
        .zeroString("")
        .extremeValueString("")
        .printAsNsTimeInterval();
    tip->setColumnGroup("Reject");
    tip->addColumn("reject_delta", "delta").zeroString("");
    tip->addColumn("reject_abs", "abs").zeroString("");

    tip->setColumnGroup("GC");
    tip->addColumn("gc_msgs_delta", "delta").zeroString("");
    tip->addColumn("gc_msgs_abs", "abs").zeroString("");

    tip->setColumnGroup("History");
    tip->addColumn("history_size", "# GUIDs").zeroString("");
}

void QueueStatsUtil::initializeTableAndTipClients(
    bmqst::Table*                  table,
    bmqst::BasicTableInfoProvider* tip,
    int                            historySize,
    bmqst::StatContext*            statContext)
{
    // Use only one level for now ...
    bmqst::StatValue::SnapshotLocation start(0, 0);
    bmqst::StatValue::SnapshotLocation end(0, historySize - 1);

    // Create table
    bmqst::TableSchema& schema = table->schema();

    schema.addDefaultIdColumn("id");

    schema.addColumn("push_messages_delta",
                     ClientStats::e_STAT_PUSH,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("push_bytes_delta",
                     ClientStats::e_STAT_PUSH,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("push_messages_abs",
                     ClientStats::e_STAT_PUSH,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("push_bytes_abs",
                     ClientStats::e_STAT_PUSH,
                     bmqst::StatUtil::value,
                     start);

    schema.addColumn("put_messages_delta",
                     ClientStats::e_STAT_PUT,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("put_bytes_delta",
                     ClientStats::e_STAT_PUT,
                     bmqst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("put_messages_abs",
                     ClientStats::e_STAT_PUT,
                     bmqst::StatUtil::increments,
                     start);
    schema.addColumn("put_bytes_abs",
                     ClientStats::e_STAT_PUT,
                     bmqst::StatUtil::value,
                     start);

    schema.addColumn("ack_delta",
                     ClientStats::e_STAT_ACK,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("ack_abs",
                     ClientStats::e_STAT_ACK,
                     bmqst::StatUtil::increments,
                     start);

    schema.addColumn("confirm_delta",
                     ClientStats::e_STAT_CONFIRM,
                     bmqst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("confirm_abs",
                     ClientStats::e_STAT_CONFIRM,
                     bmqst::StatUtil::increments,
                     start);

    // Configure records
    bmqst::TableRecords& records = table->records();
    records.setContext(statContext);
    records.setFilter(&filterDirect);

    // Create the tip
    tip->setTable(table);
    tip->setColumnGroup("");
    tip->addColumn("id", "").justifyLeft();

    tip->setColumnGroup("Push");
    tip->addColumn("push_messages_delta", "messages (d)").zeroString("");
    tip->addColumn("push_bytes_delta", "bytes (d)")
        .zeroString("")
        .printAsMemory();
    tip->addColumn("push_messages_abs", "messages").zeroString("");
    tip->addColumn("push_bytes_abs", "bytes").zeroString("").printAsMemory();

    tip->setColumnGroup("Put");
    tip->addColumn("put_messages_delta", "messages (d)").zeroString("");
    tip->addColumn("put_bytes_delta", "bytes (d)")
        .zeroString("")
        .printAsMemory();
    tip->addColumn("put_messages_abs", "messages").zeroString("");
    tip->addColumn("put_bytes_abs", "bytes").zeroString("").printAsMemory();

    tip->setColumnGroup("Confirm");
    tip->addColumn("confirm_delta", "events (d)").zeroString("");
    tip->addColumn("confirm_abs", "events").zeroString("");

    tip->setColumnGroup("Ack");
    tip->addColumn("ack_delta", "events (d)").zeroString("");
    tip->addColumn("ack_abs", "events").zeroString("");
}

}  // close package namespace
}  // close enterprise namespace
