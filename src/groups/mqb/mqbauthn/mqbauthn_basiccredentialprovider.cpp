// Copyright 2026 Bloomberg Finance L.P.
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

#include <mqbauthn_basiccredentialprovider.h>

#include <mqbscm_version.h>

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_authncredential.h>

// BDE
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_managedptr.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbauthn {

const char* BasicCredentialProvider::k_NAME      = "BasicCredentialProvider";
const char* BasicCredentialProvider::k_MECHANISM = "BASIC";

// ------------------------
// class CredentialFunctor
// ------------------------

BasicCredentialProvider::CredentialFunctor::CredentialFunctor(
    const bsl::string& mechanism,
    const bsl::string& username,
    const bsl::string& password)
: d_mechanism(mechanism)
, d_username(username)
, d_password(password)
{
}

bsl::optional<mqbplug::AuthnCredential>
BasicCredentialProvider::CredentialFunctor::operator()(
    bsl::ostream& error) const
{
    if (d_username.empty()) {
        error << "BasicCredentialProvider: username is empty";
        return bsl::nullopt;  // RETURN
    }

    bsl::string payload(d_username);
    payload.append(":");
    payload.append(d_password);

    bsl::vector<char> data(payload.begin(), payload.end());

    return mqbplug::AuthnCredential(d_mechanism, data);
}

// -----------------------------
// class BasicCredentialProvider
// -----------------------------

// CREATORS
BasicCredentialProvider::BasicCredentialProvider(bsl::string_view  username,
                                                 bsl::string_view  password,
                                                 bslma::Allocator* allocator)
: d_username(username, allocator)
, d_password(password, allocator)
, d_isStarted(false)
{
}

BasicCredentialProvider::~BasicCredentialProvider()
{
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

// MANIPULATORS
mqbplug::CredentialProvider::CredentialFunc BasicCredentialProvider::load()
{
    return CredentialFunctor(k_MECHANISM, d_username, d_password);
}

int BasicCredentialProvider::start(bsl::ostream& errorDescription)
{
    if (d_isStarted) {
        errorDescription << "start() can only be called once on this object";
        return -1;  // RETURN
    }

    if (d_username.empty()) {
        errorDescription << "BasicCredentialProvider: username is empty";
        return -1;  // RETURN
    }

    d_isStarted = true;

    BALL_LOG_INFO << "BasicCredentialProvider started for user '" << d_username
                  << "'";

    return 0;
}

void BasicCredentialProvider::stop()
{
    if (!d_isStarted) {
        return;  // RETURN
    }

    d_isStarted = false;

    BALL_LOG_INFO << "BasicCredentialProvider stopped";
}

// ----------------------------------------
// class BasicCredentialProviderPluginFactory
// ----------------------------------------

BasicCredentialProviderPluginFactory::BasicCredentialProviderPluginFactory()
{
    // NOTHING
}

BasicCredentialProviderPluginFactory::~BasicCredentialProviderPluginFactory()
{
    // NOTHING
}

bslma::ManagedPtr<mqbplug::CredentialProvider>
BasicCredentialProviderPluginFactory::create(bslma::Allocator* allocator)
{
    const bdlb::NullableValue<mqbcfg::CredentialProviderConfig>& providerCfg =
        mqbcfg::BrokerConfig::get().authentication().credentialProvider();

    if (providerCfg.isNull()) {
        return bslma::ManagedPtr<mqbplug::CredentialProvider>();  // RETURN
    }

    if (providerCfg.value().name() != BasicCredentialProvider::k_NAME) {
        return bslma::ManagedPtr<mqbplug::CredentialProvider>();  // RETURN
    }

    allocator = bslma::Default::allocator(allocator);

    bsl::string username(allocator);
    bsl::string password(allocator);

    for (bsl::vector<mqbcfg::PluginSettingKeyValue>::const_iterator it =
             providerCfg.value().settings().cbegin();
         it != providerCfg.value().settings().cend();
         ++it) {
        if (!it->value().isStringValValue()) {
            continue;  // CONTINUE
        }
        if (it->key() == "username") {
            username = it->value().stringVal();
        }
        else if (it->key() == "password") {
            password = it->value().stringVal();
        }
    }

    return bslma::ManagedPtr<mqbplug::CredentialProvider>(
        new (*allocator)
            BasicCredentialProvider(username, password, allocator),
        allocator);
}

}  // close package namespace
}  // close enterprise namespace
