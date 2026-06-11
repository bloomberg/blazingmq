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
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbauthn {

const char* BasicCredentialProvider::k_NAME = "BasicCredentialProvider";

namespace {

// NOLINTNEXTLINE(*-avoid-c-arrays)
const char k_MECHANISM[] = "BASIC";

// ============================
// class BasicCredentialFunctor
// ============================

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class BasicCredentialFunctor {
    // DATA
    mqbplug::AuthnCredential d_credential;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BasicCredentialFunctor,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    BasicCredentialFunctor(bsl::string_view  username,
                           bsl::string_view  password,
                           bslma::Allocator* basicAllocator = 0)
    : d_credential(basicAllocator)
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    {
        bsl::string payload(username, basicAllocator);
        payload.append(":");
        payload.append(password);

        bsl::vector<char> data(payload.begin(), payload.end(), basicAllocator);

        d_credential = mqbplug::AuthnCredential(k_MECHANISM,
                                                data,
                                                basicAllocator);
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    BasicCredentialFunctor(const BasicCredentialFunctor& original,
                           bslma::Allocator*             basicAllocator = 0)
    : d_credential(original.d_credential, basicAllocator)
    {
    }

    // ACCESSORS
    bsl::optional<mqbplug::AuthnCredential> operator()(bsl::ostream&) const
    {
        return d_credential;
    }
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

}  // close unnamed namespace

// -----------------------------
// class BasicCredentialProvider
// -----------------------------

// CREATORS
BasicCredentialProvider::BasicCredentialProvider(
    bsl::string_view  username,
    bsl::string_view  password,
    bslma::Allocator* basicAllocator)
: d_username(username, basicAllocator)
, d_password(password, basicAllocator)
, d_isStarted(false)
, d_allocator_p(basicAllocator)
{
}

BasicCredentialProvider::~BasicCredentialProvider()
{
    BSLS_ASSERT_OPT(!d_isStarted &&
                    "stop() must be called before destroying this object");
}

// MANIPULATORS
mqbplug::CredentialProvider::CredentialCb BasicCredentialProvider::load()
{
    return BasicCredentialFunctor(d_username, d_password, d_allocator_p);
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

// ------------------------------------------
// class BasicCredentialProviderPluginFactory
// ------------------------------------------

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

    const mqbcfg::CredentialProviderConfig& cfg = providerCfg.value();
    if (cfg.name() != BasicCredentialProvider::k_NAME) {
        return bslma::ManagedPtr<mqbplug::CredentialProvider>();  // RETURN
    }

    bsl::string username(allocator);
    bsl::string password(allocator);

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (bsl::vector<mqbcfg::PluginSettingKeyValue>::const_iterator it =
             cfg.settings().cbegin();
         it != cfg.settings().cend();
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
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    return bslma::ManagedPtr<mqbplug::CredentialProvider>(
        bslma::ManagedPtrUtil::allocateManaged<BasicCredentialProvider>(
            allocator,
            username,
            password));
}

}  // close package namespace
}  // close enterprise namespace
