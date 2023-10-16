// plugins_prometheusstatconsumer.cpp                                 -*-C++-*-
#include <plugins_prometheusstatconsumer.h>

// PLUGINS
#include <plugins_version.h>

// MQB
#include <mqbstat_brokerstats.h>
#include <mqbstat_clusterstats.h>
#include <mqbstat_domainstats.h>
#include <mqbstat_queuestats.h>

// MWC
#include <mwcio_statchannelfactory.h>
#include <mwcsys_statmonitor.h>
#include <mwcsys_threadutil.h>
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_arrayutil.h>
#include <bdld_manageddatum.h>
#include <bdlf_bind.h>
#include <bsl_vector.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>

// PROMETHEUS
#include "prometheus/labels.h"

namespace BloombergLP {
namespace plugins {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQBRKR.PROMETHEUSSTATCONSUMER");

const char* k_THREADNAME = "bmqPrometheusPush";

class Tagger {
  private:
    // DATA
    prometheus::Labels labels;

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

    // ACCESSORS
    prometheus::Labels& getLabels() { return labels; }
};

}  // close unnamed namespace

// ----------------------------
// class PrometheusStatConsumer
// ----------------------------

PrometheusStatConsumer::~PrometheusStatConsumer()
{
    stop();
}

PrometheusStatConsumer::PrometheusStatConsumer(
    const StatContextsMap& statContextsMap,
    bslma::Allocator* /*allocator*/)
: d_contextsMap(statContextsMap)
, d_consumerConfig_p(mqbplug::StatConsumerUtil::findConsumerConfig(name()))
, d_publishInterval(0)
, d_snapshotId(0)
, d_actionCounter(0)
, d_isStarted(false)
, d_threadStop(false)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_publishInterval >= 0);

    // Initialize stat contexts
    d_systemStatContext_p       = getStatContext("system");
    d_brokerStatContext_p       = getStatContext("broker");
    d_clustersStatContext_p     = getStatContext("clusters");
    d_clusterNodesStatContext_p = getStatContext("clusterNodes");
    d_domainsStatContext_p      = getStatContext("domains");
    d_domainQueuesStatContext_p = getStatContext("domainQueues");
    d_clientStatContext_p       = getStatContext("clients");
    d_channelsStatContext_p     = getStatContext("channels");

    // Initialize Prometheus config
    const char* prometheusHost = "127.0.1.1";
    size_t      prometheusPort = 9091;
    d_prometheusMode           = "push";  // TODO enum or check validity?

    d_prometheusRegistry_p = std::make_shared<prometheus::Registry>();

    if (d_prometheusMode == "push") {
        bsl::string clientHostName =
            "clientHostName";  // TODO: do we need this? to recognise different
                               // sources of statistics?

        // Push mode
        const auto labels = prometheus::Gateway::GetInstanceLabel(
            clientHostName);  // creates label { "instance": clientHostName }
        d_prometheusGateway_p = bsl::make_unique<prometheus::Gateway>(
            prometheusHost,
            bsl::to_string(prometheusPort),
            name(),  // TODO: use plugin name as a job name? probably should be
                     // configurable
            labels);
        d_prometheusGateway_p->RegisterCollectable(d_prometheusRegistry_p);
    }
    else {
        // Pull mode
        bsl::ostringstream endpoint;
        endpoint << prometheusHost << ":" << prometheusPort;
        d_prometheusExposer_p = bsl::make_unique<prometheus::Exposer>(
            endpoint.str());
        d_prometheusExposer_p->RegisterCollectable(d_prometheusRegistry_p);
    }
}

int PrometheusStatConsumer::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    if (!d_consumerConfig_p) {
        BALL_LOG_ERROR << "Could not find config for StatConsumer '" << name()
                       << "'";
        return -1;  // RETURN
    }

    d_publishInterval = d_consumerConfig_p->publishInterval();

    if (!isEnabled() || d_isStarted) {
        return 0;  // RETURN
    }

    setActionCounter();

    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    d_snapshotId = (static_cast<int>(d_publishInterval.seconds()) /
                    brkrCfg.stats().snapshotInterval());

    if (d_prometheusMode == "push") {
        // create push thread
        int rc = bslmt::ThreadUtil::create(
            &d_prometheusPushThreadHandle,
            mwcsys::ThreadUtil::defaultAttributes(),
            bdlf::BindUtil::bind(&PrometheusStatConsumer::prometheusPushThread,
                                 this));
        if (rc != 0) {
            BALL_LOG_ERROR << "#PROMETHEUS_REPORTING "
                           << "Failed to start prometheusPushThread thread"
                           << "' [rc: " << rc << "]";
            return rc;  // RETURN
        }
    }

    d_isStarted = true;
    return 0;
}

