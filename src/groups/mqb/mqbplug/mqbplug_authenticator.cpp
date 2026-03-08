// Copyright 2025 Bloomberg Finance L.P.
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

// mqbplug_authenticator.cpp                                          -*-C++-*-
#include <mqbplug_authenticator.h>

#include <mqbscm_version.h>

// MQB
#include <mqbcfg_messages.h>

// BDE
#include <bsl_string_view.h>
#include <bsl_vector.h>

namespace BloombergLP {
namespace mqbplug {

// --------------------------
// class AuthenticationResult
// --------------------------

AuthenticationResult::~AuthenticationResult()
{
    // NOTHING
}

// -------------------
// class Authenticator
// -------------------

Authenticator::~Authenticator()
{
    // NOTHING
}

// --------------------------------
// class AuthenticatorPluginFactory
// --------------------------------

AuthenticatorPluginFactory::AuthenticatorPluginFactory()
{
    // NOTHING
}

AuthenticatorPluginFactory::~AuthenticatorPluginFactory()
{
    // NOTHING
}

// -----------------------
// class AuthenticatorUtil
// -----------------------

const mqbcfg::AuthenticatorPluginConfig*
AuthenticatorUtil::findAuthenticatorConfig(bsl::string_view name)
{
    const bsl::vector<mqbcfg::AuthenticatorPluginConfig>& authenticatorsCfg =
        mqbcfg::BrokerConfig::get().authentication().authenticators();

    // Linear search is acceptable here since:
    // 1. This is typically called during startup phase, not hot path
    // 2. Number of configured authenticators is usually small (< 10)
    // 3. The cost of maintaining a persistent cache would outweigh benefits
    for (bsl::vector<mqbcfg::AuthenticatorPluginConfig>::const_iterator cit =
             authenticatorsCfg.cbegin();
         cit != authenticatorsCfg.cend();
         ++cit) {
        if (cit->name() == name) {
            return &(*cit);
        }
    }
    return 0;
}

}  // close package namespace
}  // close enterprise namespace
