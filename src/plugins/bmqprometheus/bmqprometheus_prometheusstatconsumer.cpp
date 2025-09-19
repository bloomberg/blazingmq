// Copyright 2016-2023 Bloomberg Finance L.P.
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

#include <bmqprometheus_prometheusstatconsumer.h>

// PROMETHEUS
#include <bmqprometheus_version.h>

// MQB
#include <mqbstat_brokerstats.h>
#include <mqbstat_clusterstats.h>
#include <mqbstat_domainstats.h>
#include <mqbstat_queuestats.h>

// BMQ
#include <bmqio_statchannelfactory.h>
#include <bmqsys_statmonitor.h>
#include <bmqsys_threadutil.h>
#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_arrayutil.h>
#include <bdld_manageddatum.h>
#include <bdlf_bind.h>
#include <bdlt_currenttime.h>
#include <bsl_atomic.h>
#include <bsl_cstddef.h>
#include <bsl_exception.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslmt_condition.h>
#include <bslmt_mutex.h>
#include <bslmt_threadutil.h>
#include <bsls_performancehint.h>

// PROMETHEUS
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/gateway.h"
#include "prometheus/gauge.h"
#include "prometheus/labels.h"

namespace BloombergLP {
namespace bmqprometheus {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQBRKR.PROMETHEUSSTATCONSUMER");

const char* k_THREADNAME = "bmqPrometheusPush";

class Tagger {
  private:
    // DATA
    ::prometheus::Labels labels;

  public:
    // MANIPULATORS
    Tagger& setCluster(const bslstl::StringRef& value)
    {
        labels["Cluster"] = value;
        return *this;
    }

    Tagger& setDomain(const bslstl::StringRef& value)
    {
        labels["Domain"] = value;
        return *this;
    }

    Tagger& setTier(const bslstl::StringRef& value)
    {
        labels["Tier"] = value;
        return *this;
    }

    Tagger& setQueue(const bslstl::StringRef& value)
    {
        labels["Queue"] = value;
        return *this;
    }

    Tagger& setRole(const bslstl::StringRef& value)
    {
        labels["Role"] = value;
        return *this;
    }

    Tagger& setInstance(const bslstl::StringRef& value)
    {
        labels["Instance"] = value;
        return *this;
    }

    Tagger& setRemoteHost(const bslstl::StringRef& value)
    {
        labels["RemoteHost"] = value;
        return *this;
    }

    Tagger& setDataType(const bslstl::StringRef& value)
    {
        labels["DataType"] = value;
        return *this;
    }

    Tagger& setAppId(const bslstl::StringRef& value)
    {
        labels["AppId"] = value;
        return *this;
    }

    Tagger& setPort(const bslstl::StringRef& value)
    {
        labels["Port"] = value;
        return *this;
    }

    // ACCESSORS
    ::prometheus::Labels& getLabels() { return labels; }
};

bsl::unique_ptr<PrometheusStatExporter>
makeExporter(const mqbcfg::ExportMode::Value&          mode,
             const bsl::string&                        host,
             const bsl::size_t                         port,
             std::shared_ptr< ::prometheus::Registry>& registry);

}  // close unnamed namespace

// ----------------------------
// class PrometheusStatConsumer
// ----------------------------

void PrometheusStatConsumer::stopImpl()
{
    if (!d_isStarted || !d_prometheusStatExporter_p) {
        return;  // RETURN
    }
    d_prometheusStatExporter_p->stop();
    d_isStarted = false;
}

PrometheusStatConsumer::~PrometheusStatConsumer()
{
    stopImpl();
}

PrometheusStatConsumer::PrometheusStatConsumer(
    const StatContextsMap& statContextsMap,
    BSLA_UNUSED bslma::Allocator* allocator)
: d_contextsMap(statContextsMap)
, d_publishInterval(0)
, d_snapshotInterval(0)
, d_snapshotId(0)
, d_actionCounter(0)
, d_isStarted(false)
, d_prometheusRegistry_p(std::make_shared< ::prometheus::Registry>())
{
    // Initialize stat contexts
    d_systemStatContext_p       = getStatContext("system");
    d_brokerStatContext_p       = getStatContext("broker");
    d_clustersStatContext_p     = getStatContext("clusters");
    d_clusterNodesStatContext_p = getStatContext("clusterNodes");
    d_domainsStatContext_p      = getStatContext("domains");
    d_domainQueuesStatContext_p = getStatContext("domainQueues");
    d_clientStatContext_p       = getStatContext("clients");
    d_channelsStatContext_p     = getStatContext("channels");
}

int PrometheusStatConsumer::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    d_consumerConfig_p = mqbplug::StatConsumerUtil::findConsumerConfig(name());
    if (!d_consumerConfig_p) {
        BALL_LOG_ERROR << "Could not find config for StatConsumer '" << name()
                       << "'";
        return -1;  // RETURN
    }

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    d_publishInterval                = d_consumerConfig_p->publishInterval();
    d_snapshotInterval               = brkrCfg.stats().snapshotInterval();

