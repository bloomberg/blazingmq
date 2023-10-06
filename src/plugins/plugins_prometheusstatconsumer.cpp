// plugins_prometheusstatconsumer.cpp                                     -*-C++-*-
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
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdlb_arrayutil.h>
#include <bdld_manageddatum.h>
#include <bdlf_bind.h>
#include <bdlt_currenttime.h>
#include <bsl_vector.h>
#include <bsls_annotation.h>
#include <bsls_performancehint.h>

// PROMETHEUS
#include "prometheus/labels.h"


namespace BloombergLP {
namespace plugins {

namespace {

BALL_LOG_SET_NAMESPACE_CATEGORY("BMQBRKR.PROMETHEUSSTATCONSUMER");

const char *k_THREADNAME     = "bmqPrometheusPush";

struct Tagger {
    bslstl::StringRef d_cluster;
    bslstl::StringRef d_domain;
    bslstl::StringRef d_tier;
    bslstl::StringRef d_queue;
    bslstl::StringRef d_role;
    bslstl::StringRef d_instance;
    bslstl::StringRef d_remoteHost;
    bslstl::StringRef d_dataType;

    Tagger& setCluster(const bslstl::StringRef& value)
    {
        d_cluster = value;
        return *this;
    }

    Tagger& setDomain(const bslstl::StringRef& value)
    {
        d_domain = value;
        return *this;
    }

    Tagger& setTier(const bslstl::StringRef& value)
    {
        d_tier = value;
        return *this;
    }

    Tagger& setQueue(const bslstl::StringRef& value)
    {
        d_queue = value;
        return *this;
    }

    Tagger& setRole(const bslstl::StringRef& value)
    {
        d_role = value;
        return *this;
    }

    Tagger& setInstance(const bslstl::StringRef& value)
    {
        d_instance = value;
        return *this;
    }

    Tagger& setRemoteHost(const bslstl::StringRef& value)
    {
        d_remoteHost = value;
        return *this;
    }

    Tagger& setDataType(const bslstl::StringRef& value)
    {
        d_dataType = value;
        return *this;
    }

    prometheus::Labels getLabels()
    {
        // TODO: find a better way
        prometheus::Labels labels;
        if (!d_dataType.empty())
            labels.emplace("DataType", d_dataType);
        if (!d_cluster.empty())
            labels.emplace("Cluster", d_cluster);
        if (!d_domain.empty())
            labels.emplace("Domain", d_domain);
        if (!d_tier.empty())
            labels.emplace("Tier", d_tier);
        if (!d_queue.empty())
            labels.emplace("Queue", d_queue);
        if (!d_role.empty())
            labels.emplace("Role", d_role);
        if (!d_instance.empty())
            labels.emplace("Instance", d_instance);
        if (!d_remoteHost.empty())
            labels.emplace("RemoteHost", d_remoteHost);

        return labels;
    }
};

}  // close unnamed namespace

                          // ----------------------
                          // class PrometheusStatConsumer
                          // ----------------------

PrometheusStatConsumer::~PrometheusStatConsumer()
{
    stop();
}

PrometheusStatConsumer::PrometheusStatConsumer(const StatContextsMap&  statContextsMap,
                                   bslma::Allocator       *allocator)
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

    // Populate host name from config
    d_systemStatContext_p       = getStatContext("system");
    d_brokerStatContext_p       = getStatContext("broker");
    d_clustersStatContext_p     = getStatContext("clusters");
    d_clusterNodesStatContext_p = getStatContext("clusterNodes");
    d_domainsStatContext_p      = getStatContext("domains");
    d_domainQueuesStatContext_p = getStatContext("domainQueues");
    d_clientStatContext_p       = getStatContext("clients");
    d_channelsStatContext_p     = getStatContext("channels");

    // Initialize Prometheus
    // FIXME: need to generate mqbcfg_messages.cpp/h with new fields
    // d_prometheusHost(d_consumerConfig_p->host());
    // d_prometheusPort(d_consumerConfig_p->port());
    // d_prometheusMode(d_consumerConfig_p->mode());
    const char * prometheusHost = "localhost";
    size_t prometheusPort = 9090;
    d_prometheusMode = "pull"; // TODO enum or check validity?