void PrometheusStatConsumer::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    if (d_prometheusMode == "push") {
        d_threadStop = true;
        d_prometheusThreadCondition.signal();
        bslmt::ThreadUtil::join(d_prometheusPushThreadHandle);
    }

    d_isStarted = false;
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

    if (d_prometheusMode == "push") {
        // Wake up push thread
        d_prometheusThreadCondition.signal();
    }
}

void PrometheusStatConsumer::captureQueueStats()
{
    // Lookup the 'domainQueues' stat context
    // This is guaranteed to work because it was asserted in the ctor.
    const mwcst::StatContext& domainsStatContext =
        *d_domainQueuesStatContext_p;

    typedef mqbstat::QueueStatsDomain::Stat Stat;  // Shortcut

    for (mwcst::StatContextIterator domainIt =
             domainsStatContext.subcontextIterator();
         domainIt;
         ++domainIt) {
        for (mwcst::StatContextIterator queueIt =
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

                auto& heartbeatGauge = prometheus::BuildGauge()
                                           .Name("queue_heartbeat")
                                           .Help("queue heartbeat")
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
                     false},
                };

                for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                     dpIt != bdlb::ArrayUtil::end(defs);
                     ++dpIt) {
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
                    {"queue_content_msgs", Stat::e_MESSAGES_MAX, false},
                    {"queue_content_bytes", Stat::e_BYTES_MAX, false},
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
                    const bsls::Types::Int64 value =
                        mqbstat::QueueStatsDomain::getValue(
                            *queueIt,
                            d_snapshotId,
                            static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                                dpIt->d_stat));
                    updateMetric(dpIt, labels, value);
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
        mwcsys::StatMonitorUtil::ACCESSOR(*d_systemStatContext_p,             \
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

    prometheus::Labels labels{{"DataType", "host-data"}};
    bslstl::StringRef  instanceName =
        mqbcfg::BrokerConfig::get().brokerInstanceName();
    if (!instanceName.empty()) {
        labels.emplace("instanceName", instanceName);
    }

    for (bsl::vector<bsl::pair<bsl::string, double> >::iterator it =
             datapoints.begin();
         it != datapoints.end();
         ++it) {
        auto& gauge = prometheus::BuildGauge()
                          .Name(it->first)
                          // TODO:    .Help()
                          .Register(*d_prometheusRegistry_p);
        gauge.Add(labels).Set(it->second);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(datapoints.size() == k_NUM_SYS_STATS);
}

void PrometheusStatConsumer::captureNetworkStats()
{
    bsl::vector<bsl::pair<bsl::string, double> > datapoints;

    const int k_NUM_NETWORK_STATS = 4;
    datapoints.reserve(k_NUM_NETWORK_STATS);

    const mwcst::StatContext* localContext =
        d_channelsStatContext_p->getSubcontext("local");
    const mwcst::StatContext* remoteContext =
        d_channelsStatContext_p->getSubcontext("remote");
    // NOTE: Should be StatController::k_CHANNEL_STAT_*, but can't due to
    //       dependency limitations.

#define RETRIEVE_METRIC(TAIL, STAT, CONTEXT)                                  \
    datapoints.emplace_back("brkr_system_net_" TAIL,                          \
                            mwcio::StatChannelFactoryUtil::getValue(          \
                                *CONTEXT,                                     \
                                d_snapshotId,                                 \
                                mwcio::StatChannelFactoryUtil::Stat::STAT));

    RETRIEVE_METRIC("local_in_bytes", e_BYTES_IN_DELTA, localContext);
    RETRIEVE_METRIC("local_out_bytes", e_BYTES_OUT_DELTA, localContext);
    RETRIEVE_METRIC("remote_in_bytes", e_BYTES_IN_DELTA, remoteContext);
    RETRIEVE_METRIC("remote_out_bytes", e_BYTES_OUT_DELTA, remoteContext);

#undef RETRIEVE_METRIC

    prometheus::Labels labels{{"DataType", "host-data"}};
    bslstl::StringRef  instanceName =
        mqbcfg::BrokerConfig::get().brokerInstanceName();
    if (!instanceName.empty()) {
        labels.emplace("instanceName", instanceName);
    }

    for (bsl::vector<bsl::pair<bsl::string, double> >::iterator it =
             datapoints.begin();
         it != datapoints.end();
         ++it) {
        auto& counter = prometheus::BuildCounter()
                            .Name(it->first)
                            // TODO:    .Help()
                            .Register(*d_prometheusRegistry_p);
        counter.Add(labels).Increment(it->second);
    }

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
    for (mwcst::StatContextIterator clusterIt =
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
    const mwcst::StatContext& clustersStatContext = *d_clustersStatContext_p;

    for (mwcst::StatContextIterator clusterIt =
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
    for (mwcst::StatContextIterator clusterIt =
             d_clustersStatContext_p->subcontextIterator();
         clusterIt;
         ++clusterIt) {
        // Iterate over each partition
        for (mwcst::StatContextIterator partitionIt =
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
            const bsl::string data_outstanding_bytes =
                prefix + "data_outstanding_bytes";

            const DatapointDef defs[] = {
                {rollover_time.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_ROLLOVER_TIME,
                 false},
                {journal_outstanding_bytes.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_JOURNAL_CONTENT,
                 false},
                {data_outstanding_bytes.c_str(),
                 mqbstat::ClusterStats::Stat::e_PARTITION_DATA_CONTENT,
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
    const mwcst::StatContext& domainsStatContext = *d_domainsStatContext_p;

    typedef mqbstat::DomainStats::Stat Stat;  // Shortcut

    for (mwcst::StatContextIterator domainIt =
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

void PrometheusStatConsumer::updateMetric(const DatapointDef*       def_p,
                                          const prometheus::Labels& labels,
                                          const bsls::Types::Int64  value)
{
    if (value != 0) {
        // To save metrics, only report non-null values
        if (def_p->d_isCounter) {
            auto& counter = prometheus::BuildCounter()
                                .Name(def_p->d_name)
                                // TODO:     .Help(def_p->d_help)
                                .Register(*d_prometheusRegistry_p);
            counter.Add(labels).Increment(static_cast<double>(value));
        }
        else {
            auto& gauge = prometheus::BuildGauge()
                              .Name(def_p->d_name)
                              // TODO:     .Help(def_p->d_help)
                              .Register(*d_prometheusRegistry_p);
            gauge.Add(labels).Set(static_cast<double>(value));
        }
    }
}

void PrometheusStatConsumer::setPublishInterval(
    bsls::TimeInterval publishInterval)
{
    // executed by the *SCHEDULER* thread of StatController

    const int snapshotInterval =
        mqbcfg::BrokerConfig::get().stats().snapshotInterval();

    // PRECONDITIONS
    BSLS_ASSERT(publishInterval.seconds() >= 0);
    BSLS_ASSERT(snapshotInterval > 0);
    BSLS_ASSERT(publishInterval.seconds() % snapshotInterval == 0);

    BALL_LOG_INFO << "Set PrometheusStatConsumer publish interval to "
                  << publishInterval;

    d_publishInterval = publishInterval;
    d_snapshotId      = static_cast<int>(d_publishInterval.seconds()) /
                   snapshotInterval;

    setActionCounter();
}

const mwcst::StatContext*
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
    const int snapshotInterval =
        mqbcfg::BrokerConfig::get().stats().snapshotInterval();

    // PRECONDITIONS
    BSLS_ASSERT(snapshotInterval > 0);
    BSLS_ASSERT(d_publishInterval >= 0);
    BSLS_ASSERT(d_publishInterval.seconds() % snapshotInterval == 0);

    d_actionCounter = static_cast<int>(d_publishInterval.seconds()) /
                      snapshotInterval;
}

void PrometheusStatConsumer::prometheusPushThread()
{
    // executed by the dedicated prometheus push thread
    mwcsys::ThreadUtil::setCurrentThreadName(k_THREADNAME);

    BALL_LOG_INFO << "Prometheus Push thread has started [id: "
                  << bslmt::ThreadUtil::selfIdAsUint64() << "]";
    while (!d_threadStop) {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_prometheusThreadMutex);
        d_prometheusThreadCondition.wait(&d_prometheusThreadMutex);

        auto returnCode = d_prometheusGateway_p->Push();
        if (returnCode != 200) {
            BALL_LOG_WARN << "Push to Prometheus failed with code: "
                          << returnCode;
        }

        BALL_LOG_DEBUG << "Pushed to Prometheus with code: " << returnCode;
    }

    BALL_LOG_INFO << "Prometheus Push thread terminated "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";
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

}  // close package namespace
}  // close enterprise namespace

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------ END-OF-FILE ---------------------------------