    if (!isEnabled() || d_isStarted) {
        return 0;  // RETURN
    }

    setActionCounter();

    d_snapshotId              = static_cast<int>(d_publishInterval.seconds() /
                                    d_snapshotInterval.seconds());
    const auto& prometheusCfg = d_consumerConfig_p->prometheusSpecific();
    if (prometheusCfg.isNull()) {
        BALL_LOG_ERROR
            << "Could not find 'prometheusSpecific' section in the config";
        return -2;  // RETURN
    }

    if (!d_prometheusStatExporter_p) {
        d_prometheusStatExporter_p = makeExporter(prometheusCfg->mode(),
                                                  prometheusCfg->host(),
                                                  prometheusCfg->port(),
                                                  d_prometheusRegistry_p);
    }
    if (!d_prometheusStatExporter_p) {
        BALL_LOG_ERROR
            << "Could not create an instance of PrometheusStatExporter";
        return -3;  // RETURN
    }
    if (0 != d_prometheusStatExporter_p->start()) {
        BALL_LOG_ERROR << "Could not start the PrometheusStatExporter";
        return -4;  // RETURN
    }

    d_isStarted = true;
    return 0;
}

void PrometheusStatConsumer::stop()
{
    stopImpl();
}

void PrometheusStatConsumer::onSnapshot()
{
    // executed by the *SCHEDULER* thread of StatController
    if (!isEnabled()) {
        return;  // RETURN
    }

    if (--d_actionCounter != 0) {
        return;  // RETURN
    }

    setActionCounter();

    captureSystemStats();
    captureNetworkStats();
    captureBrokerStats();
    LeaderSet leaders;
    collectLeaders(&leaders);
    captureClusterStats(leaders);
    captureClusterPartitionsStats();
    captureDomainStats(leaders);
    captureQueueStats();

    d_prometheusStatExporter_p->onData();
}