    //d_prometheusRegistry_p = std::make_shared<prometheus::Registry>();

    if (d_prometheusMode == "push") {
        bsl::string clientHostName = "clientHostName"; //TODO: do we need this? to recognise different sources of statistics?
        //bsl::string jobName = 

        // Push mode
        // create a push gateway
        // const auto labels = prometheus::Gateway::GetInstanceLabel(clientHostName); // creates label { "instance": clientHostName }
        // d_prometheusGateway_p         = bsl::make_unique<prometheus::Gateway>(prometheusHost,
        //                                       bsl::to_string(prometheusPort),
        //                                       name(),  // use plugin name as a job name
        //                                       labels);
        // d_prometheusGateway_p->RegisterCollectable(d_prometheusRegistry_p);
    } else {
        // Pull mode
        // bsl::ostringstream endpoint;
        // endpoint << prometheusHost << ":" << prometheusPort;
        // d_prometheusExposer = bsl::make_unique<prometheus::Exposer>(endpoint.str());
        // d_prometheusExposer->RegisterCollectable(d_prometheusRegistry_p);
    }
}

int
PrometheusStatConsumer::start(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    if (!d_consumerConfig_p) {
        BALL_LOG_ERROR << "Could not find config for StatConsumer '"
                       << name() << "'";
        return -1;
    }

    d_publishInterval = d_consumerConfig_p->publishInterval();

    if (!isEnabled() || d_isStarted) {
        return 0;                                                     // RETURN
    }

    setActionCounter();

    // TODO: do we need d_snapshotId? we can use d_snapshotId = 1 to get all snapshots data and collect them in Registry
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    d_snapshotId = (  d_publishInterval.seconds()
                    / brkrCfg.stats().snapshotInterval());

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
            return rc;
        }
    }

    d_isStarted = true;
    return 0;
}

void
PrometheusStatConsumer::stop()
{
    if (!d_isStarted) {
        return;                                                       // RETURN
    }

    if (d_prometheusMode == "push") {
        d_threadStop = true;
        d_prometheusThreadCondition.signal();
        bslmt::ThreadUtil::join(d_prometheusPushThreadHandle);
    }

    d_isStarted = false;
}

void
PrometheusStatConsumer::onSnapshot()
{
                        // executed by the *SCHEDULER* thread of StatController
    if (!isEnabled()) {
        return;                                                       // RETURN
    }

    if (--d_actionCounter != 0) {
        return;                                                       // RETURN
    }

    setActionCounter();
    
    captureSystemStats();
    captureNetworkStats();
    captureQueueStats();

    if (d_prometheusMode == "push") {
        // Wake up push thread
        d_prometheusThreadCondition.signal();
    }
}

