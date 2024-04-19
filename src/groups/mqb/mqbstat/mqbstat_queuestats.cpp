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

// MWC
#include <mwcst_statcontext.h>
#include <mwcst_statutil.h>
#include <mwcst_statvalue.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all domain's queues
/// statistics)
static const char k_DOMAIN_STAT_NAME[] = "domain";

/// Name of the stat context to create (holding all client's queues
/// statistics)
static const char k_CLIENT_STAT_NAME[] = "client";

// -----------------------
// struct DomainQueueStats
// -----------------------

/// Namespace for the constants of stat values that applies to the queues on
/// the domain
struct DomainQueueStats {
    enum Enum {
        /// Value:      Current number of clients who opened the queue with
        ///             the `WRITE` flag
        e_STAT_NB_PRODUCER

        ,
        e_STAT_NB_CONSUMER
        // Value:      Current number of clients who opened the queue with
        //             the 'READ' flag

        ,
        e_STAT_MESSAGES
        // Value:      Current number of messages in the queue

        ,
        e_STAT_BYTES
        // Value:      Accumulated bytes of all messages currently in the
        //             queue

        ,
        e_STAT_ACK
        // Value:      Number of ack messages delivered by this queue

        ,
        e_STAT_ACK_TIME
        // Value:      The time between PUT and ACK (in nanoseconds).

        ,
        e_STAT_NACK
        // Value:      Number of NACK messages generated for this queue

        ,
        e_STAT_CONFIRM
        // Value:      Number of CONFIRM messages received by this queue

        ,
        e_STAT_CONFIRM_TIME
        // Value:      The time between PUSH and CONFIRM (in nanoseconds).

        ,
        e_STAT_REJECT
        // Value:      Number of messages rejected by this queue (RDA
        //             reaching zero)

        ,
        e_STAT_QUEUE_TIME
        // Value:      The time spent by the message in the queue (in
        //             nanoseconds).

        ,
        e_STAT_PUSH
        // Value:      Accumulated bytes of all messages ever pushed from
        //             the queue
        // Increment:  Number of messages ever pushed from the queue

        ,
        e_STAT_PUT
        // Value:      Accumulated bytes of all messages ever put in the
        //             queue
        // Increment:  Number of messages ever put in the queue

        ,
        e_STAT_GC_MSGS
        // Value:      Accumulated number of messages ever GC'ed in the
        //             queue

        ,
        e_STAT_ROLE
        // Value:      Role (Unknown, Primary, Replica, Proxy)

        ,
        e_CFG_MSGS
        // Value:      The configured queue messages capacity

        ,
        e_CFG_BYTES
        // Value:      The configured queue bytes capacity
        ,
        e_STAT_NO_SC_MSGS
        // Value:      Accumulated number of messages in the strong
        //             consistency queue expired before receiving quorum
        //             Receipts
    };
};

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
        e_STAT_CONFIRM
        // Value:      Number of confirm messages delivered to the client

        ,
        e_STAT_PUSH
        // Value:      Accumulated bytes of all messages ever pushed to
        //             the client
        // Increments: Number of messages ever pushed to the client

        ,
        e_STAT_PUT
        // Value:      Accumulated bytes of all messages ever received from
        //             the client
        // Increments: Number of messages ever received from the client
    };
};

/// Functor method returning `true`, i.e., filter out, if the specified
/// `record` represents a `*direct*` stat (used when printing to filter them
/// out).
bool filterDirect(const mwcst::TableRecords::Record& record)
{
    return record.type() == mwcst::StatContext::DMCST_TOTAL_VALUE;
}

/// Functor object returning `true`, i.e., filter out, if the specified 'name'
/// matches context's name
class ContextNameMatcher {
  private:
    // DATA
    const bsl::string d_name;

  public:
    // CREATORS
    ContextNameMatcher(const bsl::string& name)
    : d_name(name)
    {
        // NOTHING
    }

