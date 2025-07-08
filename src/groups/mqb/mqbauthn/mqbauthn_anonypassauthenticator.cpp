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

#include <mqbauthn_anonypassauthenticator.h>

// MQB
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsla_unused.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbauthn {

// -----------------------------------
// class AnonyPassAuthenticationResult
// -----------------------------------

AnonyPassAuthenticationResult::AnonyPassAuthenticationResult(
    bsl::string_view   principal,
    bsls::Types::Int64 lifetimeMs,
    bslma::Allocator*  allocator)
: d_principal(principal)
, d_lifetimeMs(lifetimeMs)
, d_allocator_p(allocator)
{
}

AnonyPassAuthenticationResult::~AnonyPassAuthenticationResult()
{
}

bsl::string_view AnonyPassAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
AnonyPassAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// ----------------------------
// class AnonyPassAuthenticator
// ----------------------------

AnonyPassAuthenticator::AnonyPassAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_authenticatorConfig_p(config)
, d_isStarted(false)
, d_allocator_p(allocator)
{
    if (!config) {
        // No config is provided for an anonymous authenticator.
    }
}

AnonyPassAuthenticator::~AnonyPassAuthenticator()
{
    stop();
}

bsl::string_view AnonyPassAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view AnonyPassAuthenticator::mechanism() const
{
    return "Anonymous";
}

int AnonyPassAuthenticator::authenticate(
    BSLA_UNUSED bsl::ostream&                       errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "AnonyPassAuthenticator: "
                  << "authentication passed for mechanism '" << mechanism()
                  << "' unconditionally.";

    *result = bsl::allocate_shared<AnonyPassAuthenticationResult>(
        d_allocator_p,
        "",
        600 * 1000);
    return 0;
}

int AnonyPassAuthenticator::start(BSLA_UNUSED bsl::ostream& errorDescription)
{
    // Since this is the default anonymous authenticator, it does not require
    // any specific configuration or initialization.
    // We just need to ensure it is started only once.

    if (d_isStarted) {
        return 0;  // RETURN
    }

    d_isStarted = true;

    return 0;
}

void AnonyPassAuthenticator::stop()
{
    d_isStarted = false;
}

// -----------------------------------------
// class AnonyPassAuthenticatorPluginFactory
// -----------------------------------------

AnonyPassAuthenticatorPluginFactory::AnonyPassAuthenticatorPluginFactory()
{
    // NOTHING
}

AnonyPassAuthenticatorPluginFactory::~AnonyPassAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
AnonyPassAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            AnonyPassAuthenticator::k_NAME);

    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) AnonyPassAuthenticator(config, allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