void
PrometheusStatConsumer::captureQueueStats()
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
            bdld::DatumMapRef                     map  =
                                                        mdSp->datum().theMap();

            const int role = mqbstat::QueueStatsDomain::getValue(
                                      *queueIt,
                                      d_snapshotId,
                                      mqbstat::QueueStatsDomain::Stat::e_ROLE);

            Tagger tagger;
            tagger
                .setCluster(map.find("cluster")->theString())
                .setDomain(map.find("domain")->theString())
                .setTier(map.find("tier")->theString())
                .setQueue(map.find("queue")->theString())
                .setRole(mqbstat::QueueStatsDomain::Role::toAscii(
                     static_cast<mqbstat::QueueStatsDomain::Role::Enum>(role)))
                .setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName())
                .setDataType("HostData");

            BALL_LOG_INFO << "!!! Labels: " << "cluster: " << tagger.d_cluster 
                          << ", domain: " << tagger.d_domain
                          << ", queue: " << tagger.d_queue
                          << ", tier: " << tagger.d_tier
                          << ", remoteHost: " << tagger.d_remoteHost
                          << ", Instance: " << tagger.d_instance
                          << ", dataType: " << tagger.d_dataType;
            prometheus::Labels labels = tagger.getLabels();

            // Heartbeat metric
            {
                // This metric is *always* reported for every queue, so that
                // there is guarantee to always (i.e. at any point in time) be
                // a time series containing all the tags that can be leveraged
                // in Grafana.

                // auto& heartbeatGauge = prometheus::BuildGauge()
                //             .Name("queue_heartbeat")
                //             .Help("queue heartbeat")
                //             .Register(*d_prometheusRegistry_p);
                // heartbeatGauge.Add(labels).Set(0);
            }

            // Queue metrics
            const mwcst::StatContext& queueContext = *queueIt;
            {  // for scoping only
                static const DatapointDef defs[] = {
                    { "queue_producers_count",   Stat::e_NB_PRODUCER, false },
                    { "queue_consumers_count",   Stat::e_NB_CONSUMER, false },
                    { "queue_put_msgs",          Stat::e_PUT_MESSAGES_DELTA, true },
                    { "queue_put_bytes",         Stat::e_PUT_BYTES_DELTA, true },
                    { "queue_push_msgs",         Stat::e_PUSH_MESSAGES_DELTA, true },
                    { "queue_push_bytes",        Stat::e_PUSH_BYTES_DELTA, true },
                    { "queue_ack_msgs",          Stat::e_ACK_DELTA, true },
                    { "queue_ack_time_avg",      Stat::e_ACK_TIME_AVG, false },
                    { "queue_ack_time_max",      Stat::e_ACK_TIME_MAX, false },
                    { "queue_nack_msgs",         Stat::e_NACK_DELTA, true },
                    { "queue_confirm_msgs",      Stat::e_CONFIRM_DELTA, true },
                    { "queue_confirm_time_avg",  Stat::e_CONFIRM_TIME_AVG, false },
                    { "queue_confirm_time_max",  Stat::e_CONFIRM_TIME_MAX, false },
                };

                BALL_LOG_INFO << "!!! Queue stat: !!!";
                for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                     dpIt != bdlb::ArrayUtil::end(defs);
                     ++dpIt) {
                    updateMetric(dpIt, labels, queueContext);
                }
            }

            // The following metrics only make sense to be reported from the
            // primary node only.
            if (role == mqbstat::QueueStatsDomain::Role::e_PRIMARY) {
                static const DatapointDef defs[] = {
                    { "queue.gc_msgs",              Stat::e_GC_MSGS_DELTA, true },
                    { "queue.cfg_msgs",             Stat::e_CFG_MSGS, false },
                    { "queue.cfg_bytes",            Stat::e_CFG_BYTES, false },
                    { "queue.content_msgs",         Stat::e_MESSAGES_MAX, false },
                    { "queue.content_bytes",        Stat::e_BYTES_MAX, false },
                    { "queue.queue_time_avg",       Stat::e_QUEUE_TIME_AVG, false },
                    { "queue.queue_time_max",       Stat::e_QUEUE_TIME_MAX, false },
                    { "queue.reject_msgs",          Stat::e_REJECT_DELTA, true},
                    { "queue.nack_noquorum_msgs",   Stat::e_NO_SC_MSGS_DELTA, true},
                };
                
                BALL_LOG_INFO << "!!! Prim. node stat: !!!";
                for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
                     dpIt != bdlb::ArrayUtil::end(defs);
                     ++dpIt) {
                    updateMetric(dpIt, labels, queueContext);
                }
            }
        }
    }
}

