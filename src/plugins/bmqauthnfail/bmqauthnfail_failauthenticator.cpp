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

#include <bmqauthnfail_failauthenticator.h>

// BMQAUTHNFAIL
#include <bmqauthnfail_version.h>

// MQB
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsla_unused.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqauthnfail {

// ------------------------------
// class FailAuthenticationResult
// ------------------------------

FailAuthenticationResult::FailAuthenticationResult(
    const bslstl::StringRef& principal,
    bsls::Types::Int64       lifetimeMs,
    bslma::Allocator*        allocator)
: d_principal(principal)
, d_lifetimeMs(lifetimeMs)
, d_allocator_p(allocator)
{
}

FailAuthenticationResult::~FailAuthenticationResult()
{
}

bslstl::StringRef FailAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
FailAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// -----------------------
// class FailAuthenticator
// -----------------------

FailAuthenticator::FailAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_authenticatorConfig_p(config)
, d_isStarted(false)
, d_allocator_p(allocator)
{
}

FailAuthenticator::~FailAuthenticator()
{
    stop();
}

bslstl::StringRef FailAuthenticator::name() const
{
    return k_NAME;
}

bslstl::StringRef FailAuthenticator::mechanism() const
{
    return "Basic";
}

int FailAuthenticator::authenticate(
    BSLA_UNUSED bsl::ostream&                       errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    *result = bsl::allocate_shared<FailAuthenticationResult>(d_allocator_p,
                                                             "",
                                                             600 * 1000);
    return -1;
}

int FailAuthenticator::start(bsl::ostream& errorDescription)
{
    if (!d_authenticatorConfig_p) {
        errorDescription << "Could not find config for FailAuthenticator '"
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

void FailAuthenticator::stop()
{
    d_isStarted = false;
}

// ------------------------------------
// class FailAuthenticatorPluginFactory
// ------------------------------------

FailAuthenticatorPluginFactory::FailAuthenticatorPluginFactory()
{
    // NOTHING
}

FailAuthenticatorPluginFactory::~FailAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
FailAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            FailAuthenticator::k_NAME);

    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) FailAuthenticator(config, allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
