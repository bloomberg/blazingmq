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

// mqbstat_clusterstats.cpp                                           -*-C++-*-
#include <mqbstat_clusterstats.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_messages.h>
#include <mqbi_cluster.h>

#include <bmqst_statcontext.h>
#include <bmqst_statutil.h>
#include <bmqst_statvalue.h>

// BDE
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslmf_assert.h>
#include <bslmf_movableref.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all cluster's statistics)
static const char k_CLUSTER_STAT_NAME[] = "clusters";

/// Name of the stat context to create (holding all cluster node's
/// statistics)
static const char k_CLUSTER_NODES_STAT_NAME[] = "clusterNodes";

/// The default utilization value reported when we cannot
/// compute utilization.
const bsls::Types::Int64 k_UNDEFINED_UTILIZATION_VALUE = 0;

}  // close unnamed namespace

// ------------------
// class ClusterStats
// ------------------

bsls::Types::Int64 ClusterStats::getValue(const bmqst::StatContext& context,
                                          int                       snapshotId,
                                          const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const bmqst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const bmqst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE,                     \
                      ClusterStatsIndex::STAT),                               \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    bmqst::StatUtil::OPERATION(                                               \
        context.value(bmqst::StatContext::e_DIRECT_VALUE,                     \
                      ClusterStatsIndex::STAT),                               \
        latestSnapshot,                                                       \
        oldestSnapshot)

    switch (stat) {
    case Stat::e_CLUSTER_STATUS: {
        // We want to favor reporting 'unhealthiness', that is, if the cluster
        // was unhealthy for 'one snapshot' over the entire period, we should
        // report it as unhealthy, therefore we will report the maximum
        // snapshot value, and hence ensure that 'healthy < un-healty'.
        BSLMF_ASSERT(ClusterStatus::e_CLUSTER_STATUS_HEALTHY <
                     ClusterStatus::e_CLUSTER_STATUS_UNHEALTHY);

        return STAT_RANGE(rangeMax, e_CLUSTER_STATUS);
    }
    case Stat::e_ROLE: {
        return static_cast<Role::Enum>(
            (*context.datum())->theMap().find("role")->theInteger());
    }
    case Stat::e_LEADER_STATUS: {
        // Favor leader over non follower.
        BSLMF_ASSERT(LeaderStatus::e_FOLLOWER < LeaderStatus::e_LEADER);

        return STAT_RANGE(rangeMax, e_LEADER_STATUS);
    }
    case Stat::e_CSL_REPLICATION_TIME_NS_AVG: {
        const bsls::Types::Int64 value = STAT_RANGE(averagePerEvent,
                                                    e_CSL_REPLICATION_TIME_NS);
        return value == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0
                                                                       : value;
    }
    case Stat::e_CSL_REPLICATION_TIME_NS_MAX: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_CSL_REPLICATION_TIME_NS);
        return value == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0
                                                                       : value;
    }

    case Stat::e_CSL_LOG_OFFSET_BYTES: {
        return STAT_SINGLE(value, e_CSL_LOG_OFFSET_BYTES);
    }
    case Stat::e_CSL_WRITE_BYTES: {
        return STAT_RANGE(valueDifference, e_CSL_WRITE_BYTES);
    }
    case Stat::e_CSL_CFG_BYTES: {
        return STAT_SINGLE(value, e_CSL_CFG_BYTES);
    }

    case Stat::e_PARTITION_CFG_JOURNAL_BYTES: {
        return STAT_SINGLE(value, e_PARTITION_CFG_JOURNAL_BYTES);
    }
    case Stat::e_PARTITION_CFG_DATA_BYTES: {
        return STAT_SINGLE(value, e_PARTITION_CFG_DATA_BYTES);
    }
    case Stat::e_PARTITION_PRIMARY_STATUS: {
        // Favor primary over replica.
        BSLMF_ASSERT(PartitionStats::PrimaryStatus::e_REPLICA <
                     PartitionStats::PrimaryStatus::e_PRIMARY);

        return STAT_RANGE(rangeMax, e_PRIMARY_STATUS);
    }
    case Stat::e_PARTITION_ROLLOVER_TIME: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_PARTITION_ROLLOVER_TIME);
        return value == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0
                                                                       : value;
    }
    case Stat::e_PARTITION_DATA_CONTENT: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_PARTITION_DATA_BYTES);
        return value == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0
                                                                       : value;
    }
    case Stat::e_PARTITION_JOURNAL_CONTENT: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_PARTITION_JOURNAL_BYTES);
        return value == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0
                                                                       : value;
    }
    case Stat::e_PARTITION_DATA_OFFSET: {
        return STAT_SINGLE(value, e_PARTITION_DATA_OFFSET_BYTES);
    }
    case Stat::e_PARTITION_JOURNAL_OFFSET: {
        return STAT_SINGLE(value, e_PARTITION_JOURNAL_OFFSET_BYTES);
    }
    case Stat::e_PARTITION_DATA_UTILIZATION_MAX: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_PARTITION_DATA_BYTES);
        if (value == bsl::numeric_limits<bsls::Types::Int64>::min()) {
            return k_UNDEFINED_UTILIZATION_VALUE;
        }
        const bsls::Types::Int64 limit =
            STAT_SINGLE(value, e_PARTITION_CFG_DATA_BYTES);
        BSLS_ASSERT_SAFE(limit != 0);
        return 100 * value / limit;
    }
    case Stat::e_PARTITION_JOURNAL_UTILIZATION_MAX: {
        const bsls::Types::Int64 value = STAT_RANGE(rangeMax,
                                                    e_PARTITION_JOURNAL_BYTES);
        if (value == bsl::numeric_limits<bsls::Types::Int64>::min()) {
            return k_UNDEFINED_UTILIZATION_VALUE;
        }
        const bsls::Types::Int64 limit =
            STAT_SINGLE(value, e_PARTITION_CFG_JOURNAL_BYTES);
        BSLS_ASSERT_SAFE(limit != 0);
        return 100 * value / limit;
    }
    case Stat::e_PARTITION_SEQUENCE_NUMBER: {
        return STAT_SINGLE(value, e_PARTITION_SEQUENCE_NUMBER);
    }
    case Stat::e_PARTITION_REPLICATION_TIME_NS_AVG: {
        const bsls::Types::Int64 value =
            STAT_RANGE(averagePerEvent, e_PARTITION_REPLICATION_TIME_NS);
        return value == bsl::numeric_limits<bsls::Types::Int64>::max() ? 0
                                                                       : value;
    }
    case Stat::e_PARTITION_REPLICATION_TIME_NS_MAX: {
        const bsls::Types::Int64 value =
            STAT_RANGE(rangeMax, e_PARTITION_REPLICATION_TIME_NS);
        return value == bsl::numeric_limits<bsls::Types::Int64>::min() ? 0
                                                                       : value;
    }

    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;
