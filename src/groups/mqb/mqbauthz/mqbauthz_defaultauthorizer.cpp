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
#include <mqbauthz_policy.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authorizer.h>
#include <mqbpoly_policies.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bdlsb_fixedmeminstreambuf.h>
#include <bsl_fstream.h>
#include <bsla_maybeunused.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mqbauthz {

namespace {

struct DefaultAuthorizer_PolicyHandler {
    enum { k_DENY, k_ALLOW };

    const Policy::Permission* d_permission;

    DefaultAuthorizer_PolicyHandler(const Policy::Permission* permission)
    : d_permission(permission)
    {
    }

    int operator()(BSLA_MAYBE_UNUSED const mqbact::ConnectClient& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isConnectClientAllowed() ? k_ALLOW : k_DENY;
    }
    int operator()(BSLA_MAYBE_UNUSED const mqbact::ConnectProxy& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isConnectProxyAllowed() ? k_ALLOW : k_DENY;
    }
    int operator()(BSLA_MAYBE_UNUSED const mqbact::ConnectAdmin& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isConnectAdminAllowed() ? k_ALLOW : k_DENY;
    }
    int operator()(BSLA_MAYBE_UNUSED const mqbact::ConnectClusterNode& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isConnectClusterNodeAllowed() ? k_ALLOW : k_DENY;
    }
    int operator()(const mqbact::QueueRead& action, const bdlat_SelectionInfo&)
    {
        return d_permission->isQueueReadAllowed(action.uri()) ? k_ALLOW
                                                              : k_DENY;
    }
    int operator()(const mqbact::QueueWrite& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isQueueWriteAllowed(action.uri()) ? k_ALLOW
                                                               : k_DENY;
    }
    int operator()(const mqbact::ExecuteAdminCommand& action,
                   const bdlat_SelectionInfo&)
    {
        return d_permission->isExecuteAdminCommandAllowed(action.command())
                   ? k_ALLOW
                   : k_DENY;
    }
};

struct FindKey {
    bsl::string_view d_key;

    FindKey(bsl::string_view key)
    : d_key(key)
    {
    }

    bool operator()(const mqbcfg::PluginSettingKeyValue& pluginSetting) const
    {
        return pluginSetting.key() == d_key;
    }
};

}

bsl::string_view DefaultAuthorizer::k_NAME = "DefaultAuthorizer";

// -----------------------
// class DefaultAuthorizer
// -----------------------

DefaultAuthorizer::DefaultAuthorizer()
: d_policy()
{
    // NOTHING
}

DefaultAuthorizer::DefaultAuthorizer(bslmf::MovableRef<PolicyMP> policy)
: d_policy(bslmf::MovableRefUtil::move(policy))
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
    if (!d_policy) {
        BALL_LOG_INFO << "Authorize allow on " << action;
        return true;
    }

    bsl::optional<const Policy::Permission*> permission = d_policy->get(
        authnResult.principal());

    if (!permission) {
        // TODO(tfoxhall): Maybe fallback to a default policy?
        return false;
    }

    DefaultAuthorizer_PolicyHandler handler(*permission);
    int                             rc = action.accessSelection(handler);
    return rc == DefaultAuthorizer_PolicyHandler::k_ALLOW;
}

// ------------------------------------
// class DefaultAuthorizerPluginFactory
// ------------------------------------

DefaultAuthorizerPluginFactory::~DefaultAuthorizerPluginFactory()
{
    // NOTHING
}

