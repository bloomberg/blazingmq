// plugins_prometheusstatconsumer.h                                       -*-C++-*-
#ifndef INCLUDED_PLUGINS_PROMETHEUSSTATCONSUMER
#define INCLUDED_PLUGINS_PROMETHEUSSTATCONSUMER

//@PURPOSE: Provide a 'StatConsumer' plugin for publishing stats to Prometheus.
//
//@CLASSES:
//  plugins::PrometheusStatConsumer: bmqbrkr plugin for publishing stats to Prometheus.
//
//@DESCRIPTION: 'plugins::PrometheusStatConsumer' handles the publishing of
// statistics to Prometheus.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_statconsumer.h>

// MWC
#include <mwcc_monitoredqueue_bdlccfixedqueue.h>
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
#include <bslmt_threadutil.h>
#include <bsls_keyword.h>
#include <bsls_timeinterval.h>
#include <bslstl_stringref.h>

// PROMETHEUS
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/family.h"
#include "prometheus/gateway.h"
#include "prometheus/info.h"
#include "prometheus/registry.h"

namespace BloombergLP {

// FORWARD DECLARATION
namespace mwcst { class StatContext; }

namespace plugins {

typedef mqbplug::StatConsumer            StatConsumer;
typedef StatConsumer::StatContextsMap    StatContextsMap;
typedef StatConsumer::CommandProcessorFn CommandProcessorFn;

                          // ======================
                          // class PrometheusStatConsumer
                          // ======================

class PrometheusStatConsumer : public mqbplug::StatConsumer {
  private:
    // PRIVATE DATA
    bsls::TimeInterval d_publishInterval;
        // Prometheus stat publish interval.  Specified as a number of seconds.
        // Must be a multiple of the snapshot interval.
  private:
    // NOT IMPLEMENTED
    PrometheusStatConsumer(const PrometheusStatConsumer& other) = delete;
    PrometheusStatConsumer& operator=(
                             const PrometheusStatConsumer& other) = delete;
  public:
    // CREATORS
    PrometheusStatConsumer(const StatContextsMap&  statContextsMap,
                           bslma::Allocator       *allocator);
        // Create a new 'PrometheusStatConsumer' using the specified 'statContextMap'
        // and the optionally specified 'allocator' for memory allocation.

    ~PrometheusStatConsumer() override;
        // Destructor.

    // MANIPULATORS
    int start(bsl::ostream& errorDescription) override;
        // Start the PrometheusStatConsumer and return 0 on success, or return a
        // non-zero value and populate the specified 'errorDescription' with
        // the description of any failure encountered.

    void stop() override;
        // Stop the PrometheusStatConsumer.

    void onSnapshot() override;
        // Publish stats to Prometheus if publishing at the intervals specified by
        // the config.

    void setPublishInterval(
                     bsls::TimeInterval publishInterval) override;
        // Set the Prometheus publish interval with the specified 'publishInterval'.
        // Disable Prometheus publishing if 'publishInterval' is 0.  It is expected
        // that specified 'publishInterval' is a multiple of the snapshot
        // interval or 0.

    // ACCESSORS
    bslstl::StringRef name() const override;

    bool isEnabled() const override;
        // Returns true if Prometheus reporting is enabled, false otherwise.

    bsls::TimeInterval publishInterval() const override;
        // Return current value of publish interval
};

                    // =========================================
                    // class PrometheusStatConsumerPluginFactory
                    // =========================================

class PrometheusStatConsumerPluginFactory
    : public mqbplug::StatConsumerPluginFactory {
    // This is the factory class for plugins of type 'PrometheusStatConsumer'. All it
    // does is allows to instantiate a concrete object of the
    // 'PrometheusStatConsumer' interface, taking any required arguments.

  public:
    // CREATORS
    PrometheusStatConsumerPluginFactory();

    ~PrometheusStatConsumerPluginFactory() override;

    // MANIPULATORS
    bslma::ManagedPtr<StatConsumer>
    create(const StatContextsMap&     statContexts,
           const CommandProcessorFn&  commandProcessor,
           bdlbb::BlobBufferFactory  *bufferFactory,
           bslma::Allocator          *allocator) override;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

                           // ----------------------------
                           // class PrometheusStatConsumer
                           // ----------------------------

inline
bsls::TimeInterval
PrometheusStatConsumer::publishInterval() const
{
    return d_publishInterval;
}

inline
bslstl::StringRef
PrometheusStatConsumer::name() const
{
    return "PrometheusStatConsumer";
}

inline
bool
PrometheusStatConsumer::isEnabled() const
{
    return d_publishInterval.seconds() > 0;
}

}  // close package namespace
}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------ END-OF-FILE ---------------------------------
