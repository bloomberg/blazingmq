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

#ifndef INCLUDED_PROMETHEUS_PROMETHEUSSTATCONSUMER
#define INCLUDED_PROMETHEUS_PROMETHEUSSTATCONSUMER

//@PURPOSE: Provide a 'StatConsumer' plugin for publishing stats to Prometheus.
//
//@CLASSES:
//  bmqprometheus::PrometheusStatConsumer: bmqbrkr plugin for publishing stats
//  to Prometheus.
//
//@DESCRIPTION: 'bmqprometheus::PrometheusStatConsumer' handles the publishing
// of statistics to Prometheus.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_statconsumer.h>

// MWC
#include <mwcc_monitoredqueue_bdlccfixedqueue.h>
#include <mwcst_statcontext.h>
#include <mwcu_throttledaction.h>

// BDE
#include <bdlcc_sharedobjectpool.h>
#include <bsl_deque.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bslstl_stringref.h>

// PROMETHEUS
#include <prometheus/labels.h>
#include <prometheus/registry.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mwcst {
class StatContext;
}

namespace bmqprometheus {

using StatConsumer       = mqbplug::StatConsumer;
using StatContextsMap    = StatConsumer::StatContextsMap;
using CommandProcessorFn = StatConsumer::CommandProcessorFn;

// ============================
// class PrometheusStatExporter
// ============================

/// This is the interface class for exporting stats to Prometheus
class PrometheusStatExporter {
  public:
    PrometheusStatExporter(const PrometheusStatExporter&)            = delete;
    PrometheusStatExporter& operator=(const PrometheusStatExporter&) = delete;
    PrometheusStatExporter()                                         = default;

    virtual ~PrometheusStatExporter() = default;

    virtual void onData() {}
    virtual int  start() = 0;
    virtual void stop()  = 0;
};

// ============================
// class PrometheusStatConsumer
// ============================

class PrometheusStatConsumer : public mqbplug::StatConsumer {
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.PROMETHEUSSTATCONSUMER");

  private:
    // PRIVATE TYPES
    using LeaderSet = bsl::unordered_set<bslstl::StringRef>;

    struct DatapointDef {
        const char* d_name;
        int         d_stat;
        bool        d_isCounter;
    };

    using DatapointDefCIter = const DatapointDef*;

    const mwcst::StatContext* d_systemStatContext_p;
    // The system stat context

    const mwcst::StatContext* d_brokerStatContext_p;
    // The broker stat context

    const mwcst::StatContext* d_clustersStatContext_p;
    // The cluster stat context

    const mwcst::StatContext* d_clusterNodesStatContext_p;
    // The cluster nodes stat context

    const mwcst::StatContext* d_domainsStatContext_p;
    // The domain stat context

    const mwcst::StatContext* d_domainQueuesStatContext_p;
    // The domain queues stat context

    const mwcst::StatContext* d_clientStatContext_p;
    // The client stat context

    const mwcst::StatContext* d_channelsStatContext_p;
    // The channels stat context

    StatContextsMap d_contextsMap;
    // Map of stat contexts

    const mqbcfg::StatPluginConfig* d_consumerConfig_p;
    // Broker configuration for consumer.

    bsls::TimeInterval d_publishInterval;
    // Prometheus stat publish interval.  Specified as a number of seconds.
    // Must be a multiple of the snapshot interval.

    bsls::TimeInterval d_snapshotInterval;
    // Stats snapshot interval.  Specified as a number of seconds.

    int d_snapshotId;
    // Snapshot id which is used to locate data in stat history.  Calculated
    // as a result of dividing the publish interval by the snapshot interval.

    int d_actionCounter;
    // Stats are published to Prometheus only every publish interval.  This
    // counter is used to keep track of when to publish.

    bool d_isStarted;
    // Is the PrometheusStatConsumer started

    bsl::unique_ptr<PrometheusStatExporter> d_prometheusStatExporter_p;
    // Entity responsible for sending statistics to Prometheus backend

    std::shared_ptr< ::prometheus::Registry> d_prometheusRegistry_p;
    // Container for storing statistics in Prometheus format

  private:
    // PRIVATE ACCESSORS

