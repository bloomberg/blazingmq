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

// prometheus_pluginlibrary.cpp -*-C++-*-
#include <prometheus_pluginlibrary.h>

// PROMETHEUS
#include <prometheus_prometheusstatconsumer.h>
#include <prometheus_version.h>

// BDE
#include <bsl_sstream.h>
#include <bslma_default.h>

namespace BloombergLP {
namespace prometheus {

// -------------------
// class PluginLibrary
// -------------------

PluginLibrary::PluginLibrary(bslma::Allocator* allocator)
: d_plugins(bslma::Default::allocator(allocator))
{
    allocator = bslma::Default::allocator(allocator);

    mqbplug::PluginInfo& prometheusPluginInfo = d_plugins.emplace_back(
        mqbplug::PluginType::e_STATS_CONSUMER,
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

int PluginLibrary::activate()
{
    return 0;
}

void PluginLibrary::deactivate()
{
    // NOTHING
}

const bsl::vector<mqbplug::PluginInfo>& PluginLibrary::plugins() const
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
