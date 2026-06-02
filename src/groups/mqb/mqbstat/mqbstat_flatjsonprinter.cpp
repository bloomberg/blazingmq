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

#include <mqbstat_flatjsonprinter.h>

#include <mqbscm_version.h>

// MQB
#include <mqbstat_queuestats.h>

// BDE
#include <ball_log.h>
#include <bdlt_currenttime.h>
#include <bdlt_datetime.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
#include <bsl_sstream.h>
#include <bsl_string_view.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

// ================================
// class DomainQueueStatsTraversal
// ================================

/// Helper class for traversing queue stat contexts within the
/// "domainQueues" top-level stat context.  Calls `onQueueStatsVisited`
/// for each queue.
class DomainQueueStatsTraversal {
  public:
    // PUBLIC TYPES
    typedef bsl::function<void(bsl::string_view          domainName,
                               bsl::string_view          queueName,
                               bsl::string_view          appId,
                               const bmqst::StatContext& ctx)>
        OnQueueStatsVisited;

  private:
    // DATA
    const bmqst::StatContext& d_ctx;

  public:
    // CREATORS

    /// Create a new traversal over the specified `domainQueuesCtx`.
    explicit DomainQueueStatsTraversal(
        const bmqst::StatContext& domainQueuesCtx);

    // ACCESSORS

    /// Iterate all domains and queues, calling the specified
    /// `onQueueStatsVisited` for each queue stat context found.
    void forEachQueue(const OnQueueStatsVisited& onQueueStatsVisited) const;
};

inline DomainQueueStatsTraversal::DomainQueueStatsTraversal(
    const bmqst::StatContext& domainQueuesCtx)
: d_ctx(domainQueuesCtx)
{
    // NOTHING
}

inline void DomainQueueStatsTraversal::forEachQueue(
    const OnQueueStatsVisited& onQueueStatsVisited) const
{
    for (bmqst::StatContextIterator domainIt = d_ctx.subcontextIterator();
         domainIt;
         ++domainIt) {
        const bsl::string_view domainName = domainIt->name();

        for (bmqst::StatContextIterator queueIt =
                 domainIt->subcontextIterator();
             queueIt;
             ++queueIt) {
            const bsl::string_view uri = queueIt->name();
            const size_t           pos = uri.find_last_of("/");

            // fallback to full uri if there is a problem with queue name
            const bsl::string_view queueName = (pos != bsl::string::npos &&
                                                pos + 1 < uri.length())
                                                   ? uri.substr(pos + 1)
                                                   : uri;

            onQueueStatsVisited(domainName,
                                queueName,
                                bsl::string_view(),
                                *queueIt);

            for (bmqst::StatContextIterator appIdIt =
                     queueIt->subcontextIterator();
                 appIdIt;
                 ++appIdIt) {
                onQueueStatsVisited(domainName,
                                    queueName,
                                    appIdIt->name(),
                                    *appIdIt);
            }
        }
    }
}

// =========================
// class DomainQueuesVisitor
// =========================

struct DomainQueuesVisitor {
    bsl::ostream&    d_os;
    bsl::string_view d_prefix;

    explicit DomainQueuesVisitor(bsl::ostream& os, bsl::string_view prefix)
    : d_os(os)
    , d_prefix(prefix)
    {
        // NOTHING
    }

