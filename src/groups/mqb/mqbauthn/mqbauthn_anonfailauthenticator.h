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

// mqbauthn_anonfailauthenticator.h                             -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_ANONFAILAUTHENTICATOR
#define INCLUDED_MQBAUTHN_ANONFAILAUTHENTICATOR

/// @file mqbauthn_anonfailauthenticator.h
///
/// @brief Provide an anonymous fail authenticator plugin.
///
/// @bbref{mqbauthn::AnonFailAuthenticator} provides a built-in authenticator
/// plugin that always fails authentication, regardless of the input
/// provided.  It's used as a testing authenticator or for blocking access
/// when authentication is required but should be rejected.
/// @bbref{mqbauthn::AnonFailAuthenticatorPluginFactory} is the corresponding
/// factory classes for the authenticator plugin.

// MQB
#include <mqbplug_authenticator.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsla_annotations.h>
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

// ==================================
// class AnonFailAuthenticationResult
// ==================================

class AnonFailAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AnonFailAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    AnonFailAuthenticationResult(bsl::string_view                  principal,
                                 bsl::optional<bsls::Types::Int64> lifetimeMs,
                                 bslma::Allocator*                 allocator);

    ~AnonFailAuthenticationResult() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE;
    const bsl::optional<bsls::Types::Int64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE;
};

// ===========================
// class AnonFailAuthenticator
// ===========================

class AnonFailAuthenticator : public mqbplug::Authenticator {
  public:
    // PUBLIC CLASS DATA
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_NAME =
        "AnonFailAuthenticator";
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_MECHANISM =
        "ANONYMOUS";

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.ANONFAILAUTHENTICATOR");

    // DATA
    bool d_isStarted;

  private:
    // NOT IMPLEMENTED
    AnonFailAuthenticator(const AnonFailAuthenticator& other)
        BSLS_KEYWORD_DELETED;
    AnonFailAuthenticator&
    operator=(const AnonFailAuthenticator& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AnonFailAuthenticator,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    AnonFailAuthenticator();

    /// Destructor.
    ~AnonFailAuthenticator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of the plugin.
    bsl::string_view name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the authentication mechanism the Authenticator supports.
    bsl::string_view mechanism() const BSLS_KEYWORD_OVERRIDE;

    /// Authenticate using the data provided in the specified `input`.
    /// This authenticator always fails authentication.
    /// - Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes.
    /// - The `result` will not be populated as authentication always fails.
    int authenticate(bsl::ostream& errorDescription,
                     bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
                     const mqbplug::AuthenticationData& input) const
        BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Start the Authenticator and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the Authenticator.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

// ========================================
// class AnonFailAuthenticatorPluginFactory
// ========================================

class AnonFailAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    AnonFailAuthenticatorPluginFactory();
    ~AnonFailAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