void
PrometheusStatConsumer::captureSystemStats()
{
    bsl::vector<bsl::pair<bsl::string, double>> datapoints;

    const int k_NUM_SYS_STATS = 10;
    datapoints.reserve(k_NUM_SYS_STATS);

#define COPY_METRIC(TAIL, ACCESSOR)                                           \
    datapoints.emplace_back("brkr_system." TAIL,                              \
        mwcsys::StatMonitorUtil::ACCESSOR(*d_systemStatContext_p,             \
                                           d_snapshotId));

    COPY_METRIC("cpu_sys",                  cpuSystem);
    COPY_METRIC("cpu_usr",                  cpuUser);
    COPY_METRIC("cpu_all",                  cpuAll);
    COPY_METRIC("mem_res",                  memResident);
    COPY_METRIC("mem_virt",                 memVirtual);
    COPY_METRIC("os_pagefaults_minor",      minorPageFaults);
    COPY_METRIC("os_pagefaults_major",      majorPageFaults);
    COPY_METRIC("os_swaps",                 numSwaps);
    COPY_METRIC("os_ctxswitch_voluntary",   voluntaryContextSwitches);
    COPY_METRIC("os_ctxswitch_involuntary", involuntaryContextSwitches);

#undef COPY_METRIC

    prometheus::Labels labels {{"dataType", "HostData"}};
    bslstl::StringRef instanceName =
                              mqbcfg::BrokerConfig::get().brokerInstanceName();
    if (!instanceName.empty()) {
        labels.emplace("instanceName", instanceName);
    }

    BALL_LOG_INFO << "!!! System metrics, label: " << instanceName;
    for (bsl::vector<bsl::pair<bsl::string, double>>::iterator it = datapoints.begin();
            it != datapoints.end();
            ++it) {

        BALL_LOG_INFO << "!!! " << it->first << " : " << it->second;

        // auto& gauge = prometheus::BuildGauge()
        //             .Name(it->first)
        // // TODO:    .Help()
        //             .Register(*d_prometheusRegistry_p);
        // gauge.Add(labels).Set(it->second);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(datapoints.size() == k_NUM_SYS_STATS);
}

void
PrometheusStatConsumer::captureNetworkStats()
{
    bsl::vector<bsl::pair<bsl::string, double>> datapoints;

    const int k_NUM_NETWORK_STATS = 4;
    datapoints.reserve(k_NUM_NETWORK_STATS);

    const mwcst::StatContext *localContext =
                               d_channelsStatContext_p->getSubcontext("local");
    const mwcst::StatContext *remoteContext =
                              d_channelsStatContext_p->getSubcontext("remote");
        // NOTE: Should be StatController::k_CHANNEL_STAT_*, but can't due to
        //       dependency limitations.

#define RETRIEVE_METRIC(TAIL, STAT, CONTEXT)                                \
    datapoints.emplace_back("brkr_system_net_" TAIL,                        \
        mwcio::StatChannelFactoryUtil::getValue(                            \
                       *CONTEXT,                                            \
                       d_snapshotId,                                        \
                       mwcio::StatChannelFactoryUtil::Stat::STAT));

    RETRIEVE_METRIC("local_in_bytes",   e_BYTES_IN_DELTA,  localContext);
    RETRIEVE_METRIC("local_out_bytes",  e_BYTES_OUT_DELTA, localContext);
    RETRIEVE_METRIC("remote_in_bytes",  e_BYTES_IN_DELTA,  remoteContext);
    RETRIEVE_METRIC("remote_out_bytes", e_BYTES_OUT_DELTA, remoteContext);

#undef RETRIEVE_METRIC

    prometheus::Labels labels {{"dataType", "HostData"}};
    bslstl::StringRef instanceName =
                              mqbcfg::BrokerConfig::get().brokerInstanceName();
    if (!instanceName.empty()) {
        labels.emplace("instanceName", instanceName);
    }

    BALL_LOG_INFO << "!!! Network metrics, label: " << instanceName;
    for (bsl::vector<bsl::pair<bsl::string, double>>::iterator it = datapoints.begin();
            it != datapoints.end();
            ++it) {

        BALL_LOG_INFO << "!!! " << it->first << " : " << it->second;

        // auto& counter = prometheus::BuildCounter()
        //             .Name(it->first)
        // // TODO:    .Help()
        //             .Register(*d_prometheusRegistry_p);
        // counter.Add(labels).Increment(it->second);
    }

    // POSTCONDITIONS
    BSLS_ASSERT_SAFE(datapoints.size() == k_NUM_NETWORK_STATS);
}

void
PrometheusStatConsumer::captureBrokerStats()
{
    typedef mqbstat::BrokerStats::Stat Stat;  // Shortcut

    static const DatapointDef defs[] = {
        { "brkr_summary_queues_count",  Stat::e_QUEUE_COUNT, false },
        { "brkr_summary_clients_count", Stat::e_CLIENT_COUNT, false },
    };

    Tagger tagger;
    tagger.setInstance(mqbcfg::BrokerConfig::get().brokerInstanceName()).setDataType("HostData");
    BALL_LOG_INFO << "!!! Broker Labels: "
                    << ", Instance: " << tagger.d_instance
                    << ", dataType: " << tagger.d_dataType;
    prometheus::Labels labels = tagger.getLabels();

    BALL_LOG_INFO << "!!! Broker stat: !!!";
    for (DatapointDefCIter dpIt = bdlb::ArrayUtil::begin(defs);
            dpIt != bdlb::ArrayUtil::end(defs);
            ++dpIt) {
        updateMetric(dpIt, labels, *d_brokerStatContext_p);
    }
}

void
PrometheusStatConsumer::updateMetric(const DatapointDef *def_p, prometheus::Labels& labels, const mwcst::StatContext& context)
{
    const bsls::Types::Int64 value =
        mqbstat::QueueStatsDomain::getValue(
            context,
            d_snapshotId,
            static_cast<mqbstat::QueueStatsDomain::Stat::Enum>(
                def_p->d_stat));

    if (value != 0) {
        BALL_LOG_INFO << "!!! " << def_p->d_name << " : " << value;
        // To save metrics, only report non-null values
        if (def_p->d_isCounter) {
            // auto& counter = prometheus::BuildCounter()
            //             .Name(def_p->d_name)
            //// TODO:     .Help(def_p->d_help)
            //             .Register(*d_prometheusRegistry_p);
            //counter.Add(labels).Increment(value);
        } else {
            // auto& gauge = prometheus::BuildGauge()
            //             .Name(def_p->d_name)
            //// TODO:     .Help(def_p->d_help)
            //             .Register(*d_prometheusRegistry_p);
            //gauge.Add(labels).Set(value);
        }
    }
}

void
PrometheusStatConsumer::setPublishInterval(bsls::TimeInterval publishInterval)
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
    d_snapshotId      = d_publishInterval.seconds() / snapshotInterval;

    setActionCounter();    
}

