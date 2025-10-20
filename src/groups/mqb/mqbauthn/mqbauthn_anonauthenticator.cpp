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

#include <mqbauthn_anonauthenticator.h>

// MQB
#include <mqbcfg_messages.h>
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
namespace mqbauthn {

// ------------------------------
// class AnonAuthenticationResult
// ------------------------------

AnonAuthenticationResult::AnonAuthenticationResult(
    bsl::string_view                  principal,
    bsl::optional<bsls::Types::Int64> lifetimeMs,
    bslma::Allocator*                 allocator)
: d_principal(principal, allocator)
, d_lifetimeMs(lifetimeMs)
{
}

AnonAuthenticationResult::~AnonAuthenticationResult()
{
}

bsl::string_view AnonAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
AnonAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// -----------------------
// class AnonAuthenticator
// -----------------------

AnonAuthenticator::AnonAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_shouldPass(true)
{
    if (!config) {
        return;
    }

    // Load the configured `shouldPass` argument
    bsl::vector<mqbcfg::PluginConfigKeyValue>::const_iterator it =
        config->configs().cbegin();
    for (; it != config->configs().cend(); ++it) {
        if (it->key() == "shouldPass" && it->value().isBoolValValue()) {
            d_shouldPass = it->value().boolVal();
        }
    }
}

AnonAuthenticator::~AnonAuthenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

bsl::string_view AnonAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view AnonAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int AnonAuthenticator::authenticate(
    bsl::ostream&                                   errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    if (d_shouldPass) {
        BALL_LOG_INFO << "AnonAuthenticator: "
                      << "authentication passed for mechanism '" << mechanism()
                      << "' unconditionally (shouldPass=true).";

        // No `lifetime` is returned since we don't expect a user to
        // reauthenticate if they don't know how to authenticate in the first
        // place.
        *result = bsl::allocate_shared<AnonAuthenticationResult>(d_allocator_p,
                                                                 "",
                                                                 bsl::nullopt);
        return 0;  // RETURN
    }
    else {
        BALL_LOG_INFO << "AnonAuthenticator: "
                      << "authentication failed for mechanism '" << mechanism()
                      << "' unconditionally (shouldPass=false).";

        errorDescription << "Authentication rejected by AnonAuthenticator";

        // Always return failure - do not populate result
        return -1;  // RETURN
    }
}

int AnonAuthenticator::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;  // RETURN
    }

    d_isStarted = true;

    return 0;
}

void AnonAuthenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;
}

// ------------------------------------
// class AnonAuthenticatorPluginFactory
// ------------------------------------

AnonAuthenticatorPluginFactory::AnonAuthenticatorPluginFactory()
{
    // NOTHING
}

AnonAuthenticatorPluginFactory::~AnonAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
AnonAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            AnonAuthenticator::k_NAME);

    bslma::ManagedPtr<mqbplug::Authenticator> result =
        bslma::ManagedPtrUtil::allocateManaged<AnonAuthenticator>(allocator,
                                                                  config);

    return result;
}

}  // close package namespace
}  // close enterprise namespace
