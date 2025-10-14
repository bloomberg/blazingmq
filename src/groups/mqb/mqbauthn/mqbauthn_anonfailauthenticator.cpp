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

#include <mqbauthn_anonfailauthenticator.h>

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

// ---------------------------
// class AnonFailAuthenticator
// ---------------------------

AnonFailAuthenticator::AnonFailAuthenticator()
: d_isStarted(false)
{
}

AnonFailAuthenticator::~AnonFailAuthenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

bsl::string_view AnonFailAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view AnonFailAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int AnonFailAuthenticator::authenticate(
    bsl::ostream& errorDescription,
    BSLA_UNUSED bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "AnonFailAuthenticator: "
                  << "authentication failed for mechanism '" << mechanism()
                  << "' unconditionally.";

    errorDescription << "Authentication rejected by AnonFailAuthenticator";

    // Always return failure - do not populate result
    return -1;
}

int AnonFailAuthenticator::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;
    }

    d_isStarted = true;

    return 0;
}

void AnonFailAuthenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;
}

// ----------------------------------------
// class AnonFailAuthenticatorPluginFactory
// ----------------------------------------

AnonFailAuthenticatorPluginFactory::AnonFailAuthenticatorPluginFactory()
{
    // NOTHING
}

AnonFailAuthenticatorPluginFactory::~AnonFailAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
AnonFailAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    bslma::ManagedPtr<mqbplug::Authenticator> result(
        new (*allocator) AnonFailAuthenticator(),
        allocator);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
