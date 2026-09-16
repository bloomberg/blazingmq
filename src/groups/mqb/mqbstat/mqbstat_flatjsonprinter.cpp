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
#include <mqbstat_clusterstats.h>
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
#include <bsls_types.h>

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

// ==================================
// class SparseDomainQueueStatsWriter
// ==================================

/// @brief Write the stats of one queue or queue application as a JSON object.
///
/// - Metrics equal to zero are omitted from the output.
/// - If all metrics are omitted, the JSON object is not written at all.
class SparseDomainQueueStatsWriter {
  private:
    // PRIVATE TYPES
    typedef mqbstat::QueueStatsDomain::Stat Stat;

  private:
    // DATA
    bsl::ostream&             d_os;
    bsl::string_view          d_prefix;
    bsl::string_view          d_domainName;
    bsl::string_view          d_queueName;
    bsl::string_view          d_appId;
    const bmqst::StatContext& d_ctx;

    /// Flag indicating if this writer has written anything.
    bool d_isDirty;

  private:
    // PRIVATE ACCESSORS

    /// @brief Write a JSON field.
    ///
    /// @param key The field name.
    /// @param val The field value.
    template <class KEY, class VAL>
    void wrap(const KEY& key, const VAL& val) const
    {
        d_os << ",\"" << key << "\":\"" << val << "\"";
    }

    // PRIVATE MANIPULATORS

    /// @brief Open the JSON object and write the prefix together with the
    /// queue identification fields.
    void lazyOpenJson()
    {
        d_os << "{" << d_prefix;
        if (d_appId.empty()) {
            wrap("stat", "queue");
            wrap("domain", d_domainName);
            wrap("queue", d_queueName);
        }
        else {
            wrap("stat", "queue_app");
            wrap("domain", d_domainName);
            wrap("queue", d_queueName);
            wrap("app", d_appId);
        }
    }

    /// @brief Write a JSON field for the given metric, unless its value is
    /// zero.  Open the JSON object on the first such field.
    ///
    /// @param stat The metric to write.
    void metric(Stat::Enum stat)
    {
        const bsls::Types::Int64 val =
            mqbstat::QueueStatsDomain::getValue(d_ctx, -1, stat);
        if (val != 0) {
            // This is the first time we see a non-zero metric, make sure to
            // open a JSON with the required prefix metrics and flip the flag.
            if (!d_isDirty) {
                lazyOpenJson();
                d_isDirty = true;
            }
            wrap(Stat::toString(stat), val);
        }
    }

  public:
    // CREATORS

    /// @brief Create a writer for one queue or queue application.
    ///
    /// @param os The stream to write to.
    /// @param prefix The fields opening the JSON object.
    /// @param domainName The name of the domain owning the queue.
    /// @param queueName The name of the queue.
    /// @param appId The application, or empty for the queue itself.
    /// @param ctx The stat context holding the metrics to write.
    explicit SparseDomainQueueStatsWriter(bsl::ostream&             os,
                                          bsl::string_view          prefix,
                                          bsl::string_view          domainName,
                                          bsl::string_view          queueName,
                                          bsl::string_view          appId,
                                          const bmqst::StatContext& ctx)
    : d_os(os)
    , d_prefix(prefix)
    , d_domainName(domainName)
    , d_queueName(queueName)
    , d_appId(appId)
    , d_ctx(ctx)
    , d_isDirty(false)
    {
        // NOTHING
    }

    // MANIPULATORS

