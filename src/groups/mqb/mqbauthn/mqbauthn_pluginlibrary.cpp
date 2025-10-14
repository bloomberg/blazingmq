// Copyright 2016-2025 Bloomberg Finance L.P.
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

#include <mqbauthn_pluginlibrary.h>

// MQB
#include <mqbauthn_anonfailauthenticator.h>
#include <mqbauthn_anonpassauthenticator.h>
#include <mqbauthn_basicauthenticator.h>

// BDE
#include <bslma_default.h>

namespace BloombergLP {
namespace mqbauthn {

// -------------------
// class PluginLibrary
// -------------------

PluginLibrary::PluginLibrary(bslma::Allocator* allocator)
: d_plugins(allocator)
{
    // AnonPassAuthenticator
    mqbplug::PluginInfo& authnpassPluginInfo = d_plugins.emplace_back(
        mqbplug::PluginType::e_AUTHENTICATOR,
        mqbauthn::AnonPassAuthenticator::k_NAME);

    authnpassPluginInfo.setFactory(
        bsl::allocate_shared<AnonPassAuthenticatorPluginFactory>(allocator));
    authnpassPluginInfo.setDescription("Anonymous Pass Authenticator");

    // AnonFailAuthenticator
    mqbplug::PluginInfo& authnfailPluginInfo = d_plugins.emplace_back(
        mqbplug::PluginType::e_AUTHENTICATOR,
        mqbauthn::AnonFailAuthenticator::k_NAME);

    authnfailPluginInfo.setFactory(
        bsl::allocate_shared<AnonFailAuthenticatorPluginFactory>(allocator));
    authnfailPluginInfo.setDescription("Anonymous Fail Authenticator");

    // BasicAuthenticator
    mqbplug::PluginInfo& basicPluginInfo = d_plugins.emplace_back(
        mqbplug::PluginType::e_AUTHENTICATOR,
        mqbauthn::BasicAuthenticator::k_NAME);

    basicPluginInfo.setFactory(
        bsl::allocate_shared<BasicAuthenticatorPluginFactory>(allocator));
    basicPluginInfo.setDescription("Basic Username/Password Authenticator");
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
