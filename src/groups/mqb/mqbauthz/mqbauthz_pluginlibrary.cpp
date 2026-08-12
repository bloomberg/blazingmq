// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbauthz_pluginlibrary.h>

#include <mqbscm_version.h>

// MQB
#include <mqbauthz_defaultauthorizer.h>

namespace BloombergLP {
namespace mqbauthz {

// -------------------
// class PluginLibrary
// -------------------

PluginLibrary::PluginLibrary(const allocator_type& allocator)
: d_plugins(allocator)
{
    // DefaultAuthorizer
    mqbplug::PluginInfo& defaultPluginInfo = d_plugins.emplace_back(
        mqbplug::PluginType::e_AUTHORIZER,
        mqbauthz::DefaultAuthorizer::k_NAME);
    defaultPluginInfo.setFactory(
        bsl::allocate_shared<DefaultAuthorizerPluginFactory>(allocator));
    defaultPluginInfo.setDescription("Built-in Default Allow-All Authorizer");
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