    void operator()(bsl::string_view          domainName,
                    bsl::string_view          queueName,
                    bsl::string_view          appId,
                    const bmqst::StatContext& ctx) const
    {
#define WRAP(KEY, VAL) ",\"" << (KEY) << "\":\"" << (VAL) << "\""

#define METRIC(STAT)                                                          \
    WRAP(mqbstat::QueueStatsDomain::Stat::toString(STAT),                     \
         mqbstat::QueueStatsDomain::getValue(ctx, -1, (STAT)))

        typedef mqbstat::QueueStatsDomain::Stat Stat;

        d_os << "{" << d_prefix;
        if (appId.empty()) {
            d_os << WRAP("type", "queue");
            d_os << WRAP("domain", domainName);
            d_os << WRAP("queue", queueName);
            // no `app`
        }
        else {
            d_os << WRAP("type", "queue_app");
            d_os << WRAP("domain", domainName);
            d_os << WRAP("queue", queueName);
            d_os << WRAP("app", appId);
        }
        d_os << METRIC(Stat::e_NB_PRODUCER);
        d_os << METRIC(Stat::e_NB_CONSUMER);
        d_os << METRIC(Stat::e_MESSAGES_CURRENT);
        d_os << METRIC(Stat::e_MESSAGES_MAX);
        d_os << METRIC(Stat::e_MESSAGES_UTILIZATION_MAX);
        d_os << METRIC(Stat::e_BYTES_CURRENT);
        d_os << METRIC(Stat::e_BYTES_MAX);
        d_os << METRIC(Stat::e_BYTES_UTILIZATION_MAX);
        d_os << METRIC(Stat::e_PUT_MESSAGES_DELTA);
        d_os << METRIC(Stat::e_PUT_BYTES_DELTA);
        d_os << METRIC(Stat::e_PUT_MESSAGES_ABS);
        d_os << METRIC(Stat::e_PUT_BYTES_ABS);
        d_os << METRIC(Stat::e_PUSH_MESSAGES_DELTA);
        d_os << METRIC(Stat::e_PUSH_BYTES_DELTA);
        d_os << METRIC(Stat::e_PUSH_MESSAGES_ABS);
        d_os << METRIC(Stat::e_PUSH_BYTES_ABS);
        d_os << METRIC(Stat::e_ACK_DELTA);
        d_os << METRIC(Stat::e_ACK_ABS);
        d_os << METRIC(Stat::e_ACK_TIME_AVG);
        d_os << METRIC(Stat::e_ACK_TIME_MAX);
        d_os << METRIC(Stat::e_NACK_DELTA);
        d_os << METRIC(Stat::e_NACK_ABS);
        d_os << METRIC(Stat::e_CONFIRM_DELTA);
        d_os << METRIC(Stat::e_CONFIRM_ABS);
        d_os << METRIC(Stat::e_CONFIRM_TIME_AVG);
        d_os << METRIC(Stat::e_CONFIRM_TIME_MAX);
        d_os << METRIC(Stat::e_REJECT_ABS);
        d_os << METRIC(Stat::e_REJECT_DELTA);
        d_os << METRIC(Stat::e_QUEUE_TIME_AVG);
        d_os << METRIC(Stat::e_QUEUE_TIME_MAX);
        d_os << METRIC(Stat::e_GC_MSGS_DELTA);
        d_os << METRIC(Stat::e_GC_MSGS_ABS);
        d_os << METRIC(Stat::e_ROLE);
        d_os << METRIC(Stat::e_CFG_MSGS);
        d_os << METRIC(Stat::e_CFG_BYTES);
        d_os << METRIC(Stat::e_NO_SC_MSGS_DELTA);
        d_os << METRIC(Stat::e_NO_SC_MSGS_ABS);
        d_os << METRIC(Stat::e_HISTORY_ABS);
        d_os << "}" << bsl::endl;

#undef METRIC
#undef WRAP
    }
};

}  // close unnamed namespace

// ------------------------------------------
// class FlatJsonPrinter::FlatJsonPrinterImpl
// ------------------------------------------

/// The implementation class for FlatJsonPrinter, containing all the cached
/// options for printing statistics as JSON.  This implementation exists and is
/// hidden from the package include for the following reasons:
/// - Don't want to expose `bdljsn` names and symbols to the outer scope.
/// - Member fields and functions defined for this implementation are used only
///   locally, so there is no reason to make it visible.
class FlatJsonPrinter::FlatJsonPrinterImpl {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.FLATJSONPRINTERIMPL");

  private:
    // PRIVATE TYPES
    typedef FlatJsonPrinter::StatContextsMap StatContextsMap;

  private:
    // DATA
    /// StatContext-s map
    const StatContextsMap d_contexts;

    /// Allocator
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    FlatJsonPrinterImpl(const FlatJsonPrinterImpl& other) BSLS_KEYWORD_DELETED;
    FlatJsonPrinterImpl&
    operator=(const FlatJsonPrinterImpl& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FlatJsonPrinterImpl,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `FlatJsonPrinterImpl` object, using the specified
    /// `statContextsMap` and the specified `allocator`.
    explicit FlatJsonPrinterImpl(const StatContextsMap& statContextsMap,
                                 bslma::Allocator*      allocator);

    // MANIPULATORS

    /// Print the flat JSON-encoded stats to the specified `stream`, using
    /// the specified `statId` to identify the snapshot.
    ///
    /// THREAD: This method is called in the `snapshot` thread.
    void printStats(bsl::ostream& stream, int statId);
};

inline FlatJsonPrinter::FlatJsonPrinterImpl::FlatJsonPrinterImpl(
    const StatContextsMap& statContextsMap,
    bslma::Allocator*      allocator)
: d_contexts(statContextsMap, allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

inline void
FlatJsonPrinter::FlatJsonPrinterImpl::printStats(bsl::ostream& stream,
                                                 int           statId)
{
    // executed by the `snapshot` thread

    bdlt::Datetime     now = bdlt::CurrentTime::utc();
    bsl::ostringstream ts(d_allocator_p);
    ts << "\"ts\":\"" << now << "\",\"stat_id\":" << statId /* no ',' */;
    bsl::string commonPrefix(d_allocator_p);
    commonPrefix = ts.str();
    // `commonPrefix` is the same for all metrics.

    const bmqst::StatContext& ctx = *d_contexts.find("domainQueues")->second;
    DomainQueueStatsTraversal traversal(ctx);

    traversal.forEachQueue(DomainQueuesVisitor(stream, commonPrefix));
}

// ---------------------
// class FlatJsonPrinter
// ---------------------

FlatJsonPrinter::FlatJsonPrinter(const StatContextsMap& statContextsMap,
                                 bslma::Allocator*      allocator)
: d_impl_mp(bslma::ManagedPtrUtil::allocateManaged<FlatJsonPrinterImpl>(
      allocator,
      statContextsMap))
{
    // NOTHING
}

void FlatJsonPrinter::printStats(bsl::ostream& stream, int statId)
{
    // executed by the `snapshot` thread

    d_impl_mp->printStats(stream, statId);
}

}  // close package namespace
}  // close enterprise namespace
