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

#include <bmqauthnpass_passauthenticator.h>

// BMQAUTHNPASS
#include <bmqauthnpass_version.h>

// MQB
#include <mqbplug_authenticator.h>

// BMQ

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
namespace bmqauthnpass {

// ------------------------------
// class PassAuthenticationResult
// ------------------------------

PassAuthenticationResult::PassAuthenticationResult(
    bsl::string_view   principal,
    bsls::Types::Int64 lifetimeMs,
    bslma::Allocator*  allocator)
: d_principal(principal)
, d_lifetimeMs(lifetimeMs)
, d_allocator_p(allocator)
{
}

PassAuthenticationResult::~PassAuthenticationResult()
{
}

bsl::string_view PassAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
PassAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// -----------------------
// class PassAuthenticator
// -----------------------

PassAuthenticator::PassAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_authenticatorConfig_p(config)
, d_isStarted(false)
, d_allocator_p(allocator)
{
}

PassAuthenticator::~PassAuthenticator()
{
    stop();
}

bsl::string_view PassAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view PassAuthenticator::mechanism() const
{
    return "Basic";
}

int PassAuthenticator::authenticate(
    BSLA_UNUSED bsl::ostream&                       errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "PassAuthenticator: "
                  << "authentication passed for mechanism '" << mechanism()
                  << "' unconditionally.";

    *result = bsl::allocate_shared<PassAuthenticationResult>(d_allocator_p,
                                                             "",
                                                             600 * 1000);
    return 0;
}

int PassAuthenticator::start(bsl::ostream& errorDescription)
{
    if (!d_authenticatorConfig_p) {
        errorDescription << "Could not find config for PassAuthenticator '"
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

void PassAuthenticator::stop()
{
    d_isStarted = false;
}

// ------------------------------------
// class PassAuthenticatorPluginFactory
// ------------------------------------

PassAuthenticatorPluginFactory::PassAuthenticatorPluginFactory()
{
    // NOTHING
}

PassAuthenticatorPluginFactory::~PassAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
PassAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            PassAuthenticator::k_NAME);

    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) PassAuthenticator(config, allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