    /// @brief Attempt to write the data provided to this writer if it has
    /// meaningful non-zero metrics.
    void write()
    {
        BSLS_ASSERT_SAFE(!d_isDirty && "'write' can only be invoked once");

        metric(Stat::e_NB_PRODUCER);
        metric(Stat::e_NB_CONSUMER);
        metric(Stat::e_MESSAGES_CURRENT);
        metric(Stat::e_MESSAGES_MAX);
        metric(Stat::e_MESSAGES_UTILIZATION_MAX);
        metric(Stat::e_BYTES_CURRENT);
        metric(Stat::e_BYTES_MAX);
        metric(Stat::e_BYTES_UTILIZATION_MAX);
        metric(Stat::e_PUT_MESSAGES_DELTA);
        metric(Stat::e_PUT_BYTES_DELTA);
        metric(Stat::e_PUT_MESSAGES_ABS);
        metric(Stat::e_PUT_BYTES_ABS);
        metric(Stat::e_PUSH_MESSAGES_DELTA);
        metric(Stat::e_PUSH_BYTES_DELTA);
        metric(Stat::e_PUSH_MESSAGES_ABS);
        metric(Stat::e_PUSH_BYTES_ABS);
        metric(Stat::e_ACK_DELTA);
        metric(Stat::e_ACK_ABS);
        metric(Stat::e_ACK_TIME_AVG);
        metric(Stat::e_ACK_TIME_MAX);
        metric(Stat::e_NACK_DELTA);
        metric(Stat::e_NACK_ABS);
        metric(Stat::e_CONFIRM_DELTA);
        metric(Stat::e_CONFIRM_ABS);
        metric(Stat::e_CONFIRM_TIME_AVG);
        metric(Stat::e_CONFIRM_TIME_MAX);
        metric(Stat::e_REJECT_ABS);
        metric(Stat::e_REJECT_DELTA);
        metric(Stat::e_QUEUE_TIME_AVG);
        metric(Stat::e_QUEUE_TIME_MAX);
        metric(Stat::e_GC_MSGS_DELTA);
        metric(Stat::e_GC_MSGS_ABS);
        metric(Stat::e_ROLE);
        metric(Stat::e_CFG_MSGS);
        metric(Stat::e_CFG_BYTES);
        metric(Stat::e_NO_SC_MSGS_DELTA);
        metric(Stat::e_NO_SC_MSGS_ABS);
        metric(Stat::e_HISTORY_ABS);

        // Close JSON object if it was lazily opened by `lazyOpenJson`.
        if (d_isDirty) {
            d_os << "}" << bsl::endl;
        }
    }
};

// =========================
// class DomainQueuesVisitor
// =========================

/// @brief Write one JSON object per visited queue stat context.
class DomainQueuesVisitor {
  private:
    // DATA
    bsl::ostream&    d_os;
    bsl::string_view d_prefix;

  public:
    // CREATORS

    /// @brief Create a visitor writing queue stats as JSON objects.
    ///
    /// @param os The stream to write to.
    /// @param prefix The fields opening every JSON object.
    explicit DomainQueuesVisitor(bsl::ostream& os, bsl::string_view prefix)
    : d_os(os)
    , d_prefix(prefix)
    {
        // NOTHING
    }

    // ACCESSORS

    /// @brief Write a JSON object holding the stats of one queue or queue
    /// application.
    ///
    /// @param domainName The name of the domain owning the queue.
    /// @param queueName The name of the queue.
    /// @param appId The application, or empty for the queue itself.
    /// @param ctx The stat context holding the metrics to write.
    void operator()(bsl::string_view          domainName,
                    bsl::string_view          queueName,
                    bsl::string_view          appId,
                    const bmqst::StatContext& ctx) const
    {
        SparseDomainQueueStatsWriter(d_os,
                                     d_prefix,
                                     domainName,
                                     queueName,
                                     appId,
                                     ctx)
            .write();
    }
};

// ===========================
// class ClusterStatsTraversal
// ===========================

/// Helper class for traversing cluster stat contexts within the "clusters"
/// top-level stat context.  Calls `onClusterStatsVisited` for each cluster
/// and each partition within that cluster.
class ClusterStatsTraversal {
  public:
    // PUBLIC TYPES
    typedef bsl::function<void(bsl::string_view          clusterName,
                               bsl::string_view          partitionName,
                               const bmqst::StatContext& ctx)>
        OnClusterStatsVisited;

  private:
    // DATA
    const bmqst::StatContext& d_ctx;

  public:
    // CREATORS

    /// Create a new traversal over the specified `clustersCtx`.
    explicit ClusterStatsTraversal(const bmqst::StatContext& clustersCtx);

    // ACCESSORS

    /// Iterate all clusters and partitions, calling the specified
    /// `onClusterStatsVisited` for each stat context found.  For
    /// cluster-level contexts, `partitionName` is empty.
    void
    forEachCluster(const OnClusterStatsVisited& onClusterStatsVisited) const;
};