    /// Return a pointer to the statContext with the specified 'name' from
    /// 'd_contextsMap', asserting that it exists.
    const mwcst::StatContext* getStatContext(const char* name) const;

    // PRIVATE MANIPULATORS

    /// Capture all queue related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureQueueStats();

    /// Capture all system related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureSystemStats();

    /// Capture all network related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureNetworkStats();

    /// Capture all broker related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureBrokerStats();

    /// Record all the current leaders in the specified 'leaders' set.
    void collectLeaders(LeaderSet* leaders);

    /// Capture all cluster related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureClusterStats(const LeaderSet& leaders);

    /// Capture all cluster's partitions related data points, and store them in
    /// Prometheus Registry for further publishing to Prometheus.
    void captureClusterPartitionsStats();

    /// Capture all queue related data points, and store them in Prometheus
    /// Registry for further publishing to Prometheus.
    void captureDomainStats(const LeaderSet& leaders);

    /// Set internal action counter based on Prometheus publish interval.
    void setActionCounter();

    /// Update metric by given 'def_p', 'labels' and 'value' in Prometheus
    /// Registry.
    void updateMetric(const DatapointDef*         def_p,
                      const ::prometheus::Labels& labels,
                      const bsls::Types::Int64    value);

    /// Stop plugin
    void stopImpl();

  public:
    // NOT IMPLEMENTED

    PrometheusStatConsumer(const PrometheusStatConsumer& other) = delete;
    PrometheusStatConsumer&
    operator=(const PrometheusStatConsumer& other) = delete;

    // CREATORS

    /// Create a new 'PrometheusStatConsumer' using the specified
    /// 'statContextMap' and the optionally specified 'allocator' for memory
    /// allocation.
    PrometheusStatConsumer(const StatContextsMap& statContextsMap,
                           bslma::Allocator*      allocator);

    /// Destructor.
    ~PrometheusStatConsumer() override;

    // MANIPULATORS

    /// Start the PrometheusStatConsumer and return 0 on success, or return a
    /// non-zero value and populate the specified 'errorDescription' with
    /// the description of any failure encountered.
    int start(bsl::ostream& errorDescription) override;

    /// Stop the PrometheusStatConsumer.
    void stop() override;

    /// Publish stats to Prometheus if publishing at the intervals specified by
    /// the config.
    void onSnapshot() override;

    /// Set the Prometheus publish interval with the specified
    /// 'publishInterval'. Disable Prometheus publishing if 'publishInterval'
    /// is 0.  It is expected that specified 'publishInterval' is a multiple of
    /// the snapshot interval or 0.
    void setPublishInterval(bsls::TimeInterval publishInterval) override;

    // ACCESSORS

    /// Return plugin name
    bslstl::StringRef name() const override;

    /// Return true if Prometheus reporting is enabled, false otherwise.
    bool isEnabled() const override;

    /// Return current value of publish interval
    bsls::TimeInterval publishInterval() const override;
};

// =========================================
// class PrometheusStatConsumerPluginFactory
// =========================================

/// This is the factory class for plugin of type 'PrometheusStatConsumer'.
/// All it does is allows to instantiate a concrete object of the
/// 'PrometheusStatConsumer' interface, taking any required arguments.
class PrometheusStatConsumerPluginFactory
: public mqbplug::StatConsumerPluginFactory {
  public:
    // CREATORS
    PrometheusStatConsumerPluginFactory();

    ~PrometheusStatConsumerPluginFactory() override;

    // MANIPULATORS
    bslma::ManagedPtr<StatConsumer>
    create(const StatContextsMap&    statContexts,
           const CommandProcessorFn& commandProcessor,
           bdlbb::BlobBufferFactory* bufferFactory,
           bslma::Allocator*         allocator) override;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------------
// class PrometheusStatConsumer
// ----------------------------

inline bsls::TimeInterval PrometheusStatConsumer::publishInterval() const
{
    return d_publishInterval;
}

inline bslstl::StringRef PrometheusStatConsumer::name() const
{
    return "PrometheusStatConsumer";
}

inline bool PrometheusStatConsumer::isEnabled() const
{
    return d_publishInterval.seconds() > 0;
}

}  // close package namespace
}  // close enterprise namespace

#endif