void PrometheusStatConsumer::captureQueueStats()
{
    // Lookup the 'domainQueues' stat context
    // This is guaranteed to work because it was asserted in the ctor.
    const bmqst::StatContext& domainsStatContext =
        *d_domainQueuesStatContext_p;

    typedef mqbstat::QueueStatsDomain::Stat Stat;  // Shortcut

    for (bmqst::StatContextIterator domainIt =
             domainsStatContext.subcontextIterator();
         domainIt;
         ++domainIt) {
        for (bmqst::StatContextIterator queueIt =
                 domainIt->subcontextIterator();
             queueIt;
             ++queueIt) {
            bslma::ManagedPtr<bdld::ManagedDatum> mdSp = queueIt->datum();
            bdld::DatumMapRef                     map = mdSp->datum().theMap();

            const auto role = mqbstat::QueueStatsDomain::getValue(
                *queueIt,
                d_snapshotId,
                mqbstat::QueueStatsDomain::Stat::e_ROLE);

            Tagger tagger;
            tagger.setCluster(map.find("cluster")->theString())
                .setDomain(map.find("domain")->theString())
                .setTier(map.find("tier")->theString())
                .setQueue(map.find("queue")->theString())
                .setRole(mqbstat::QueueStatsDomain::Role::toAscii(
                    static_cast<mqbstat::QueueStatsDomain::Role::Enum>(role)))
                .setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
                .setDataType("host-data");

            const auto labels = tagger.getLabels();

            // Heartbeat metric
            {
                // This metric is *always* reported for every queue, so that
                // there is guarantee to always (i.e. at any point in time) be
                // a time series containing all the tags that can be leveraged
                // in Grafana.

                auto& heartbeatGauge = ::prometheus::BuildGauge()
                                           .Name("queue_heartbeat")
                                           .Register(*d_prometheusRegistry_p);
                heartbeatGauge.Add(labels).Set(0);
            }

            // Queue metrics
            {  // for scoping only
                static const DatapointDef defs[] = {
                    {"queue_producers_count", Stat::e_NB_PRODUCER, false},
                    {"queue_consumers_count", Stat::e_NB_CONSUMER, false},
                    {"queue_put_msgs", Stat::e_PUT_MESSAGES_DELTA, true},
                    {"queue_put_bytes", Stat::e_PUT_BYTES_DELTA, true},
                    {"queue_push_msgs", Stat::e_PUSH_MESSAGES_DELTA, true},
                    {"queue_push_bytes", Stat::e_PUSH_BYTES_DELTA, true},
                    {"queue_ack_msgs", Stat::e_ACK_DELTA, true},
                    {"queue_ack_time_avg", Stat::e_ACK_TIME_AVG, false},
                    {"queue_ack_time_max", Stat::e_ACK_TIME_MAX, false},
                    {"queue_nack_msgs", Stat::e_NACK_DELTA, true},
                    {"queue_confirm_msgs", Stat::e_CONFIRM_DELTA, true},
                    {"queue_confirm_time_avg",
                     Stat::e_CONFIRM_TIME_AVG,
                     false},
                    {"queue_confirm_time_max",
                     Stat::e_CONFIRM_TIME_MAX,
                     false}};

                for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                     dpIt != bdlb::ArrayUtil::end(defs);
                     ++dpIt) {
                    // If there are subcontexts, skip 'confirm_time_max'
                    // metric, it will be processed later.
                    if (static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                            dpIt->d_stat) == Stat::e_CONFIRM_TIME_MAX &&
                        queueIt->numSubcontexts() > 0) {
                        continue;
                    }

                    const bsls::Types::Int64 value =
                        mqbstat::QueueStatsDomain::getValue(
                            *queueIt,
                            d_snapshotId,
                            static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                                dpIt->d_stat));
                    updateMetric(dpIt, labels, value);
                }
            }

            // The following metrics only make sense to be reported from the
            // primary node only.
            if (role == mqbstat::QueueStatsDomain::Role::e_PRIMARY) {
                static const DatapointDef defs[] = {
                    {"queue_gc_msgs", Stat::e_GC_MSGS_DELTA, true},
                    {"queue_cfg_msgs", Stat::e_CFG_MSGS, false},
                    {"queue_cfg_bytes", Stat::e_CFG_BYTES, false},
                    {"queue_content_msgs_max", Stat::e_MESSAGES_MAX, false},
                    {"queue_msgs_utilization_max",
                     Stat::e_MESSAGES_UTILIZATION_MAX,
                     false},
                    {"queue_content_bytes_max", Stat::e_BYTES_MAX, false},
                    {"queue_bytes_utilization_max",
                     Stat::e_BYTES_UTILIZATION_MAX,
                     false},
                    {"queue_queue_time_avg", Stat::e_QUEUE_TIME_AVG, false},
                    {"queue_queue_time_max", Stat::e_QUEUE_TIME_MAX, false},
                    {"queue_reject_msgs", Stat::e_REJECT_DELTA, true},
                    {"queue_nack_noquorum_msgs",
                     Stat::e_NO_SC_MSGS_DELTA,
                     true},
                };

                for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                     dpIt != bdlb::ArrayUtil::end(defs);
                     ++dpIt) {
                    // If there are subcontexts, skip 'cqueue_time_max' metric,
                    // it will be processed later.
                    if (static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                            dpIt->d_stat) == Stat::e_QUEUE_TIME_MAX &&
                        queueIt->numSubcontexts() > 0) {
                        continue;
                    }

                    const bsls::Types::Int64 value =
                        mqbstat::QueueStatsDomain::getValue(
                            *queueIt,
                            d_snapshotId,
                            static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                                dpIt->d_stat));
                    updateMetric(dpIt, labels, value);
                }
            }

            // Add `appId` tag to metrics.

            // These per-appId metrics exist for both primary and replica
            static const DatapointDef defsCommon[] = {
                {"queue_confirm_time_max", Stat::e_CONFIRM_TIME_MAX, false},
            };

            // These per-appId metrics exist only for primary
            static const DatapointDef defsPrimary[] = {
                {"queue_queue_time_max", Stat::e_QUEUE_TIME_MAX, false},
                {"queue_content_msgs_max", Stat::e_MESSAGES_MAX, false},
                {"queue_content_bytes_max", Stat::e_BYTES_MAX, false},
            };
            for (bmqst::StatContextIterator appIdIt =
                     queueIt->subcontextIterator();
                 appIdIt;
                 ++appIdIt) {
                tagger.setAppId(appIdIt->name());
                const auto labels = tagger.getLabels();

                for (DatapointDefCIter dpIt =
                         bdlb::ArrayUtil::begin(defsCommon);
                     dpIt != bdlb::ArrayUtil::end(defsCommon);
                     ++dpIt) {
                    const bsls::Types::Int64 value =
                        mqbstat::QueueStatsDomain::getValue(
                            *appIdIt,
                            d_snapshotId,
                            static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                                dpIt->d_stat));
                    updateMetric(dpIt, labels, value);
                }

                if (role == mqbstat::QueueStatsDomain::Role::e_PRIMARY) {
                    for (DatapointDefCIter dpIt =
                             bdlb::ArrayUtil::begin(defsPrimary);
                         dpIt != bdlb::ArrayUtil::end(defsPrimary);
                         ++dpIt) {
                        const bsls::Types::Int64 value =
                            mqbstat::QueueStatsDomain::getValue(
                                *appIdIt,
                                d_snapshotId,
                                static_cast<
                                    mqbstat::QueueStatsDomain::Stat::Enum>(
                                    dpIt->d_stat));
                        updateMetric(dpIt, labels, value);
                    }
                }
            }
        }
    }
}