#undef STAT_RANGE
#undef STAT_SINGLE
}

bsl::shared_ptr<PartitionStats>
ClusterStats::getPartitionStats(int partitionId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(0 <= partitionId &&
                    partitionId < static_cast<int>(d_partitionsStats.size()));
    const bsl::shared_ptr<PartitionStats>& partitionStats_sp =
        d_partitionsStats[partitionId];
    BSLS_ASSERT_OPT(partitionStats_sp->statContext() &&
                    "initialize was not called");
    return partitionStats_sp;
}

ClusterStats::ClusterStats(bslma::Allocator* allocator)
: d_statContext_mp(0)
, d_partitionsStats(allocator)
{
    // NOTHING
}

void ClusterStats::initialize(const bsl::string&  name,
                              int                 partitionsCount,
                              bmqst::StatContext* clusterStatContext,
                              bslma::Allocator*   allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_statContext_mp && "initialize was already called");

    // Ensure the datum is created, as a map with the appropriate keys.  Note
    // that the value set here (especially for 'role')' is not relevant as the
    // proper setters will be called right after by the appropriate higher
    // level components.
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);
    d_statContext_mp = clusterStatContext->addSubcontext(
        bmqst::StatContextConfiguration(name, &localAllocator));

    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bslma::Allocator*     alloc = d_statContext_mp->datumAllocator();
    bdld::DatumMapBuilder builder(alloc);
    builder.pushBack("role", bdld::Datum::createInteger(Role::e_PROXY));
    builder.pushBack("upstream", bdld::Datum::copyString("", alloc));

    datum->adopt(builder.commit());

    // Create one child per partition
    d_partitionsStats.reserve(partitionsCount);
    for (int pId = 0; pId < partitionsCount; ++pId) {
        const bsl::string partitionName = "partition" + bsl::to_string(pId);
        bsl::shared_ptr<bmqst::StatContext> statContext_sp(
            d_statContext_mp->addSubcontext(
                bmqst::StatContextConfiguration(partitionName,
                                                &localAllocator)),
            allocator);
        bsl::shared_ptr<PartitionStats> partitionStats_sp;
        partitionStats_sp.createInplace(allocator, statContext_sp);
        partitionStats_sp->setNodeRole(
            PartitionStats::PrimaryStatus::e_UNKNOWN);
        d_partitionsStats.emplace_back(
            bslmf::MovableRefUtil::move(partitionStats_sp));
    }
}