    // ACCESSORS
    bool
    operator()(const bslma::ManagedPtr<mwcst::StatContext>& context_mp) const
    {
        return (context_mp->name() == d_name);
    }
};

}  // close unnamed namespace

// ----------------------
// class QueueStatsDomain
// ----------------------

bsls::Types::Int64
QueueStatsDomain::getValue(const mwcst::StatContext& context,
                           int                       snapshotId,
                           const Stat::Enum&         stat)
{
    // invoked from the SNAPSHOT thread

    const mwcst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const mwcst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::DMCST_DIRECT_VALUE, STAT),          \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::DMCST_DIRECT_VALUE, STAT),          \
        latestSnapshot,                                                       \
        oldestSnapshot)

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
    case QueueStatsDomain::Stat::e_BYTES_CURRENT: {
        return STAT_SINGLE(value, DomainQueueStats::e_STAT_BYTES);
    }
    case QueueStatsDomain::Stat::e_BYTES_MAX: {
        return STAT_RANGE(rangeMax, DomainQueueStats::e_STAT_BYTES);
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
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

QueueStatsDomain::QueueStatsDomain()
: d_statContext_mp(0)
, d_subContexts_mp(0)
{
    // NOTHING
}

void QueueStatsDomain::initialize(const bmqt::Uri&  uri,
                                  mqbi::Domain*     domain,
                                  bslma::Allocator* allocator)
{
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize was already called");

    // Create subContext
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    d_statContext_mp = domain->queueStatContext()->addSubcontext(
        mwcst::StatContextConfiguration(uri.canonical(), &localAllocator));

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

    // Create subcontexts if queue mode is `fanout` and domain name is in
    // `appIdPostingDomains` list.
    if (domain->config().mode().isFanoutValue()) {
        const bsl::vector<bsl::string>& appIdPostingDomains =
            mqbcfg::BrokerConfig::get().stats().appIdPostingDomains();
        if (bsl::find(appIdPostingDomains.begin(),
                      appIdPostingDomains.end(),
                      uri.domain()) != appIdPostingDomains.end()) {
            d_subContexts_mp.load(new (*allocator)
                                      bsl::list<StatSubContextMp>(allocator),
                                  allocator);
            const bsl::vector<bsl::string>& appIDs =
                domain->config().mode().fanout().appIDs();
            for (bsl::vector<bsl::string>::const_iterator cit = appIDs.begin();
                 cit != appIDs.end();
                 ++cit) {
                d_subContexts_mp->push_back(d_statContext_mp->addSubcontext(
                    mwcst::StatContextConfiguration(*cit, &localAllocator)));
            }
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

void QueueStatsDomain::onEvent(EventType::Enum type, bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    switch (type) {
    case EventType::e_ACK: {
        // For ACK, we don't have any bytes value, but we also wouldn't care ..
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_ACK, 1);
    } break;
    case EventType::e_ACK_TIME: {
        d_statContext_mp->reportValue(DomainQueueStats::e_STAT_ACK_TIME,
                                      value);
    } break;
    case EventType::e_NACK: {
        // For NACK, we don't care about the bytes value ..
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_NACK, 1);
    } break;
    case EventType::e_CONFIRM: {
        // For CONFIRM, we don't care about the bytes value ..
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_CONFIRM, 1);
    } break;
    case EventType::e_REJECT: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_REJECT, 1);
    } break;
    case EventType::e_PUSH: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_PUSH, value);
    } break;
    case EventType::e_PUT: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_PUT, value);
    } break;
    case EventType::e_ADD_MESSAGE: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_BYTES, value);
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_MESSAGES, 1);
    } break;
    case EventType::e_DEL_MESSAGE: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_BYTES, -value);
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_MESSAGES, -1);
    } break;
    case EventType::e_GC_MESSAGE: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_GC_MSGS, value);
    } break;
    case EventType::e_PURGE: {
        // NOTE: Setting the value like that will cause weird results if using
        //       the stat to get rates
        d_statContext_mp->setValue(DomainQueueStats::e_STAT_BYTES, 0);
        d_statContext_mp->setValue(DomainQueueStats::e_STAT_MESSAGES, 0);
    } break;
    case EventType::e_CHANGE_ROLE: {
        d_statContext_mp->setValue(DomainQueueStats::e_STAT_ROLE, value);
    } break;
    case EventType::e_CFG_MSGS: {
        d_statContext_mp->setValue(DomainQueueStats::e_CFG_MSGS, value);
    } break;
    case EventType::e_CFG_BYTES: {
        d_statContext_mp->setValue(DomainQueueStats::e_CFG_BYTES, value);
    } break;
    case EventType::e_NO_SC_MESSAGE: {
        d_statContext_mp->adjustValue(DomainQueueStats::e_STAT_NO_SC_MSGS,
                                      value);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

void QueueStatsDomain::setQueueContentRaw(bsls::Types::Int64 messages,
                                          bsls::Types::Int64 bytes)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    d_statContext_mp->setValue(DomainQueueStats::e_STAT_BYTES, bytes);
    d_statContext_mp->setValue(DomainQueueStats::e_STAT_MESSAGES, messages);
}

