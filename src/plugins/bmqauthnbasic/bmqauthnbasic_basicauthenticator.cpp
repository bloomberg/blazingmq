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

// MQB
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <ball_log.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqauthnbasic {

// -----------------------------
// class BasicAuthenticationData
// -----------------------------

BasicAuthenticationData::BasicAuthenticationData(
    const bsl::vector<char>& authPayload,
    const bsl::string&       clientIpAddress)
: d_authPayload(authPayload)
, d_clientIpAddress(clientIpAddress)
{
    // NOTHING
}

BasicAuthenticationData::~BasicAuthenticationData()
{
    // NOTHING
}

// ------------------------
// class BasicAuthenticator
// ------------------------

BasicAuthenticator::BasicAuthenticator(bslma::Allocator* allocator)
: d_authenticatorConfig_p(
      mqbplug::AuthenticatorUtil::findAuthenticatorConfig(name()))
, d_isStarted(false)
, d_allocator_p(allocator)
{
}

BasicAuthenticator::~BasicAuthenticator()
{
    stop();
}

int BasicAuthenticator::authenticate(
    bsl::ostream&                      errorDescription,
    mqbplug::AuthenticationResult*     result,
    const mqbplug::AuthenticationData& input) const
{
    BSLS_ASSERT_SAFE(result);

    bsl::string principal(input.authPayload().data());

    result->setLifetimeMs(600 * 1000);
    result->setPrincipal(principal);

    return 0;
}

int BasicAuthenticator::start(bsl::ostream& errorDescription)
{
    if (!d_authenticatorConfig_p) {
        errorDescription << "Could not find config for StatConsumer '"
                         << name() << "'";
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
    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) BasicAuthenticator(allocator),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
