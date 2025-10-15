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
#include <bsl_string_view.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqauthnfail {

namespace {

const int k_TIMEOUT_SECONDS = 600;

}  // namespace

// ------------------------------
// class FailAuthenticationResult
// ------------------------------

FailAuthenticationResult::FailAuthenticationResult(
    bsl::string_view   principal,
    bsls::Types::Int64 lifetimeMs,
    bslma::Allocator*  allocator)
: d_principal(principal, allocator)
, d_lifetimeMs(lifetimeMs)
{
}

FailAuthenticationResult::~FailAuthenticationResult()
{
}

bsl::string_view FailAuthenticationResult::principal() const
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

bsl::string_view FailAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view FailAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int FailAuthenticator::authenticate(
    bsl::ostream&                                   errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "FailAuthenticator: "
                  << "authentication failed for mechanism '" << mechanism()
                  << "' unconditionally.";

    errorDescription << "FailAuthenticator: "
                     << "authentication failed for mechanism '" << mechanism()
                     << "' unconditionally.";

    *result = bsl::allocate_shared<FailAuthenticationResult>(
        d_allocator_p,
        "",
        k_TIMEOUT_SECONDS * bdlt::TimeUnitRatio::k_MILLISECONDS_PER_SECOND);
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

    if (!config) {
        return bslma::ManagedPtr<mqbplug::Authenticator>();
    }

    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) FailAuthenticator(config, allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