void QueueStatsDomain::reportConfirmTime(bsls::Types::Int64 value,
                                         const bsl::string& appId)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    // Report `confirm time` metric to the queue context
    d_statContext_mp->reportValue(DomainQueueStats::e_STAT_CONFIRM_TIME,
                                  value);
    if (!d_subContexts_mp) {
        return;  // RETURN
    }

    // Report `confirm time` metric to corresponding appId subcontext
    bsl::list<StatSubContextMp>::iterator it = bsl::find_if(
        d_subContexts_mp->begin(),
        d_subContexts_mp->end(),
        ContextNameMatcher(appId));
    if (it != d_subContexts_mp->end()) {
        it->get()->reportValue(DomainQueueStats::e_STAT_CONFIRM_TIME, value);
    }
}

void QueueStatsDomain::reportQueueTime(bsls::Types::Int64 value,
                                       const bsl::string& appId)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    // Report `queue time` metric to the queue context
    d_statContext_mp->reportValue(DomainQueueStats::e_STAT_QUEUE_TIME, value);

    if (!d_subContexts_mp) {
        return;  // RETURN
    }

    // Report `queue time` metric to corresponding appId subcontext
    bsl::list<StatSubContextMp>::iterator it = bsl::find_if(
        d_subContexts_mp->begin(),
        d_subContexts_mp->end(),
        ContextNameMatcher(appId));
    if (it != d_subContexts_mp->end()) {
        it->get()->reportValue(DomainQueueStats::e_STAT_QUEUE_TIME, value);
    }
}

void QueueStatsDomain::updateDomainAppIds(
    const bsl::vector<bsl::string>& appIds)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator;

    if (!d_subContexts_mp) {
        return;
    }

    // Add subcontexts for appIds that are not already present
    for (bsl::vector<bsl::string>::const_iterator cit = appIds.begin();
         cit != appIds.end();
         ++cit) {
        if (bsl::find_if(d_subContexts_mp->begin(),
                         d_subContexts_mp->end(),
                         ContextNameMatcher(*cit)) ==
            d_subContexts_mp->end()) {
            d_subContexts_mp->push_back(d_statContext_mp->addSubcontext(
                mwcst::StatContextConfiguration(*cit, &localAllocator)));
        }
    }

    // Remove subcontexts if appIds are not present in updated AppIds
    bsl::list<StatSubContextMp>::iterator it = d_subContexts_mp->begin();
    while (it != d_subContexts_mp->end()) {
        if (bsl::find(appIds.begin(), appIds.end(), it->get()->name()) ==
            appIds.end()) {
            it = d_subContexts_mp->erase(it);
        }
        else {
            ++it;
        }
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
QueueStatsClient::getValue(const mwcst::StatContext& context,
                           int                       snapshotId,
                           const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const mwcst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const mwcst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::DMCST_DIRECT_VALUE, STAT),          \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::DMCST_DIRECT_VALUE, STAT),          \
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
                                  mwcst::StatContext* clientStatContext,
                                  bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize called twice");

    // Create subContext
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    d_statContext_mp = clientStatContext->addSubcontext(
        mwcst::StatContextConfiguration(uri.asString(), &localAllocator));
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

