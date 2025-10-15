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

#include <bmqauthnbasic_basicauthenticator.h>

// BMQAUTHNBASIC
#include <bmqauthnbasic_version.h>

// MQB
#include <mqbplug_authenticator.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqauthnbasic {

namespace {

const int k_TIMEOUT_SECONDS = 600;

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
: d_authenticatorConfig_p(config)
, d_isStarted(false)
, d_credentials(allocator)
, d_allocator_p(allocator)
{
    d_credentials["user1"] = "password1";
    d_credentials["user2"] = "password2";
}

BasicAuthenticator::~BasicAuthenticator()
{
    stop();
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
        errorDescription << "Invalid authentication payload format.";
        return -1;  // RETURN
    }

    bsl::string_view username = payloadView.substr(0, colonPos);
    bsl::string_view password = payloadView.substr(colonPos + 1);

    bsl::map<bsl::string, bsl::string>::const_iterator it = d_credentials.find(
        bsl::string(username, d_allocator_p));
    if (it == d_credentials.end() || it->second != password) {
        errorDescription << "Invalid username or password.";
        return -1;  // RETURN
    }

    *result = bsl::allocate_shared<BasicAuthenticationResult>(
        d_allocator_p,
        "VALID_USER-" + bsl::string(username, d_allocator_p),
        k_TIMEOUT_SECONDS * bdlt::TimeUnitRatio::k_MILLISECONDS_PER_SECOND);
    return 0;
}

int BasicAuthenticator::start(bsl::ostream& errorDescription)
{
    if (!d_authenticatorConfig_p) {
        errorDescription << "Could not find config for BasicAuthenticator '"
                         << name() << "' with mechanism '" << mechanism()
                         << "'";
        return -1;  // RETURN
    }

    if (d_isStarted) {
        return 0;  // RETURN
    }

    d_isStarted = true;

    return 0;
}

void BasicAuthenticator::stop()
{
    d_isStarted = false;
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

    if (!config) {
        return bslma::ManagedPtr<mqbplug::Authenticator>();
    }

    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) BasicAuthenticator(config, allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
