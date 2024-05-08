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

// MWC
#include <mwcst_statcontext.h>
#include <mwcst_statutil.h>
#include <mwcst_statvalue.h>

// BDE
#include <bdld_datummapbuilder.h>
#include <bdld_manageddatum.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bslmf_assert.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

/// Name of the stat context to create (holding all cluster's statistics)
static const char k_CLUSTER_STAT_NAME[] = "clusters";

/// Name of the stat context to create (holding all cluster node's
/// statistics)
static const char k_CLUSTER_NODES_STAT_NAME[] = "clusterNodes";

//-------------------------
// struct ClusterStatsIndex
//-------------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients.
struct ClusterStatsIndex {
    enum Enum {
        // Cluster stats

        /// Value: Health status of cluster, 1 implies healthy and 2 implies
        ///        un-healthy
        e_CLUSTER_STATUS = 0,
        e_LEADER_STATUS
        // Value: Leader status of cluster, non-zero (1) implies leader
        //        and 0 implies follower
        ,
        e_PARTITION_CFG_DATA_BYTES
        // Value: Configured size of partitions' data file
        ,
        e_PARTITION_CFG_JOURNAL_BYTES
        // Value: Configured size of partitions' journal file
        // Per partition stats
        ,
        e_PRIMARY_STATUS
        // Value: The primary status of the partition, from the
        //        'PrimaryStatus::Enum' values.
        ,
        e_PARTITION_ROLLOVER_TIME
        // Value: Nanoseconds time it took for rolling over the partition.
        ,
        e_PARTITION_DATA_BYTES
        // Value: Outstanding bytes in the data file of the partition.
        ,
        e_PARTITION_JOURNAL_BYTES
        // Value: Outstanding bytes in the journal file of the partition.
    };
};

// ----------------------------
// struct ClusterNodeStatsIndex
// ----------------------------

/// Namespace for the constants of stat values that applies to the
/// cluster node
struct ClusterNodeStatsIndex {
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

//-------------------------
// struct ClusterStatsIndex
//-------------------------

/// Namespace for the constants of stat values that applies to the queues
/// from the clients
struct ClusterStatus {
    enum Enum { e_CLUSTER_STATUS_HEALTHY = 1, e_CLUSTER_STATUS_UNHEALTHY = 2 };
};

}  // close unnamed namespace

// ------------------
// class ClusterStats
// ------------------

bsls::Types::Int64 ClusterStats::getValue(const mwcst::StatContext& context,
                                          int                       snapshotId,
                                          const Stat::Enum&         stat)

{
    // invoked from the SNAPSHOT thread

    const mwcst::StatValue::SnapshotLocation latestSnapshot(0, 0);
    const mwcst::StatValue::SnapshotLocation oldestSnapshot(0, snapshotId);

#define STAT_SINGLE(OPERATION, STAT)                                          \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::e_DIRECT_VALUE,                     \
                      ClusterStatsIndex::STAT),                               \
        latestSnapshot)

#define STAT_RANGE(OPERATION, STAT)                                           \
    mwcst::StatUtil::OPERATION(                                               \
        context.value(mwcst::StatContext::e_DIRECT_VALUE,                     \
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
    case Stat::e_PARTITION_CFG_JOURNAL_BYTES: {
        return STAT_SINGLE(value, e_PARTITION_CFG_JOURNAL_BYTES);
    }
    case Stat::e_PARTITION_CFG_DATA_BYTES: {
        return STAT_SINGLE(value, e_PARTITION_CFG_DATA_BYTES);
    }
    case Stat::e_PARTITION_PRIMARY_STATUS: {
        // Favor primary over replica.
        BSLMF_ASSERT(PrimaryStatus::e_REPLICA < PrimaryStatus::e_PRIMARY);

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

    default: {
        BSLS_ASSERT_SAFE(false && "Attempting to access an unknown stat");
    }
    }

    return 0;
#undef STAT_RANGE
#undef STAT_SINGLE
}

ClusterStats::ClusterStats(bslma::Allocator* allocator)
: d_statContext_mp(0)
, d_partitionsStatContexts(allocator)
{
    // NOTHING
}

void ClusterStats::initialize(const bsl::string&  name,
                              int                 partitionsCount,
                              mwcst::StatContext* clusterStatContext,
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
        mwcst::StatContextConfiguration(name, &localAllocator));

    bslma::ManagedPtr<bdld::ManagedDatum> datum = d_statContext_mp->datum();
    bslma::Allocator*     alloc = d_statContext_mp->datumAllocator();
    bdld::DatumMapBuilder builder(alloc);
    builder.pushBack("role", bdld::Datum::createInteger(Role::e_PROXY));
    builder.pushBack("upstream", bdld::Datum::copyString("", alloc));

    datum->adopt(builder.commit());

    // Create one child per partition
    d_partitionsStatContexts.reserve(partitionsCount);
    for (int pId = 0; pId < partitionsCount; ++pId) {
        const bsl::string partitionName = "partition" + bsl::to_string(pId);
        d_partitionsStatContexts.emplace_back(
            bsl::shared_ptr<mwcst::StatContext>(
                d_statContext_mp->addSubcontext(
                    mwcst::StatContextConfiguration(partitionName,
                                                    &localAllocator))));
        setNodeRoleForPartition(pId, PrimaryStatus::e_UNKNOWN);
    }
}

