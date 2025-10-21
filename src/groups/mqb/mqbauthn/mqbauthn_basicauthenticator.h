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

// mqbauthn_basicauthenticator.h                               -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_BASICAUTHENTICATOR
#define INCLUDED_MQBAUTHN_BASICAUTHENTICATOR

/// @file mqbauthn_basicauthenticator.h
///
/// @brief Provide a basic authenticator plugin that uses username and
/// password.
///
/// @bbref{mqbauthn::BasicAuthenticator} provides a built-in authenticator
/// plugin that authenticates using username and password credentials.
/// It's designed for testing or development purposes and uses basic mechanism
/// to authenticate based on username and password.
/// The credential that's passed in as AuthenticationData is expected to be
/// a string of the form "username:password".  The colon character (:) is
/// forbidden in "username" but accepted in "password".
/// @bbref{mqbauthn::BasicAuthenticationResult} and
/// @bbref{mqbauthn::BasicAuthenticatorPluginFactory} are the corresponding
/// result and factory classes for the authenticator plugin.

// MQB
#include <mqbplug_authenticator.h>

// BDE
#include <ball_log.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_keyword.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcfg {
class AuthenticatorPluginConfig;
}

namespace mqbauthn {

// ===============================
// class BasicAuthenticationResult
// ===============================

class BasicAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BasicAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    BasicAuthenticationResult(bsl::string_view   principal,
                              bsls::Types::Int64 lifetimeMs,
                              bslma::Allocator*  allocator);

    ~BasicAuthenticationResult() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE;
    const bsl::optional<bsls::Types::Int64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE;
};

// ========================
// class BasicAuthenticator
// ========================

class BasicAuthenticator : public mqbplug::Authenticator {
  public:
    // PUBLIC CLASS DATA
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_NAME =
        "BasicAuthenticator";
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_MECHANISM =
        "BASIC";

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.BASICAUTHENTICATOR");

    // DATA
    bslma::Allocator* d_allocator_p;

    bsl::map<bsl::string, bsl::string> d_credentials;

    bool d_isStarted;

  private:
    // NOT IMPLEMENTED
    BasicAuthenticator(const BasicAuthenticator& other) BSLS_KEYWORD_DELETED;
    BasicAuthenticator&
    operator=(const BasicAuthenticator& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BasicAuthenticator,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    BasicAuthenticator(const mqbcfg::AuthenticatorPluginConfig* config,
                       bslma::Allocator*                        allocator = 0);

    /// Destructor.
    ~BasicAuthenticator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of the plugin.
    bsl::string_view name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the authentication mechanism the Authenticator supports.
    bsl::string_view mechanism() const BSLS_KEYWORD_OVERRIDE;

    /// Authenticate using the data provided in the specified `input`.
    /// - Return `0` on success, and populate the specified `result` with
    ///   client identity and its remaining lifetime if it has a fixed
    ///   duration.
    /// - Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes.
    int authenticate(bsl::ostream& errorDescription,
                     bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
                     const mqbplug::AuthenticationData& input) const
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the authenticator and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the authenticator.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

// =====================================
// class BasicAuthenticatorPluginFactory
// =====================================

class BasicAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    BasicAuthenticatorPluginFactory();
    ~BasicAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