void PrometheusStatConsumer::captureSystemStats()
{
    bsl::vector<bsl::pair<bsl::string, double> > datapoints;

    const int k_NUM_SYS_STATS = 10;
    datapoints.reserve(k_NUM_SYS_STATS);

#define COPY_METRIC(TAIL, ACCESSOR)                                           \
    datapoints.emplace_back(                                                  \
        "brkr_system_" TAIL,                                                  \
        bmqsys::StatMonitorUtil::ACCESSOR(*d_systemStatContext_p,             \
                                          d_snapshotId));

    COPY_METRIC("cpu_sys", cpuSystem);
    COPY_METRIC("cpu_usr", cpuUser);
    COPY_METRIC("cpu_all", cpuAll);
    COPY_METRIC("mem_res", memResident);
    COPY_METRIC("mem_virt", memVirtual);
    COPY_METRIC("os_pagefaults_minor", minorPageFaults);
    COPY_METRIC("os_pagefaults_major", majorPageFaults);
    COPY_METRIC("os_swaps", numSwaps);
    COPY_METRIC("os_ctxswitch_voluntary", voluntaryContextSwitches);
    COPY_METRIC("os_ctxswitch_involuntary", involuntaryContextSwitches);

#undef COPY_METRIC

    Tagger tagger;
    tagger.setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
        .setDataType("host-data");

    for (bsl::vector<bsl::pair<bsl::string, double> >::iterator it =
             datapoints.begin();
         it != datapoints.end();
         ++it) {
        auto& gauge = ::prometheus::BuildGauge().Name(it->first).Register(
            *d_prometheusRegistry_p);
        gauge.Add(tagger.getLabels()).Set(it->second);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(datapoints.size() == k_NUM_SYS_STATS);
}

void PrometheusStatConsumer::captureNetworkStats()
{
    bsl::vector<bsl::pair<bsl::string, double> > datapoints;

    const int k_NUM_NETWORK_STATS = 4;
    datapoints.reserve(k_NUM_NETWORK_STATS);

    const bmqst::StatContext* localContext =
        d_channelsStatContext_p->getSubcontext("local");
    const bmqst::StatContext* remoteContext =
        d_channelsStatContext_p->getSubcontext("remote");
    // NOTE: Should be StatController::k_CHANNEL_STAT_*, but can't due to
    //       dependency limitations.

    Tagger tagger;
    tagger.setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
        .setDataType("host-data");

#define RETRIEVE_METRIC(TAIL, STAT, CONTEXT)                                  \
    datapoints.emplace_back("brkr_system_net_" TAIL,                          \
                            bmqio::StatChannelFactoryUtil::getValue(          \
                                *CONTEXT,                                     \
                                d_snapshotId,                                 \
                                bmqio::StatChannelFactoryUtil::Stat::STAT));

    RETRIEVE_METRIC("local_in_bytes", e_BYTES_IN_DELTA, localContext);
    RETRIEVE_METRIC("local_out_bytes", e_BYTES_OUT_DELTA, localContext);
    RETRIEVE_METRIC("remote_in_bytes", e_BYTES_IN_DELTA, remoteContext);
    RETRIEVE_METRIC("remote_out_bytes", e_BYTES_OUT_DELTA, remoteContext);

#undef RETRIEVE_METRIC

    for (bsl::vector<bsl::pair<bsl::string, double> >::iterator it =
             datapoints.begin();
         it != datapoints.end();
         ++it) {
        auto& counter = ::prometheus::BuildCounter().Name(it->first).Register(
            *d_prometheusRegistry_p);
        counter.Add(tagger.getLabels()).Increment(it->second);
    }

    auto reportConnections = [&](const bsl::string&        metricName,
                                 const bmqst::StatContext* context) {
        // In order to eliminate possible duplication of port contexts
        // aggregate them before posting
        bsl::unordered_map<bsl::string,
                           bsl::pair<bsls::Types::Int64, bsls::Types::Int64> >
            portMap;

        bmqst::StatContextIterator it = context->subcontextIterator();
        for (; it; ++it) {
            if (it->isDeleted()) {
                // As we iterate over 'living' sub contexts in the begining and
                // over deleted sub contexts in the end, we can just stop here.
                break;
            }
            tagger.setPort(bsl::to_string(it->id()));
            ::prometheus::BuildCounter()
                .Name("brkr_system_net_" + metricName + "_delta")
                .Register(*d_prometheusRegistry_p)
                .Add(tagger.getLabels())
                .Increment(static_cast<double>(
                    bmqio::StatChannelFactoryUtil::getValue(
                        *it,
                        d_snapshotId,
                        bmqio::StatChannelFactoryUtil::Stat::
                            e_CONNECTIONS_DELTA)));
            ::prometheus::BuildGauge()
                .Name("brkr_system_net_" + metricName)
                .Register(*d_prometheusRegistry_p)
                .Add(tagger.getLabels())
                .Set(static_cast<
                     double>(bmqio::StatChannelFactoryUtil::getValue(
                    *it,
                    d_snapshotId,
                    bmqio::StatChannelFactoryUtil::Stat::e_CONNECTIONS_ABS)));
        }
    };

    reportConnections("local_tcp_connections", localContext);
    reportConnections("remote_tcp_connections", remoteContext);

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(datapoints.size() == k_NUM_NETWORK_STATS);
}

void PrometheusStatConsumer::captureBrokerStats()
{
    typedef mqbstat::BrokerStats::Stat Stat;  // Shortcut

    static const DatapointDef defs[] = {
        {"brkr_summary_queues_count", Stat::e_QUEUE_COUNT, false},
        {"brkr_summary_clients_count", Stat::e_CLIENT_COUNT, false},
    };

    Tagger tagger;
    tagger.setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
        .setDataType("host-data");

    for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
         dpIt != bdlb::ArrayUtil::end(defs);
         ++dpIt) {
        const bsls::Types::Int64 value = mqbstat::BrokerStats::getValue(
            *d_brokerStatContext_p,
            d_snapshotId,
            static_cast<Stat::Enum>(dpIt->d_stat));
        updateMetric(dpIt, tagger.getLabels(), value);
    }
}