void ClusterStats::onPartitionEvent(PartitionEventType::Enum type,
                                    int                      partitionId,
                                    bsls::Types::Int64       value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         static_cast<int>(d_partitionsStatContexts.size()));
    BSLS_ASSERT_SAFE(d_partitionsStatContexts[partitionId] &&
                     "initialize was not called");

    mwcst::StatContext* sc = d_partitionsStatContexts[partitionId].get();

    switch (type) {
    case PartitionEventType::e_PARTITION_ROLLOVER: {
        sc->reportValue(ClusterStatsIndex::e_PARTITION_ROLLOVER_TIME, value);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    }
}

ClusterStats& ClusterStats::setHealthStatus(bool value)
{
    d_statContext_mp->setValue(
        ClusterStatsIndex::e_CLUSTER_STATUS,
        value ? ClusterStatus::e_CLUSTER_STATUS_HEALTHY
              : ClusterStatus::e_CLUSTER_STATUS_UNHEALTHY);
    return *this;
}

ClusterStats& ClusterStats::setIsMember(bool value)
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

    return *this;
}

ClusterStats& ClusterStats::setUpstream(const bsl::string& value)
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

    return *this;
}

ClusterStats& ClusterStats::setIsLeader(LeaderStatus::Enum value)
{
    d_statContext_mp->setValue(ClusterStatsIndex::e_LEADER_STATUS, value);
    return *this;
}

ClusterStats&
ClusterStats::setPartitionCfgBytes(bsls::Types::Int64 dataBytes,
                                   bsls::Types::Int64 journalBytes)
{
    d_statContext_mp->setValue(ClusterStatsIndex::e_PARTITION_CFG_DATA_BYTES,
                               dataBytes);
    d_statContext_mp->setValue(
        ClusterStatsIndex::e_PARTITION_CFG_JOURNAL_BYTES,
        journalBytes);
    return *this;
}

ClusterStats& ClusterStats::setNodeRoleForPartition(int partitionId,
                                                    PrimaryStatus::Enum value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         static_cast<int>(d_partitionsStatContexts.size()));
    BSLS_ASSERT_SAFE(d_partitionsStatContexts[partitionId] &&
                     "initialize was not called");

    d_partitionsStatContexts[partitionId]->setValue(
        ClusterStatsIndex::e_PRIMARY_STATUS,
        value);
    return *this;
}

ClusterStats&
ClusterStats::setPartitionOutstandingBytes(int                partitionId,
                                           bsls::Types::Int64 dataBytes,
                                           bsls::Types::Int64 journalBytes)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(partitionId >= 0 &&
                     partitionId <
                         static_cast<int>(d_partitionsStatContexts.size()));
    BSLS_ASSERT_SAFE(d_partitionsStatContexts[partitionId] &&
                     "initialize was not called");

    d_partitionsStatContexts[partitionId]->reportValue(
        ClusterStatsIndex::e_PARTITION_DATA_BYTES,
        dataBytes);
    d_partitionsStatContexts[partitionId]->reportValue(
        ClusterStatsIndex::e_PARTITION_JOURNAL_BYTES,
        journalBytes);
    return *this;
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
ClusterNodeStats::getValue(const mwcst::StatContext& context,
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

void ClusterNodeStats::onEvent(EventType::Enum type, bsls::Types::Int64 value)
{
    BSLS_ASSERT_SAFE(d_statContext_mp && "initialize was not called");

    switch (type) {
    case EventType::e_ACK: {
        // For ACK, we don't have any bytes value, but we also wouldn't care ..
        d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_ACK, 1);
    } break;
    case EventType::e_CONFIRM: {
        // For CONFIRM, we don't care about the bytes value ..
        d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_CONFIRM,
                                      1);
    } break;
    case EventType::e_PUSH: {
        d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_PUSH,
                                      value);
    } break;
    case EventType::e_PUT: {
        d_statContext_mp->adjustValue(ClusterNodeStatsIndex::e_STAT_PUT,
                                      value);
    } break;
    default: {
        BSLS_ASSERT_SAFE(false && "Unknown event type");
    } break;
    };
}

// ----------------------
// class ClusterStatsUtil
// ----------------------

bsl::shared_ptr<mwcst::StatContext>
ClusterStatsUtil::initializeStatContextCluster(int               historySize,
                                               bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_CLUSTER_STAT_NAME,
                                           &localAllocator);
    config.isTable(true)
        .defaultHistorySize(historySize)
        .statValueAllocator(allocator)
        .storeExpiredSubcontextValues(true)
        .value("cluster_status")
        .value("cluster_leader")
        .value("cluster.partition.cfg_journal_bytes")
        .value("cluster.partition.cfg_data_bytes")
        .value("partition_status")
        .value("partition.rollover_time", mwcst::StatValue::e_DISCRETE)
        .value("partition.data_bytes", mwcst::StatValue::e_DISCRETE)
        .value("partition.journal_bytes", mwcst::StatValue::e_DISCRETE);

    // NOTE: For the clusters, the stat context will have two levels of
    //       children, first level is per cluster, and second level is per
    //       partition in the cluster.  For convenience, we configure the stat
    //       context at the root, meaning that not all columns will be relevant
    //       to all rows, but this is ok, the drawback is a slight waste of
    //       memory - which is acceptable here since a broker will not have
    //       thousands of clusters and partitions.

    return bsl::shared_ptr<mwcst::StatContext>(
        new (*allocator) mwcst::StatContext(config, allocator),
        allocator);
}

bsl::shared_ptr<mwcst::StatContext>
ClusterStatsUtil::initializeStatContextClusterNodes(
    int               historySize,
    bslma::Allocator* allocator)
{
    bdlma::LocalSequentialAllocator<2048> localAllocator(allocator);

    mwcst::StatContextConfiguration config(k_CLUSTER_NODES_STAT_NAME,
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

}  // close package namespace
}  // close enterprise namespace
