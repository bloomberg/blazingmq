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

#include <mqbauthz_authorizationcontroller.h>

#include <mqbscm_version.h>

// MQB
#include <mqbauthz_basicauthorizer.h>
#include <mqbauthz_pluginlibrary.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authorizer.h>
#include <mqbplug_pluginfactory.h>
#include <mqbplug_plugintype.h>

// BDE
#include <ball_log.h>
#include <bdlb_nullablevalue.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mqbauthz {

namespace {

}  // close unnamed namespace

// ------------------------------
// class AuthorizationController
// ------------------------------

// CREATORS

AuthorizationController::AuthorizationController(
    bslmf::MovableRef<AuthorizerMp> authorizer)
: d_authorizer_mp(bslmf::MovableRefUtil::move(authorizer))
{
    // PRECONDITIONS
    BSLS_ASSERT(d_authorizer_mp);
}

void AuthorizationController::collectAvailablePluginFactories(
    PluginFactories*              pluginFactories,
    const mqbplug::PluginManager& pluginManager,
    mqbplug::PluginType::Enum     type)
{
    // PRECONDITIONS
    BSLS_ASSERT(pluginFactories);

    // Collect external plugin factories loaded by PluginManager
    pluginManager.get(pluginFactories, type);

    // Add built-in plugin factories
    PluginLibrary                                    builtInPlugins;
    bsl::vector<mqbplug::PluginInfo>::const_iterator it =
        builtInPlugins.plugins().cbegin();
    for (; it != builtInPlugins.plugins().cend(); ++it) {
        if (it->type() == type) {
            pluginFactories->insert(it->factory().get());
        }
    }
}

int AuthorizationController::createConfiguredAuthorizer(
    AuthorizerMp*                   result,
    bsl::ostream&                   errorDescription,
    const PluginFactories&          pluginFactories,
    const mqbcfg::AuthorizerConfig& authorizerConfig,
    bsl::allocator<>                allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    enum RcEnum { rc_SUCCESS = 0, rc_PLUGIN_NOT_FOUND = -1 };

    // If authorizer is explicitly configured, create it
    const bsl::optional<mqbcfg::AuthorizerPluginConfig>& pluginConfigOpt =
        authorizerConfig.authorizer();

    if (!pluginConfigOpt.has_value()) {
        BALL_LOG_INFO << "No configured authorizer";
        return rc_SUCCESS;
    }

    const mqbcfg::AuthorizerPluginConfig& authorizerCfg =
        pluginConfigOpt.value();

    const bsl::string& pluginName = authorizerCfg.name();
    BALL_LOG_INFO << "Attempting to create authorizer plugin '" << pluginName
                  << "'";

    // Try to create the authorizer for this plugin name
    bsl::optional<mqbplug::AuthorizerPluginFactory*> factoryOpt =
        mqbplug::PluginFactoryUtil::findType<mqbplug::AuthorizerPluginFactory>(
            pluginFactories.cbegin(),
            pluginFactories.cend());
    if (!factoryOpt.has_value()) {
        errorDescription
            << "authorizer plugin '" << pluginName
            << "' not found. Ensure the plugin is either built-in or listed "
               "in plugins.enabled[], or its configuration is correct.";
        return rc_PLUGIN_NOT_FOUND;  // RETURN
    }
    *result = (*factoryOpt)->create(allocator.mechanism());

    return rc_SUCCESS;
}

void AuthorizationController::ensureDefaultAuthorizer(
    AuthorizerMp*    result,
    bsl::allocator<> allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    // Have authenticators, nothing to do
    if (*result) {
        return;
    }

    // If no authenticators are configured, use BasicAuthorizer for allow-all
    // authorization
    BALL_LOG_INFO << "No authorizer configured, using "
                     "BasicAuthorizer as default";

    BasicAuthorizerPluginFactory basicAuthzFactory;
    *result = basicAuthzFactory.create(allocator.mechanism());
}

int AuthorizationController::allocateManaged(
    bslma::ManagedPtr<AuthorizationController>* result,
    bsl::ostream&                               errorDescription,
    const mqbcfg::AuthorizerConfig&             authorizerConfig,
    const mqbplug::PluginManager&               pluginManager,
    const bsl::allocator<>&                     allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT(result);

    enum RcEnum {
        // Enum for the various RC error categories
        rc_SUCCESS                   = 0,
        rc_CREATE_AUTHORIZERS_FAILED = -1,
    };

    BALL_LOG_INFO << "Starting AuthorizationController";

    // Step 1: Collect all available plugin factories (built-in + external)
    PluginFactories pluginFactories;
    collectAvailablePluginFactories(&pluginFactories,
                                    pluginManager,
                                    mqbplug::PluginType::e_AUTHORIZER);

    // Step 2: Create authorizers based on configuration
    AuthorizerMp authorizer;
    if (0 != createConfiguredAuthorizer(&authorizer,
                                        errorDescription,
                                        pluginFactories,
                                        authorizerConfig,
                                        allocator)) {
        return rc_CREATE_AUTHORIZERS_FAILED;  // RETURN
    }

    // Step 3: Ensure at least one authorizer is available
    ensureDefaultAuthorizer(&authorizer, allocator);

    BSLS_ASSERT(authorizer);

    BALL_LOG_INFO << "Created authorizer '" << authorizer->name() << "'";

    *result = bslma::ManagedPtrUtil::allocateManaged<AuthorizationController>(
        allocator,
        bslmf::MovableRefUtil::move(authorizer));

    BALL_LOG_INFO << "Successfully initialized authorizer controller";

    return rc_SUCCESS;
}

mqbplug::Authorizer& AuthorizationController::authorizer()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_authorizer_mp);

    return *d_authorizer_mp;
}

}  // close package namespace
}  // close enterprise namespace