void PrometheusStatConsumer::collectLeaders(LeaderSet* leaders)
{
    for (bmqst::StatContextIterator clusterIt =
             d_clustersStatContext_p->subcontextIterator();
         clusterIt;
         ++clusterIt) {
        if (mqbstat::ClusterStats::getValue(
                *clusterIt,
                d_snapshotId,
                mqbstat::ClusterStats::Stat::e_LEADER_STATUS) ==
            mqbstat::ClusterStats::LeaderStatus::e_LEADER) {
            leaders->insert(clusterIt->name());
        }
    }
}

void PrometheusStatConsumer::captureClusterStats(const LeaderSet& leaders)
{
    const bmqst::StatContext& clustersStatContext = *d_clustersStatContext_p;

    for (bmqst::StatContextIterator clusterIt =
             clustersStatContext.subcontextIterator();
         clusterIt;
         ++clusterIt) {
        typedef mqbstat::ClusterStats::Stat Stat;  // Shortcut

        // scope
        {
            static const DatapointDef defs[] = {
                {"cluster_healthiness", Stat::e_CLUSTER_STATUS, false},
            };

            const mqbstat::ClusterStats::Role::Enum role =
                static_cast<mqbstat::ClusterStats::Role::Enum>(
                    mqbstat::ClusterStats::getValue(
                        *clusterIt,
                        d_snapshotId,
                        mqbstat::ClusterStats::Stat::e_ROLE));

            Tagger tagger;
            tagger.setCluster(clusterIt->name())
                .setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
                .setRole(mqbstat::ClusterStats::Role::toAscii(role))
                .setDataType("host-data");

            if (role == mqbstat::ClusterStats::Role::e_PROXY) {
                bslma::ManagedPtr<bdld::ManagedDatum> mdSp =
                    clusterIt->datum();
                bdld::DatumMapRef map = mdSp->datum().theMap();

                bslstl::StringRef upstream = map.find("upstream")->theString();
                tagger.setRemoteHost(upstream.isEmpty() ? "_none_" : upstream);
            }

            for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                 dpIt != bdlb::ArrayUtil::end(defs);
                 ++dpIt) {
                const bsls::Types::Int64 value =
                    mqbstat::ClusterStats::getValue(
                        *clusterIt,
                        d_snapshotId,
                        static_cast<Stat::Enum>(dpIt->d_stat));

                updateMetric(dpIt, tagger.getLabels(), value);
            }
        }

        if (leaders.find(clusterIt->name()) != leaders.end()) {
            static const DatapointDef defs[] = {
                {"cluster_partition_cfg_journal_bytes",
                 Stat::e_PARTITION_CFG_JOURNAL_BYTES,
                 false},
                {"cluster_partition_cfg_data_bytes",
                 Stat::e_PARTITION_CFG_DATA_BYTES,
                 false},
                {"cluster_csl_replication_time_ns_avg",
                 Stat::e_CSL_REPLICATION_TIME_NS_AVG,
                 false},
                {"cluster_csl_replication_time_ns_max",
                 Stat::e_CSL_REPLICATION_TIME_NS_MAX,
                 false},
                {"cluster_csl_offset_bytes", Stat::e_CSL_OFFSET_BYTES, false},
                {"cluster_csl_write_bytes", Stat::e_CSL_WRITE_BYTES, false},
                {"cluster_csl_cfg_bytes", Stat::e_CSL_CFG_BYTES, false},
            };

            Tagger tagger;
            tagger.setCluster(clusterIt->name())
                .setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
                .setDataType("global-data");

            for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                 dpIt != bdlb::ArrayUtil::end(defs);
                 ++dpIt) {
                const bsls::Types::Int64 value =
                    mqbstat::ClusterStats::getValue(
                        *clusterIt,
                        d_snapshotId,
                        static_cast<Stat::Enum>(dpIt->d_stat));

                updateMetric(dpIt, tagger.getLabels(), value);
            }
        }
    }
}

