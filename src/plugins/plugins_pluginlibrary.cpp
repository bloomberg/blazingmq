// plugins_pluginlibrary.cpp                                        -*-C++-*-
#include <plugins_pluginlibrary.h>

// PLUGINS
#include <plugins_prometheusstatconsumer.h>
#include <plugins_version.h>

//// BUL
//#include <bulscm_version.h>

//// SIM
//#include <simscm_version.h>

// BDE
#include <bsl_sstream.h>
#include <bslma_default.h>


namespace BloombergLP {
namespace plugins {

                          // -------------------
                          // class PluginLibrary
                          // -------------------

PluginLibrary::PluginLibrary(bslma::Allocator *allocator)
: d_plugins(bslma::Default::allocator(allocator))
{
    allocator = bslma::Default::allocator(allocator);

    mqbplug::PluginInfo& prometheusPluginInfo =
                  d_plugins.emplace_back(mqbplug::PluginType::e_STATS_CONSUMER,
                                         "PrometheusStatConsumer");

    prometheusPluginInfo.setFactory(
               bsl::allocate_shared<PrometheusStatConsumerPluginFactory>(allocator));
    prometheusPluginInfo.setVersion(Version::version());

    bsl::stringstream prometheusDescription(allocator);
    prometheusDescription << "StatConsumer publishing to Prometheus";
    prometheusPluginInfo.setDescription(prometheusDescription.str());
}

PluginLibrary::~PluginLibrary()
{
    // NOTHING
}

int
PluginLibrary::activate()
{
    return 0;
}

void
PluginLibrary::deactivate()
{
}

const bsl::vector<mqbplug::PluginInfo>&
PluginLibrary::plugins() const
{
    return d_plugins;
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
