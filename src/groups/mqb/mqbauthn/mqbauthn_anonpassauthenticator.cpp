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

#include <mqbauthn_anonpassauthenticator.h>

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
namespace mqbauthn {

// ----------------------------------
// class AnonPassAuthenticationResult
// ----------------------------------

AnonPassAuthenticationResult::AnonPassAuthenticationResult(
    bsl::string_view                  principal,
    bsl::optional<bsls::Types::Int64> lifetimeMs,
    bslma::Allocator*                 allocator)
: d_principal(principal, allocator)
, d_lifetimeMs(lifetimeMs)
{
}

AnonPassAuthenticationResult::~AnonPassAuthenticationResult()
{
}

bsl::string_view AnonPassAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
AnonPassAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// ---------------------------
// class AnonPassAuthenticator
// ---------------------------

AnonPassAuthenticator::AnonPassAuthenticator(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
{
}

AnonPassAuthenticator::~AnonPassAuthenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

bsl::string_view AnonPassAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view AnonPassAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int AnonPassAuthenticator::authenticate(
    BSLA_UNUSED bsl::ostream&                       errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "AnonPassAuthenticator: "
                  << "authentication passed for mechanism '" << mechanism()
                  << "' unconditionally.";

    // No `lifetime` is returned since we don't expect a user to reauthenticate
    // if they don't know how to authenticate in the first place.
    *result = bsl::allocate_shared<AnonPassAuthenticationResult>(d_allocator_p,
                                                                 "",
                                                                 bsl::nullopt);
    return 0;
}

int AnonPassAuthenticator::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;
    }

    d_isStarted = true;

    return 0;
}

void AnonPassAuthenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;
}

// ----------------------------------------
// class AnonPassAuthenticatorPluginFactory
// ----------------------------------------

AnonPassAuthenticatorPluginFactory::AnonPassAuthenticatorPluginFactory()
{
    // NOTHING
}

AnonPassAuthenticatorPluginFactory::~AnonPassAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
AnonPassAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) AnonPassAuthenticator(allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