void ClusterStats::setIsMember(bool value)
{
    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bslma::Allocator*     alloc = d_statContext_mp->datumAllocator();
    bdld::DatumMapBuilder builder(alloc);

    builder.pushBack("role",
                     bdld::Datum::createInteger(value ? Role::e_MEMBER
                                                      : Role::e_PROXY));

    // Copy old, unchanged values
    builder.pushBack("upstream",
                     bdld::Datum::copyString(
                         (*datum)->theMap().find("upstream")->theString(),
                         alloc));

    datum->adopt(builder.commit());
}

void ClusterStats::setUpstream(const bsl::string& value)
{
    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bslma::Allocator*     alloc = d_statContext_mp->datumAllocator();
    bdld::DatumMapBuilder builder(alloc);

    builder.pushBack("upstream", bdld::Datum::copyString(value, alloc));

    // Copy old, unchanged values
    builder.pushBack("role",
                     bdld::Datum::createInteger(
                         (*datum)->theMap().find("role")->theInteger()));

    datum->adopt(builder.commit());
}

void ClusterStats::setPartitionCfgBytes(bsls::Types::Int64 dataBytes,
                                        bsls::Types::Int64 journalBytes,
                                        bsls::Types::Int64 cslBytes)
{
    d_statContext_mp->setValue(ClusterStatsIndex::e_PARTITION_CFG_DATA_BYTES,
                               dataBytes);
    d_statContext_mp->setValue(
        ClusterStatsIndex::e_PARTITION_CFG_JOURNAL_BYTES,
        journalBytes);
    d_statContext_mp->setValue(ClusterStatsIndex::e_CSL_CFG_BYTES, cslBytes);
    bsl::vector<bsl::shared_ptr<PartitionStats> >::const_iterator it =
        d_partitionsStats.cbegin();
    for (; it != d_partitionsStats.cend(); ++it) {
        (*it)->statContext()->setValue(
            ClusterStatsIndex::e_PARTITION_CFG_DATA_BYTES,
            dataBytes);
        (*it)->statContext()->setValue(
            ClusterStatsIndex::e_PARTITION_CFG_JOURNAL_BYTES,
            journalBytes);
    }
}

// --------------------
// class PartitionStats
// --------------------

PartitionStats::PartitionStats(
    const bsl::shared_ptr<bmqst::StatContext>& statContext)
: d_statContext_sp(statContext)
{
    // NOTHING
}

// -------------------------
// struct ClusterStats::Role
// -------------------------

bsl::ostream& ClusterStats::Role::print(bsl::ostream& stream,
                                        Enum          value,
                                        int           level,
                                        int           spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);

    stream << toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ClusterStats::Role::toAscii(Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(MEMBER)
        CASE(PROXY)
    default: return "(* UNKNOWN *)";
    }

#undef case
}

// ----------------------
// class ClusterNodeStats
// ----------------------