int DefaultAuthorizerPluginFactory::createPolicy(
    bslma::ManagedPtr<Policy>*            res,
    const mqbcfg::AuthorizerPluginConfig& config,
    bsl::allocator<>                      allocator) const
{
    enum {
        k_OK = 0,
        k_POLICY_CONFIG_NOT_FOUND,
        k_INCORRECT_POLICY_PATH_TYPE,
        k_POLICY_DEFINITION_READ_FAILED,
        k_DECODE_FAILED,
        k_INVALID_POLICY_DEFINITION
    };

    // Find the path to the policy document
    typedef bsl::vector<mqbcfg::PluginSettingKeyValue> PluginSettings;
    const PluginSettings&          settings = config.settings();
    PluginSettings::const_iterator it       = bsl::find_if(settings.cbegin(),
                                                     settings.cend(),
                                                     FindKey("policyPath"));
    if (it == settings.cend()) {
        // No policy set, everything will be denied
        BALL_LOG_WARN
            << "No policy document found, all authorized actions will be DENY";
        return k_POLICY_CONFIG_NOT_FOUND;  // RETURN
    }

    if (!it->value().isStringValValue()) {
        BALL_LOG_ERROR << "Unexpected type for policy definition path "
                          "(expected string, found "
                       << it->value().selectionName() << ")";
        return k_INCORRECT_POLICY_PATH_TYPE;
    }

    // Read the policy definition from the configured path
    const bsl::string& policyDefPath = it->value().stringVal();

    BALL_LOG_INFO << "Reading policy definition from " << policyDefPath;
    bsl::ifstream      policyDefStream(policyDefPath.c_str());
    bmqu::MemOutStream policyDefBuffer;
    policyDefBuffer << policyDefStream.rdbuf();
    bsl::string policyDef = policyDefBuffer.str();

    if (!policyDefStream || !policyDefBuffer) {
        BALL_LOG_ERROR << "Failed to read the policy definition "
                       << "[file: " << policyDefPath << "]";
        return k_POLICY_DEFINITION_READ_FAILED;  // RETURN
    }
    policyDefStream.close();

    // Decode the policy definition
    baljsn::Decoder        decoder;
    baljsn::DecoderOptions options;
    options.setSkipUnknownElements(true);

    bdlsb::FixedMemInStreamBuf jsonStreamBuf(policyDef.data(),
                                             policyDef.length());
    mqbpoly::Policy            policy;

    int rc = decoder.decode(&jsonStreamBuf, &policy, options);
    if (rc != 0) {
        BALL_LOG_ERROR << "Error decoding policy definition "
                       << "[rc: " << rc
                       << ", error: " << decoder.loggedMessages() << "], "
                       << "policy definition file (first 1024 characters):\n"
                       << policyDef.substr(
                              0,
                              bsl::min(bsl::string::size_type(1024),
                                       policyDef.length()));
        return k_DECODE_FAILED;  // RETURN
    }

    // Parse the policy document
    *res = bslma::ManagedPtrUtil::allocateManaged<Policy>(allocator);
    rc   = Policy::parse(res->get(), policy, allocator);
    if (rc != 0) {
        return k_INVALID_POLICY_DEFINITION;
    }

    return k_OK;
}

bslma::ManagedPtr<mqbplug::Authorizer>
DefaultAuthorizerPluginFactory::create(bslma::Allocator* allocator)
{
    bslma::ManagedPtr<mqbplug::Authorizer> authorizer;

    const mqbcfg::AuthorizerPluginConfig* config =
        mqbplug::AuthorizerUtil::findAuthorizerConfig(
            DefaultAuthorizer::k_NAME);

    if (config == NULL) {
        BALL_LOG_WARN << "No authorizer is configured";
        // TODO(tfoxhall): A little unclear what to do here, I think we want to
        // preserve the old allow-all behavior assuming that this is an older
        // deployment without any authorization configuration.
        bslma::ManagedPtr<DefaultAuthorizer> defaultAuthorizer =
            bslma::ManagedPtrUtil::allocateManaged<DefaultAuthorizer>(
                allocator);
        authorizer = bslmf::MovableRefUtil::move(defaultAuthorizer);
        return authorizer;  // RETURN
    }

    bslma::ManagedPtr<Policy> policy;
    int                       rc = createPolicy(&policy, *config, allocator);
    if (rc != 0) {
        BALL_LOG_ERROR << "DefaultAuthorizer could not be configured";
        return authorizer;  // RETURN
    }

    bslma::ManagedPtr<DefaultAuthorizer> defaultAuthorizer =
        bslma::ManagedPtrUtil::allocateManaged<DefaultAuthorizer>(
            allocator,
            bslmf::MovableRefUtil::move(policy));
    authorizer = bslmf::MovableRefUtil::move(defaultAuthorizer);

    return authorizer;
}

}  // close package namespace
}  // close enterprise namespace