bsl::shared_ptr<mwcst::StatContext>
QueueStatsUtil::initializeStatContextDomains(int               historySize,
                                             bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_DOMAIN_STAT_NAME,
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
        .value("ack_time", mwcst::StatValue::DMCST_DISCRETE)
        .value("nack")
        .value("confirm")
        .value("confirm_time", mwcst::StatValue::DMCST_DISCRETE)
        .value("reject")
        .value("queue_time", mwcst::StatValue::DMCST_DISCRETE)
        .value("gc")
        .value("push")
        .value("put")
        .value("role")
        .value("cfg_msgs")
        .value("cfg_bytes")
        .value("content_msgs")
        .value("content_bytes");
    // NOTE: If the stats are using too much memory, we could reconsider
    //       nb_producer, nb_consumer, messages and bytes to be using atomic
    //       int and not stat value.

    return bsl::shared_ptr<mwcst::StatContext>(
        new (*allocator) mwcst::StatContext(config, allocator),
        allocator);
}

bsl::shared_ptr<mwcst::StatContext>
QueueStatsUtil::initializeStatContextClients(int               historySize,
                                             bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_CLIENT_STAT_NAME,
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

    return bsl::shared_ptr<mwcst::StatContext>(
        new (*allocator) mwcst::StatContext(config, allocator),
        allocator);
}

