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

#include <mqbauthn_basicauthenticator.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>

// BDE
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbauthn {

namespace {

const int k_AUTHN_DURATION_SECONDS = 600;

}  // namespace

// -------------------------------
// class BasicAuthenticationResult
// -------------------------------

BasicAuthenticationResult::BasicAuthenticationResult(
    bsl::string_view   principal,
    bsls::Types::Int64 lifetimeMs,
    bslma::Allocator*  allocator)
: d_principal(principal, allocator)
, d_lifetimeMs(lifetimeMs)
{
}

BasicAuthenticationResult::~BasicAuthenticationResult()
{
}

bsl::string_view BasicAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
BasicAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// ------------------------
// class BasicAuthenticator
// ------------------------

BasicAuthenticator::BasicAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_allocator_p(allocator)
, d_credentials(allocator)
, d_isStarted(false)
{
    if (!config) {
        BALL_LOG_WARN << "No config provided for BasicAuthenticator '"
                      << name() << "' with mechanism '" << mechanism() << "'.";
        return;
    }

    // Load the configured key-values as username and password
    bool isValueFound = false;
    bsl::vector<mqbcfg::PluginSettingKeyValue>::const_iterator it =
        config->settings().cbegin();
    for (; it != config->settings().cend(); ++it) {
        if (!it->value().isStringValValue()) {
            BALL_LOG_WARN << "Expected string for 'key-value' setting...";
            continue;
        }
        d_credentials[it->key()] = it->value().stringVal();
        isValueFound             = true;
    }
    if (isValueFound) {
        BALL_LOG_INFO << "... setting found.  Loaded " << d_credentials.size()
                      << " credential(s).";
    }
    else {
        BALL_LOG_INFO << "... setting not found.  Make sure this is expected.";
    }
}

BasicAuthenticator::~BasicAuthenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

bsl::string_view BasicAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view BasicAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int BasicAuthenticator::authenticate(
    bsl::ostream&                                   errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    const mqbplug::AuthenticationData&              input) const
{
    BALL_LOG_INFO << "BasicAuthenticator: "
                  << "authentication using mechanism '" << mechanism() << "'.";

    const bsl::vector<char>& payload = input.authnPayload();
    bsl::string_view payloadView(reinterpret_cast<const char*>(payload.data()),
                                 payload.size());
    bsl::size_t      colonPos = payloadView.find(':');
    if (bsl::string::npos == colonPos) {
        errorDescription << "Invalid authentication payload format. Expected "
                            "'username:password'";
        return -1;  // RETURN
    }

    bsl::string_view username = payloadView.substr(0, colonPos);
    bsl::string_view password = payloadView.substr(colonPos + 1);

    bsl::map<bsl::string, bsl::string>::const_iterator it = d_credentials.find(
        bsl::string(username, d_allocator_p));
    if (it == d_credentials.end() || it->second != password) {
        errorDescription << "Invalid username or password for user '"
                         << username << "'";
        return -1;  // RETURN
    }

    BALL_LOG_INFO << "BasicAuthenticator: "
                  << "authentication successful for user '" << username << "'";

    *result = bsl::allocate_shared<BasicAuthenticationResult>(
        d_allocator_p,
        "BASIC_USER-" + bsl::string(username, d_allocator_p),
        k_AUTHN_DURATION_SECONDS *
            bdlt::TimeUnitRatio::k_MILLISECONDS_PER_SECOND);
    return 0;
}

int BasicAuthenticator::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;
    }

    d_isStarted = true;

    BALL_LOG_INFO << "BasicAuthenticator started with " << d_credentials.size()
                  << " configured credentials";

    return 0;
}

void BasicAuthenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    BALL_LOG_INFO << "BasicAuthenticator stopped";
}

// -------------------------------------
// class BasicAuthenticatorPluginFactory
// -------------------------------------

BasicAuthenticatorPluginFactory::BasicAuthenticatorPluginFactory()
{
    // NOTHING
}

BasicAuthenticatorPluginFactory::~BasicAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
BasicAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            BasicAuthenticator::k_NAME);

    return bslma::ManagedPtrUtil::allocateManaged<BasicAuthenticator>(
        allocator,
        config);
}

}  // close package namespace
}  // close enterprise namespace
