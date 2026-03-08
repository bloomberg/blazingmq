// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mqbplug_statconsumer.cpp                                           -*-C++-*-
#include <mqbplug_statconsumer.h>

#include <bsl_vector.h>
#include <mqbscm_version.h>

namespace BloombergLP {
namespace mqbplug {

// ------------------
// class StatConsumer
// ------------------

// CREATORS
StatConsumer::~StatConsumer()
{
    // NOTHING
}

// -------------------------------
// class StatConsumerPluginFactory
// -------------------------------

// CREATORS
StatConsumerPluginFactory::StatConsumerPluginFactory()
{
    // NOTHING
}

StatConsumerPluginFactory::~StatConsumerPluginFactory()
{
    // NOTHING
}

// ----------------------
// class StatConsumerUtil
// ----------------------

const mqbcfg::StatPluginConfig*
StatConsumerUtil::findConsumerConfig(bsl::string_view name)
{
    const bsl::vector<mqbcfg::StatPluginConfig>& configs =
        mqbcfg::BrokerConfig::get().stats().plugins();
    bsl::vector<mqbcfg::StatPluginConfig>::const_iterator it = configs.begin();
    for (; it != configs.cend(); ++it) {
        if (it->name() == name) {
            return &(*it);  // RETURN
        }
    }
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