void QueueStatsUtil::initializeTableAndTipDomains(
    mwcst::Table*                 table,
    mwcu::BasicTableInfoProvider* tip,
    int                           historySize,
    mwcst::StatContext*           statContext)
{
    // Use only one level for now ...
    mwcst::StatValue::SnapshotLocation start(0, 0);
    mwcst::StatValue::SnapshotLocation end(0, historySize - 1);

    // Create table
    mwcst::TableSchema& schema = table->schema();

    schema.addDefaultIdColumn("id");
    schema.addColumn("nb_producer",
                     DomainQueueStats::e_STAT_NB_PRODUCER,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("nb_consumer",
                     DomainQueueStats::e_STAT_NB_CONSUMER,
                     mwcst::StatUtil::value,
                     start);

    schema.addColumn("messages",
                     DomainQueueStats::e_STAT_MESSAGES,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("bytes",
                     DomainQueueStats::e_STAT_BYTES,
                     mwcst::StatUtil::value,
                     start);

    schema.addColumn("put_msgs_delta",
                     DomainQueueStats::e_STAT_PUT,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("put_bytes_delta",
                     DomainQueueStats::e_STAT_PUT,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("put_msgs_abs",
                     DomainQueueStats::e_STAT_PUT,
                     mwcst::StatUtil::increments,
                     start);
    schema.addColumn("put_bytes_abs",
                     DomainQueueStats::e_STAT_PUT,
                     mwcst::StatUtil::value,
                     start);

    schema.addColumn("push_msgs_delta",
                     DomainQueueStats::e_STAT_PUSH,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("push_bytes_delta",
                     DomainQueueStats::e_STAT_PUSH,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("push_msgs_abs",
                     DomainQueueStats::e_STAT_PUSH,
                     mwcst::StatUtil::increments,
                     start);
    schema.addColumn("push_bytes_abs",
                     DomainQueueStats::e_STAT_PUSH,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("ack_delta",
                     DomainQueueStats::e_STAT_ACK,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("ack_abs",
                     DomainQueueStats::e_STAT_ACK,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("ack_time_avg",
                     DomainQueueStats::e_STAT_ACK_TIME,
                     mwcst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("ack_time_max",
                     DomainQueueStats::e_STAT_ACK_TIME,
                     mwcst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("nack_delta",
                     DomainQueueStats::e_STAT_NACK,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("nack_abs",
                     DomainQueueStats::e_STAT_NACK,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("confirm_delta",
                     DomainQueueStats::e_STAT_CONFIRM,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("confirm_abs",
                     DomainQueueStats::e_STAT_CONFIRM,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("confirm_time_avg",
                     DomainQueueStats::e_STAT_CONFIRM_TIME,
                     mwcst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("confirm_time_max",
                     DomainQueueStats::e_STAT_CONFIRM_TIME,
                     mwcst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("reject_delta",
                     DomainQueueStats::e_STAT_REJECT,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("reject_abs",
                     DomainQueueStats::e_STAT_REJECT,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("queue_time_avg",
                     DomainQueueStats::e_STAT_QUEUE_TIME,
                     mwcst::StatUtil::averagePerEvent,
                     start,
                     end);
    schema.addColumn("queue_time_max",
                     DomainQueueStats::e_STAT_QUEUE_TIME,
                     mwcst::StatUtil::rangeMax,
                     start,
                     end);
    schema.addColumn("gc_msgs_delta",
                     DomainQueueStats::e_STAT_GC_MSGS,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("gc_msgs_abs",
                     DomainQueueStats::e_STAT_GC_MSGS,
                     mwcst::StatUtil::value,
                     start);
    schema.addColumn("no_sc_msgs_delta",
                     DomainQueueStats::e_STAT_NO_SC_MSGS,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("no_sc_msgs_abs",
                     DomainQueueStats::e_STAT_NO_SC_MSGS,
                     mwcst::StatUtil::value,
                     start);

    // Configure records
    mwcst::TableRecords& records = table->records();
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
}

void QueueStatsUtil::initializeTableAndTipClients(
    mwcst::Table*                 table,
    mwcu::BasicTableInfoProvider* tip,
    int                           historySize,
    mwcst::StatContext*           statContext)
{
    // Use only one level for now ...
    mwcst::StatValue::SnapshotLocation start(0, 0);
    mwcst::StatValue::SnapshotLocation end(0, historySize - 1);

    // Create table
    mwcst::TableSchema& schema = table->schema();

    schema.addDefaultIdColumn("id");

    schema.addColumn("push_messages_delta",
                     ClientStats::e_STAT_PUSH,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("push_bytes_delta",
                     ClientStats::e_STAT_PUSH,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("push_messages_abs",
                     ClientStats::e_STAT_PUSH,
                     mwcst::StatUtil::increments,
                     start);
    schema.addColumn("push_bytes_abs",
                     ClientStats::e_STAT_PUSH,
                     mwcst::StatUtil::value,
                     start);

    schema.addColumn("put_messages_delta",
                     ClientStats::e_STAT_PUT,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("put_bytes_delta",
                     ClientStats::e_STAT_PUT,
                     mwcst::StatUtil::valueDifference,
                     start,
                     end);
    schema.addColumn("put_messages_abs",
                     ClientStats::e_STAT_PUT,
                     mwcst::StatUtil::increments,
                     start);
    schema.addColumn("put_bytes_abs",
                     ClientStats::e_STAT_PUT,
                     mwcst::StatUtil::value,
                     start);

    schema.addColumn("ack_delta",
                     ClientStats::e_STAT_ACK,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("ack_abs",
                     ClientStats::e_STAT_ACK,
                     mwcst::StatUtil::increments,
                     start);

    schema.addColumn("confirm_delta",
                     ClientStats::e_STAT_CONFIRM,
                     mwcst::StatUtil::incrementsDifference,
                     start,
                     end);
    schema.addColumn("confirm_abs",
                     ClientStats::e_STAT_CONFIRM,
                     mwcst::StatUtil::increments,
                     start);

    // Configure records
    mwcst::TableRecords& records = table->records();
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
