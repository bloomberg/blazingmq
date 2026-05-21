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

#ifndef INCLUDED_MQBAUTHN_BASICCREDENTIALPROVIDER
#define INCLUDED_MQBAUTHN_BASICCREDENTIALPROVIDER

/// @file mqbauthn_basiccredentialprovider.h
///
/// @brief Provide a basic credential provider that supplies username and
/// password credentials for outbound broker-to-broker authentication.
///
/// @bbref{mqbauthn::BasicCredentialProvider} provides a built-in
/// credential provider plugin that supplies credentials using a configured
/// username and password.  When its credential function is invoked, it returns
/// an @bbref{mqbplug::AuthnCredential} with mechanism "BASIC" and data in the
/// form "username:password".
///
/// This provider reads `username` and `password` from its plugin configuration
/// settings (key-value pairs).

// MQB
#include <mqbplug_authncredential.h>
#include <mqbplug_credentialprovider.h>

// BDE
#include <ball_log.h>
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcfg {
class CredentialProviderPluginConfig;
}

namespace mqbauthn {

// =============================
// class BasicCredentialProvider
// =============================

class BasicCredentialProvider : public mqbplug::CredentialProvider {
  public:
    // PUBLIC CLASS DATA
    static const char* k_NAME;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.BASICCREDENTIALPROVIDER");

    // DATA
    bsl::string       d_username;
    bsl::string       d_password;
    bool              d_isStarted;
    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    BasicCredentialProvider(const BasicCredentialProvider&)
        BSLS_KEYWORD_DELETED;
    BasicCredentialProvider&
    operator=(const BasicCredentialProvider&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BasicCredentialProvider,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `BasicCredentialProvider` with the specified `username`
    /// and `password`, using the optionally specified `allocator` to supply
    /// memory.
    explicit BasicCredentialProvider(bsl::string_view  username,
                                     bsl::string_view  password,
                                     bslma::Allocator* basicAllocator = 0);

    /// Destructor.
    ~BasicCredentialProvider() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Return a credential-providing function.  The behavior is undefined
    /// unless `start()` has been called successfully.
    CredentialCb load() BSLS_KEYWORD_OVERRIDE;

    /// Start the provider and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the provider.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

// ========================================
// class BasicCredentialProviderPluginFactory
// ========================================

class BasicCredentialProviderPluginFactory
: public mqbplug::CredentialProviderPluginFactory {
  public:
    // CREATORS
    BasicCredentialProviderPluginFactory();
    ~BasicCredentialProviderPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::CredentialProvider>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
