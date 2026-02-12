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

#include <mqbauthn_testauthenticator.h>

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>

// BMQ
#include <bmqtsk_alarmlog.h>

// BDE
#include <bdlt_timeunitratio.h>
#include <bsl_iostream.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string_view.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslmt_threadutil.h>
#include <bsls_assert.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {
namespace mqbauthn {

const char* TestAuthenticator::k_NAME      = "TestAuthenticator";
const char* TestAuthenticator::k_MECHANISM = "TEST";

namespace {

const int k_AUTHN_DURATION_SECONDS = 600;

}  // namespace

// ------------------------------
// class TestAuthenticationResult
// ------------------------------

TestAuthenticationResult::TestAuthenticationResult(
    bsl::string_view   principal,
    bsls::Types::Int64 lifetimeMs,
    bslma::Allocator*  allocator)
: d_principal(principal, allocator)
, d_lifetimeMs(lifetimeMs)
{
}

TestAuthenticationResult::~TestAuthenticationResult()
{
}

bsl::string_view TestAuthenticationResult::principal() const
{
    return d_principal;
}

const bsl::optional<bsls::Types::Int64>&
TestAuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// -----------------------
// class TestAuthenticator
// -----------------------

TestAuthenticator::TestAuthenticator(
    const mqbcfg::AuthenticatorPluginConfig* config,
    bslma::Allocator*                        allocator)
: d_allocator_p(allocator)
, d_isStarted(false)
, d_sleepTimeMs(0)
{
    if (!config) {
        BALL_LOG_INFO << "No configuration provided, using defaults";
        return;
    }

    // Load sleep time configuration
    bsl::vector<mqbcfg::PluginSettingKeyValue>::const_iterator it =
        config->settings().cbegin();
    for (; it != config->settings().cend(); ++it) {
        if (it->key() == "sleepTimeMs") {
            if (!it->value().isIntValValue()) {
                BALL_LOG_WARN << "Expected int for sleepTimeMs, got type id = "
                              << it->value().selectionId();
                continue;
            }
            d_sleepTimeMs = it->value().intVal();
            BALL_LOG_INFO << "TestAuthenticator: sleepTimeMs = "
                          << d_sleepTimeMs;
        }
    }
}

TestAuthenticator::~TestAuthenticator()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

bsl::string_view TestAuthenticator::name() const
{
    return k_NAME;
}

bsl::string_view TestAuthenticator::mechanism() const
{
    return k_MECHANISM;
}

int TestAuthenticator::authenticate(
    BSLA_UNUSED bsl::ostream&                       errorDescription,
    bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
    BSLA_UNUSED const mqbplug::AuthenticationData& input) const
{
    BALL_LOG_INFO << "TestAuthenticator: authentication using mechanism '"
                  << mechanism() << "'.";

    // Apply sleep delay if configured
    if (d_sleepTimeMs > 0) {
        BALL_LOG_INFO << "TestAuthenticator: sleeping for " << d_sleepTimeMs
                      << " milliseconds";
        bsls::TimeInterval sleepInterval;
        sleepInterval.setTotalMilliseconds(d_sleepTimeMs);
        bslmt::ThreadUtil::sleep(sleepInterval);
    }

    // Always accept
    BALL_LOG_INFO << "TestAuthenticator: authentication successful";

    *result = bsl::allocate_shared<TestAuthenticationResult>(
        d_allocator_p,
        "TEST_USER",
        k_AUTHN_DURATION_SECONDS *
            bdlt::TimeUnitRatio::k_MILLISECONDS_PER_SECOND);
    return 0;
}

int TestAuthenticator::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;
    }

    d_isStarted = true;

    BALL_LOG_INFO << "TestAuthenticator started (always accepts)"
                  << ", sleepTimeMs=" << d_sleepTimeMs;

    return 0;
}

void TestAuthenticator::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    BALL_LOG_INFO << "TestAuthenticator stopped";
}

// ------------------------------------
// class TestAuthenticatorPluginFactory
// ------------------------------------

TestAuthenticatorPluginFactory::TestAuthenticatorPluginFactory()
{
    // NOTHING
}

TestAuthenticatorPluginFactory::~TestAuthenticatorPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::Authenticator>
TestAuthenticatorPluginFactory::create(bslma::Allocator* allocator)
{
    const mqbcfg::AuthenticatorPluginConfig* config =
        mqbplug::AuthenticatorUtil::findAuthenticatorConfig(
            TestAuthenticator::k_NAME);

    // Return null if no config found - this authenticator is not configured
    if (!config) {
        return bslma::ManagedPtr<mqbplug::Authenticator>();
    }

    allocator = bslma::Default::allocator(allocator);

    return bslma::ManagedPtr<mqbplug::Authenticator>(
        new (*allocator) TestAuthenticator(config, allocator),
        allocator);
}

}  // close package namespace
}  // close enterprise namespace