void PrometheusStatConsumer::captureClusterPartitionsStats()
{
    // Iterate over each cluster
    for (bmqst::StatContextIterator clusterIt =
             d_clustersStatContext_p->subcontextIterator();
         clusterIt;
         ++clusterIt) {
        // Iterate over each partition
        for (bmqst::StatContextIterator partitionIt =
                 clusterIt->subcontextIterator();
             partitionIt;
             ++partitionIt) {
            mqbstat::ClusterStats::PrimaryStatus::Enum primaryStatus =
                static_cast<mqbstat::ClusterStats::PrimaryStatus::Enum>(
                    mqbstat::ClusterStats::getValue(
                        *partitionIt,
                        d_snapshotId,
                        mqbstat::ClusterStats::Stat::
                            e_PARTITION_PRIMARY_STATUS));
            if (primaryStatus !=
                mqbstat::ClusterStats::PrimaryStatus::e_PRIMARY) {
                // Only report partition stats from the primary node
                continue;  // CONTINUE
            }

            // Generate the metric name from the partition name (e.g.,
            // 'cluster_partition1_rollover_time')
            const bsl::string prefix = "cluster_" + partitionIt->name() + "_";
            const bsl::string rollover_time = prefix + "rollover_time";
            const bsl::string journal_outstanding_bytes =
                prefix + "journal_outstanding_bytes";
            const bsl::string journal_utilization = prefix +
                                                    "journal_utilization_max";
            const bsl::string data_outstanding_bytes =
                prefix + "data_outstanding_bytes";
            const bsl::string data_utilization = prefix +
                                                 "data_utilization_max";

            const DatapointDef defs[] = {
                {rollover_time.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_ROLLOVER_TIME,
                 false},
                {journal_outstanding_bytes.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_JOURNAL_CONTENT,
                 false},
                {journal_utilization.c_str(),
                 mqbstat::ClusterStats::Stat::
                     e_PARTITION_JOURNAL_UTILIZATION_MAX,
                 false},
                {data_outstanding_bytes.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_DATA_CONTENT,
                 false},
                {data_utilization.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_DATA_UTILIZATION_MAX,
                 false}};

            Tagger tagger;
            tagger.setCluster(clusterIt->name())
                .setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
                .setDataType("global-data");

            for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                 dpIt != bdlb::ArrayUtil::end(defs);
                 ++dpIt) {
                const bsls::Types::Int64 value =
                    mqbstat::ClusterStats::getValue(
                        *partitionIt,
                        d_snapshotId,
                        static_cast<mqbstat::ClusterStats::Stat::Enum>(
                            dpIt->d_stat));
                updateMetric(dpIt, tagger.getLabels(), value);
            }
        }
    }
}