const mwcst::StatContext*
PrometheusStatConsumer::getStatContext(const char *name) const
{
    StatContextsMap::const_iterator cIt = d_contextsMap.find(name);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(cIt == d_contextsMap.end())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        BSLS_ASSERT_SAFE(false && "StatContext not found in GUTSStatConsumer");
        return 0;                                                     // RETURN
    }
    return cIt->second;
}

void
PrometheusStatConsumer::setActionCounter()
{
    const int snapshotInterval =
                        mqbcfg::BrokerConfig::get().stats().snapshotInterval();

    // PRECONDITIONS
    BSLS_ASSERT(snapshotInterval > 0);
    BSLS_ASSERT(d_publishInterval >= 0);
    BSLS_ASSERT(d_publishInterval.seconds() % snapshotInterval == 0);

    d_actionCounter = d_publishInterval.seconds() / snapshotInterval;
}

void
PrometheusStatConsumer::prometheusPushThread()
{
                                   // executed by the dedicated prometheus push thread
    mwcsys::ThreadUtil::setCurrentThreadName(k_THREADNAME);

    BALL_LOG_INFO << "Prometheus Push thread has started [id: "
                  << bslmt::ThreadUtil::selfIdAsUint64() << "]";
    while (!d_threadStop) {
        bslmt::LockGuard<bslmt::Mutex> lock(&d_prometheusThreadMutex);
        d_prometheusThreadCondition.wait(&d_prometheusThreadMutex);

        //auto returnCode = d_prometheusGateway_p->Push();
        //BALL_LOG_INFO << "Pushed to Prometheus with code: " << returnCode;
    }

    BALL_LOG_INFO << "Prometheus Push thread terminated "
                  << "[id: " << bslmt::ThreadUtil::selfIdAsUint64() << "]";
}

                     // ---------------------------------
                     // class PrometheusStatConsumerPluginFactory
                     // ---------------------------------

// CREATORS
PrometheusStatConsumerPluginFactory::PrometheusStatConsumerPluginFactory()
{
   // NOTHING
}

PrometheusStatConsumerPluginFactory::~PrometheusStatConsumerPluginFactory()
{
   // NOTHING
}

bslma::ManagedPtr<StatConsumer>
PrometheusStatConsumerPluginFactory::create(
                                const StatContextsMap&     statContexts,
                                const CommandProcessorFn& /*commandProcessor*/,
                                bdlbb::BlobBufferFactory* /*bufferFactory*/,
                                bslma::Allocator           *allocator)
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