bsls::Types::Int64
ClusterNodeStats::getValue(const bmqst::StatContext& context,
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
    case Stat::e_PUSH_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference,
                          ClusterNodeStatsIndex::e_STAT_PUSH);
    }
    case Stat::e_PUT_MESSAGES_DELTA: {
        return STAT_RANGE(incrementsDifference,
                          ClusterNodeStatsIndex::e_STAT_PUT);
    }
    case Stat::e_ACK_DELTA: {
        return STAT_RANGE(incrementsDifference,
                          ClusterNodeStatsIndex::e_STAT_ACK);
    }
    case Stat::e_CONFIRM_DELTA: {
        return STAT_RANGE(incrementsDifference,
                          ClusterNodeStatsIndex::e_STAT_CONFIRM);
    }
    case Stat::e_PUSH_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, ClusterNodeStatsIndex::e_STAT_PUSH);
    }
    case Stat::e_PUT_BYTES_DELTA: {
        return STAT_RANGE(valueDifference, ClusterNodeStatsIndex::e_STAT_PUT);
    }
    case Stat::e_PUSH_MESSAGES_ABS: {
        return STAT_SINGLE(increments, ClusterNodeStatsIndex::e_STAT_PUSH);
    }
    case Stat::e_PUT_MESSAGES_ABS: {
        return STAT_SINGLE(increments, ClusterNodeStatsIndex::e_STAT_PUT);
    }
    case Stat::e_ACK_ABS: {
        return STAT_SINGLE(increments, ClusterNodeStatsIndex::e_STAT_ACK);
    }
    case Stat::e_CONFIRM_ABS: {
        return STAT_SINGLE(increments, ClusterNodeStatsIndex::e_STAT_CONFIRM);
    }
    case Stat::e_PUSH_BYTES_ABS: {
        return STAT_SINGLE(value, ClusterNodeStatsIndex::e_STAT_PUSH);
    }
    case Stat::e_PUT_BYTES_ABS: {
        return STAT_SINGLE(value, ClusterNodeStatsIndex::e_STAT_PUT);
    }
    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;

#undef STAT_RANGE
#undef STAT_SINGLE
}

ClusterNodeStats::ClusterNodeStats()
: d_statContext_mp(0)
{
    // NOTHING
}

void ClusterNodeStats::initialize(const bmqt::Uri&    uri,
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

// ----------------------
// class ClusterStatsUtil
// ----------------------

bsl::shared_ptr<bmqst::StatContext>
ClusterStatsUtil::initializeStatContextCluster(int               historySize,
                                               bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_CLUSTER_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("cluster_status")
        .value("cluster_leader")
        .value("cluster_csl_replication_time_ns", bmqst::StatValue::e_DISCRETE)
        .value("cluster_csl_offset_bytes")
        .value("cluster_csl_write_bytes")
        .value("cluster_csl_cfg_bytes")
        .value("cluster.partition.cfg_data_bytes")
        .value("cluster.partition.cfg_journal_bytes")
        .value("partition_status")
        .value("partition.rollover_time", bmqst::StatValue::e_DISCRETE)
        .value("partition.data_bytes", bmqst::StatValue::e_DISCRETE)
        .value("partition.journal_bytes", bmqst::StatValue::e_DISCRETE)
        .value("partition.data_offset_bytes")
        .value("partition.journal_offset_bytes")
        .value("partition.sequence_number")
        .value("partition.replication_time_ns", bmqst::StatValue::e_DISCRETE);

    // NOTE: For the clusters, the stat context will have two levels of
    //       children, first level is per cluster, and second level is per
    //       partition in the cluster.  For convenience, we configure the stat
    //       context at the root, meaning that not all columns will be relevant
    //       to all rows, but this is ok, the drawback is a slight waste of
    //       memory - which is acceptable here since a broker will not have
    //       thousands of clusters and partitions.

    return bsl::shared_ptr<bmqst::StatContext>(
        new (*allocator) bmqst::StatContext(config, allocator),
        allocator);
}

bsl::shared_ptr<bmqst::StatContext>
ClusterStatsUtil::initializeStatContextClusterNodes(
    int               historySize,
    bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    bmqst::StatContextConfiguration config(k_CLUSTER_NODES_STAT_NAME,
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

}  // close package namespace
}  // close enterprise namespace