void PrometheusStatConsumer::captureDomainStats(const LeaderSet& leaders)
{
    const bmqst::StatContext& domainsStatContext = *d_domainsStatContext_p;

    typedef mqbstat::DomainStats::Stat Stat;  // Shortcut

    for (bmqst::StatContextIterator domainIt =
             domainsStatContext.subcontextIterator();
         domainIt;
         ++domainIt) {
        bslma::ManagedPtr<bdld::ManagedDatum> mdSp = domainIt->datum();
        bdld::DatumMapRef                     map  = mdSp->datum().theMap();

        const bslstl::StringRef clusterName = map.find("cluster")->theString();

        if (leaders.find(clusterName) == leaders.end()) {
            // is NOT leader
            continue;  // CONTINUE
        }

        Tagger tagger;
        tagger.setCluster(clusterName)
            .setDomain(map.find("domain")->theString())
            .setTier(map.find("tier")->theString())
            .setDataType("global-data");

        static const DatapointDef defs[] = {
            {"domain_cfg_msgs", Stat::e_CFG_MSGS, false},
            {"domain_cfg_bytes", Stat::e_CFG_BYTES, false},
            {"domain_queue_count", Stat::e_QUEUE_COUNT, false},
        };

        for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
             dpIt != bdlb::ArrayUtil::end(defs);
             ++dpIt) {
            const bsls::Types::Int64 value = mqbstat::DomainStats::getValue(
                *domainIt,
                d_snapshotId,
                static_cast<Stat::Enum>(dpIt->d_stat));

            updateMetric(dpIt, tagger.getLabels(), value);
        }
    }
}

void PrometheusStatConsumer::updateMetric(const DatapointDef*         def_p,
                                          const ::prometheus::Labels& labels,
                                          const bsls::Types::Int64    value)
{
    if (value != 0) {
        // To save metrics, only report non-null values
        if (def_p->d_isCounter) {
            auto& counter = ::prometheus::BuildCounter()
                                .Name(def_p->d_name)
                                .Register(*d_prometheusRegistry_p);
            counter.Add(labels).Increment(static_cast<double>(value));
        }
        else {
            auto& gauge = ::prometheus::BuildGauge()
                              .Name(def_p->d_name)
                              .Register(*d_prometheusRegistry_p);
            gauge.Add(labels).Set(static_cast<double>(value));
        }
    }
}

void PrometheusStatConsumer::setPublishInterval(
    bsls::TimeInterval publishInterval)
{
    // executed by the *SCHEDULER* thread of StatController

    // PRECONDITIONS
    BSLS_ASSERT(publishInterval.seconds() >= 0);
    BSLS_ASSERT(d_snapshotInterval.seconds() > 0);
    BSLS_ASSERT(publishInterval.seconds() % d_snapshotInterval.seconds() == 0);

    BALL_LOG_INFO << "Set PrometheusStatConsumer publish interval to "
                  << publishInterval;

    d_publishInterval = publishInterval;
    d_snapshotId      = static_cast<int>(d_publishInterval.seconds() /
                                    d_snapshotInterval.seconds());

    setActionCounter();
}

const bmqst::StatContext*
PrometheusStatConsumer::getStatContext(const char* name) const
{
    StatContextsMap::const_iterator cIt = d_contextsMap.find(name);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(cIt == d_contextsMap.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false &&
                         "StatContext not found in PrometheusStatConsumer");
        return 0;  // RETURN
    }
    return cIt->second;
}

void PrometheusStatConsumer::setActionCounter()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_snapshotInterval > 0);
    BSLS_ASSERT(d_publishInterval >= 0);
    BSLS_ASSERT(d_publishInterval.seconds() % d_snapshotInterval.seconds() ==
                0);

    d_actionCounter = static_cast<int>(d_publishInterval.seconds() /
                                       d_snapshotInterval.seconds());
}

// -----------------------------------------
// class PrometheusStatConsumerPluginFactory
// -----------------------------------------

// CREATORS
PrometheusStatConsumerPluginFactory::PrometheusStatConsumerPluginFactory()
{
    // NOTHING
}

PrometheusStatConsumerPluginFactory::~PrometheusStatConsumerPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<StatConsumer> PrometheusStatConsumerPluginFactory::create(
    const StatContextsMap& statContexts,
    const CommandProcessorFn& /*commandProcessor*/,
    bdlbb::BlobBufferFactory* /*bufferFactory*/,
    bslma::Allocator* allocator)
{
    allocator = bslma::Default::allocator(allocator);

    bslma::ManagedPtr<mqbplug::StatConsumer> result(
        new (*allocator) PrometheusStatConsumer(statContexts, allocator),
        allocator);
    return result;
}

// --------------------------------
// class PrometheusPullStatExporter
// --------------------------------

class PrometheusPullStatExporter : public PrometheusStatExporter {
    std::weak_ptr< ::prometheus::Registry>  d_registry_p;
    bsl::unique_ptr< ::prometheus::Exposer> d_exposer_p;
    bsl::string                             d_exposerEndpoint;

  public:
    PrometheusPullStatExporter(
        const bsl::string&                              host,
        const bsl::size_t                               port,
        const std::shared_ptr< ::prometheus::Registry>& registry)
    : d_registry_p(registry)
    {
        bsl::ostringstream endpoint;
        endpoint << host << ":" << port;
        d_exposerEndpoint = endpoint.str();
    }

