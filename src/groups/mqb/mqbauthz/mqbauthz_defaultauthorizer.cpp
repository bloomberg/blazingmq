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

#include <mqbauthz_defaultauthorizer.h>

#include <mqbscm_version.h>

// MQB
#include <mqbact_actions.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authorizer.h>

// BDE
#include <ball_log.h>
#include <bsla_maybeunused.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mqbauthz {

bsl::string_view DefaultAuthorizer::k_NAME = "DefaultAuthorizer";

// -----------------------
// class DefaultAuthorizer
// -----------------------

DefaultAuthorizer::DefaultAuthorizer(
    BSLA_MAYBE_UNUSED const mqbcfg::AuthorizerPluginConfig* config)
{
    // NOTHING
}

DefaultAuthorizer::~DefaultAuthorizer()
{
    // NOTHING
}

bsl::string_view DefaultAuthorizer::name() const
{
    return k_NAME;
}

bool DefaultAuthorizer::authorize(
    const mqbact::Action&   action,
    BSLA_MAYBE_UNUSED const mqbplug::AuthenticationResult& authnResult)

{
    // BALL_LOG_INFO << "Authorize allow on " << action;
    // return true;

    bsl::optional<const Policy::Permission*> permission = d_policy.get(authnResult.principal());

    if (!permission) {
        // TODO(tfoxhall): Maybe fallback to a default policy?
        return false;
    }

    struct PolicyHandler {
        enum { k_DENY, k_ALLOW };

        const Policy::Permission* d_permission;

        PolicyHandler(const Policy::Permission* permission)
        : d_permission(permission)
        {
        }

        int operator()(const mqbact::ConnectClient& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isConnectClientAllowed() ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::ConnectProxy& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isConnctProxyAllowed() ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::ConnectAdmin& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isConnctAdminAllowed() ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::ConnectClusterNode& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isConnctClusterNodeAllowed() ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::QueueRead& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isQueueReadAllowed(action.uri()) ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::QueueWrite& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isQueueWriteAllowed(action.uri()) ? k_ALLOW : k_DENY;
        }
        int operator()(const mqbact::ExecuteAdminCommand& action,
                       const bdlat_SelectionInfo&)
        {
            return d_permission->isExecuteAdminCommandAllowed(action.command()) ? k_ALLOW : k_DENY;
        }
    };

    PolicyHandler handler(*permission);
    int           rc = action.accessSelection(handler);
    return rc == PolicyHandler::k_ALLOW;
}

// ------------------------------------
// class DefaultAuthorizerPluginFactory
// ------------------------------------

DefaultAuthorizerPluginFactory::~DefaultAuthorizerPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authorizer>
DefaultAuthorizerPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthorizerPluginConfig* config =
        mqbplug::AuthorizerUtil::findAuthorizerConfig(
            DefaultAuthorizer::k_NAME);

    bslma::ManagedPtr<DefaultAuthorizer> defaultAuthorizer =
        bslma::ManagedPtrUtil::allocateManaged<DefaultAuthorizer>(allocator,
                                                                  config);
    bslma::ManagedPtr<mqbplug::Authorizer> authorizer(
        bslmf::MovableRefUtil::move(defaultAuthorizer));
    return authorizer;
}

}  // close package namespace
}  // close enterprise namespace
