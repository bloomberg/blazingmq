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
#include <mwcst_statcontext.h>
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

namespace BloombergLP {
namespace plugins {

                     // ---------------------------------
                     // class PrometheusStatConsumer::PrometheusEvent
                     // ---------------------------------

                          // ----------------------
                          // class PrometheusStatConsumer
                          // ----------------------

PrometheusStatConsumer::~PrometheusStatConsumer()
{
    stop();
}

PrometheusStatConsumer::PrometheusStatConsumer(const StatContextsMap&  statContextsMap,
                                   bslma::Allocator       *allocator)
: d_publishInterval(0)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_publishInterval >= 0);
}

int
PrometheusStatConsumer::start(BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    std::cout << "############PrometheusStatConsumer::start()\n";
    return 0;
}

void
PrometheusStatConsumer::stop()
{

}

void
PrometheusStatConsumer::onSnapshot()
{
                        // executed by the *SCHEDULER* thread of StatController
    std::cout << "############PrometheusStatConsumer::onSnapshot()\n";
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

    d_publishInterval = publishInterval;
}

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