    int start() override
    {
        try {
            d_exposer_p = bsl::make_unique< ::prometheus::Exposer>(
                d_exposerEndpoint);
            d_exposer_p->RegisterCollectable(d_registry_p);
            return 0;  // RETURN
        }
        catch (const bsl::exception& e) {
            BALL_LOG_WARN << "#PROMETHEUS_REPORTING "
                          << "Failed to start http server for Prometheus: "
                          << e.what();
            return -1;  // RETURN
        }
    }

    void stop() override { d_exposer_p.reset(); }
};

// --------------------------------
// class PrometheusPushStatExporter
// --------------------------------

class PrometheusPushStatExporter : public PrometheusStatExporter {
    bsl::unique_ptr< ::prometheus::Gateway> d_prometheusGateway_p;
    /// Handle of the Prometheus publishing thread
    bslmt::ThreadUtil::Handle d_prometheusPushThreadHandle;
    bslmt::Mutex              d_prometheusThreadMutex;
    bslmt::Condition          d_prometheusThreadCondition;
    bsl::atomic_bool          d_threadStop;

    /// Push gathered statistics to the push gateway in 'push' mode.
    /// THREAD: This method is called from the dedicated thread.
    void prometheusPushThread()
    {
        // executed by the dedicated prometheus push thread
        bmqsys::ThreadUtil::setCurrentThreadName(k_THREADNAME);

        BALL_LOG_INFO << "Prometheus Push thread has started [id: "
                      << bslmt::ThreadUtil::selfIdAsUint64() << "]";
        auto returnCode = 200;
        while (!d_threadStop) {
            bslmt::LockGuard<bslmt::Mutex> lock(&d_prometheusThreadMutex);
            d_prometheusThreadCondition.wait(&d_prometheusThreadMutex);
            auto newReturnCode = d_prometheusGateway_p->Push();
            if (newReturnCode != 200 && newReturnCode != returnCode) {
                BALL_LOG_WARN << "Push to Prometheus failed with code: "
                              << newReturnCode;
            }
            returnCode = newReturnCode;
        }

        BALL_LOG_INFO << "Prometheus Push thread terminated "
                      << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";
    }

    void stopImpl()
    {
        if (d_threadStop) {
            return;
        }
        d_threadStop = true;
        d_prometheusThreadCondition.signal();
        bslmt::ThreadUtil::join(d_prometheusPushThreadHandle);
    }

  public:
    PrometheusPushStatExporter(
        const bsl::string&                              host,
        const bsl::size_t&                              port,
        const std::shared_ptr< ::prometheus::Registry>& registry)
    : d_threadStop(false)
    {
        // create a push gateway
        const auto label = ::prometheus::Gateway::GetInstanceLabel(
            mqbcfg::BrokerConfig::get().hostName());
        d_prometheusGateway_p = bsl::make_unique< ::prometheus::Gateway>(
            host,
            bsl::to_string(port),
            "bmq",
            label);
        d_prometheusGateway_p->RegisterCollectable(registry);
    }

    ~PrometheusPushStatExporter() { stopImpl(); }

    void onData() override { d_prometheusThreadCondition.signal(); }

    int start() override
    {
        d_threadStop = false;
        // create push thread
        int rc = bslmt::ThreadUtil::create(
            &d_prometheusPushThreadHandle,
            bmqsys::ThreadUtil::defaultAttributes(),
            bdlf::BindUtil::bind(
                &PrometheusPushStatExporter::prometheusPushThread,
                this));
        if (rc != 0) {
            BALL_LOG_ERROR << "#PROMETHEUS_REPORTING "
                           << "Failed to start prometheusPushThread thread"
                           << "' [rc: " << rc << "]";
        }
        return rc;  // RETURN
    }

    void stop() override { stopImpl(); }
};

namespace {

bsl::unique_ptr<PrometheusStatExporter>
makeExporter(const mqbcfg::ExportMode::Value&          mode,
             const bsl::string&                        host,
             const bsl::size_t                         port,
             std::shared_ptr< ::prometheus::Registry>& registry)
{
    bsl::unique_ptr<PrometheusStatExporter> result;
    if (mode == mqbcfg::ExportMode::E_PULL) {
        result = bsl::make_unique<PrometheusPullStatExporter>(host,
                                                              port,
                                                              registry);
    }
    else if (mode == mqbcfg::ExportMode::E_PUSH) {
        result = bsl::make_unique<PrometheusPushStatExporter>(host,
                                                              port,
                                                              registry);
    }
    else {
        BALL_LOG_ERROR << "Wrong operation mode specified '" << mode << "'";
    }
    return result;
}

}  // close unnamed namespace

}  // close package namespace
}  // close enterprise namespace