inline ClusterStatsTraversal::ClusterStatsTraversal(
    const bmqst::StatContext& clustersCtx)
: d_ctx(clustersCtx)
{
    // NOTHING
}

inline void ClusterStatsTraversal::forEachCluster(
    const OnClusterStatsVisited& onClusterStatsVisited) const
{
    for (bmqst::StatContextIterator clusterIt = d_ctx.subcontextIterator();
         clusterIt;
         ++clusterIt) {
        const bsl::string_view clusterName = clusterIt->name();

        onClusterStatsVisited(clusterName, bsl::string_view(), *clusterIt);

        for (bmqst::StatContextIterator partIt =
                 clusterIt->subcontextIterator();
             partIt;
             ++partIt) {
            onClusterStatsVisited(clusterName, partIt->name(), *partIt);
        }
    }
}

// ======================
// class ClustersVisitor
// ======================

class ClustersVisitor {
  private:
    bsl::ostream&    d_os;
    bsl::string_view d_prefix;

    template <class KEY, class VAL>
    void wrap(const KEY& key, const VAL& val) const
    {
        d_os << ",\"" << key << "\":\"" << val << "\"";
    }

    void metric(const bmqst::StatContext&         ctx,
                mqbstat::ClusterStats::Stat::Enum stat) const
    {
        wrap(mqbstat::ClusterStats::Stat::toString(stat),
             mqbstat::ClusterStats::getValue(ctx, -1, stat));
    }

  public:
    explicit ClustersVisitor(bsl::ostream& os, bsl::string_view prefix)
    : d_os(os)
    , d_prefix(prefix)
    {
        // NOTHING
    }

    void operator()(bsl::string_view          clusterName,
                    bsl::string_view          partitionName,
                    const bmqst::StatContext& ctx) const
    {
        typedef mqbstat::ClusterStats::Stat Stat;

        d_os << "{" << d_prefix;
        if (partitionName.empty()) {
            wrap("stat", "cluster");
            wrap("cluster", clusterName);
            metric(ctx, Stat::e_CLUSTER_STATUS);
            metric(ctx, Stat::e_ROLE);
            metric(ctx, Stat::e_LEADER_STATUS);
            metric(ctx, Stat::e_CSL_REPLICATION_TIME_NS_AVG);
            metric(ctx, Stat::e_CSL_REPLICATION_TIME_NS_MAX);
            metric(ctx, Stat::e_CSL_LOG_OFFSET_BYTES);
            metric(ctx, Stat::e_CSL_WRITE_BYTES);
            metric(ctx, Stat::e_CSL_CFG_BYTES);
            metric(ctx, Stat::e_PARTITION_CFG_DATA_BYTES);
            metric(ctx, Stat::e_PARTITION_CFG_JOURNAL_BYTES);
        }
        else {
            wrap("stat", "cluster_partition");
            wrap("cluster", clusterName);
            wrap("partition", partitionName);
            metric(ctx, Stat::e_PARTITION_PRIMARY_STATUS);
            metric(ctx, Stat::e_PARTITION_ROLLOVER_TIME);
            metric(ctx, Stat::e_PARTITION_DATA_CONTENT);
            metric(ctx, Stat::e_PARTITION_JOURNAL_CONTENT);
            metric(ctx, Stat::e_PARTITION_DATA_OFFSET);
            metric(ctx, Stat::e_PARTITION_JOURNAL_OFFSET);
            metric(ctx, Stat::e_PARTITION_DATA_UTILIZATION_MAX);
            metric(ctx, Stat::e_PARTITION_JOURNAL_UTILIZATION_MAX);
            metric(ctx, Stat::e_PARTITION_SEQUENCE_NUMBER);
            metric(ctx, Stat::e_PARTITION_REPLICATION_TIME_NS_AVG);
            metric(ctx, Stat::e_PARTITION_REPLICATION_TIME_NS_MAX);
        }
        d_os << "}" << bsl::endl;
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

    // Clusters
    const bmqst::StatContext& clustersCtx =
        *d_contexts.find("clusters")->second;
    ClusterStatsTraversal clusterTraversal(clustersCtx);

    clusterTraversal.forEachCluster(ClustersVisitor(stream, commonPrefix));
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
