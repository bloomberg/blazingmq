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

// mqbauthn_anonauthenticator.h                             -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_ANONAUTHENTICATOR
#define INCLUDED_MQBAUTHN_ANONAUTHENTICATOR

/// @file mqbauthn_anonauthenticator.h
///
/// @brief Provide a configurable anonymous authenticator plugin.
///
/// @bbref{mqbauthn::AnonAuthenticator} provides a built-in authenticator
/// plugin that can be configured to either always succeed or always fail
/// authentication.  When configured to pass (or default with no
/// configuration), it allows anonymous access without credentials. When
/// configured to fail, it blocks all authentication attempts. This is useful
/// for testing or controlling anonymous access.
/// @bbref{mqbauthn::AnonAuthenticatorPluginFactory} is the corresponding
/// factory class for the authenticator plugin.

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

// ==============================
// class AnonAuthenticationResult
// ==============================

class AnonAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AnonAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    AnonAuthenticationResult(bsl::string_view                  principal,
                             bsl::optional<bsls::Types::Int64> lifetimeMs,
                             bslma::Allocator*                 allocator);

    ~AnonAuthenticationResult() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE;
    const bsl::optional<bsls::Types::Int64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE;
};

// =======================
// class AnonAuthenticator
// =======================

class AnonAuthenticator : public mqbplug::Authenticator {
  public:
    // PUBLIC CLASS DATA
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_NAME =
        "AnonAuthenticator";
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_MECHANISM =
        "ANONYMOUS";

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.ANONAUTHENTICATOR");

    // DATA
    bslma::Allocator* d_allocator_p;

    bool d_isStarted;

    /// If true, authentication always succeeds; if false, always fails.
    bool d_shouldPass;

  private:
    // NOT IMPLEMENTED
    AnonAuthenticator(const AnonAuthenticator& other) BSLS_KEYWORD_DELETED;
    AnonAuthenticator&
    operator=(const AnonAuthenticator& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AnonAuthenticator,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct an AnonAuthenticator with the specified `shouldPass`
    /// behavior flag and optionally specified `allocator`.  If `shouldPass`
    /// is true, authentication will always succeed; if false, authentication
    /// will always fail.
    AnonAuthenticator(bool shouldPass                                 = true,
                      const mqbcfg::AuthenticatorPluginConfig* config = 0,
                      bslma::Allocator*                        allocator = 0);

    /// Destructor.
    ~AnonAuthenticator() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of the plugin.
    bsl::string_view name() const BSLS_KEYWORD_OVERRIDE;

    /// Return the authentication mechanism the Authenticator supports.
    bsl::string_view mechanism() const BSLS_KEYWORD_OVERRIDE;

    /// Authenticate using the data provided in the specified `input`.
    /// Behavior depends on the `shouldPass` configuration:
    /// - If `shouldPass` is true:
    ///   Return `0` on success, and populate the specified `result` with
    ///   client identity and its remaining lifetime if it has a fixed
    ///   duration.
    /// - If `shouldPass` is false:
    ///   Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes. The `result` will not be populated.
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

// ====================================
// class AnonAuthenticatorPluginFactory
// ====================================

class AnonAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    explicit AnonAuthenticatorPluginFactory();

    ~AnonAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
