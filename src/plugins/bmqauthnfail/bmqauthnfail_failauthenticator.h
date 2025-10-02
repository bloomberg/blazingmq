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

// bmqauthnfail_failauthenticator.h                             -*-C++-*-
#ifndef INCLUDED_AUTHNFAIL_FAILAUTHENTICATOR
#define INCLUDED_AUTHNFAIL_FAILAUTHENTICATOR

//@PURPOSE: Provide a plugin that always authenticates unsuccessfully.
//
//@CLASSES:
//  bmqauthnfail::FailAuthenticator: Authenticator plugin that unconditionally
//  rejects any authentication request.
//
//@DESCRIPTION:
//  'bmqauthnfail::FailAuthenticator' implements a dummy authenticator for
//  testing or development purposes. It always returns failure, regardless of
//  the input provided.

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsla_annotations.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqauthnfail {

// ==========================
// class AuthenticationResult
// ==========================

class FailAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FailAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    FailAuthenticationResult(bsl::string_view   principal,
                             bsls::Types::Int64 lifetimeMs,
                             bslma::Allocator*  allocator);

    ~FailAuthenticationResult() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE;
    const bsl::optional<bsls::Types::Int64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE;
};

// =======================
// class FailAuthenticator
// =======================

class FailAuthenticator : public mqbplug::Authenticator {
  public:
    // PUBLIC CLASS DATA
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_NAME =
        "FailAuthenticator";
    static BSLS_KEYWORD_INLINE_CONSTEXPR bsl::string_view k_MECHANISM =
        "Basic";

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQAUTHNFAIL.FAILAUTHENTICATOR");

    // DATA
    const mqbcfg::AuthenticatorPluginConfig* d_authenticatorConfig_p;

    bool d_isStarted;

    bslma::Allocator* d_allocator_p;

  private:
    // NOT IMPLEMENTED
    FailAuthenticator(const FailAuthenticator& other) BSLS_KEYWORD_DELETED;
    FailAuthenticator&
    operator=(const FailAuthenticator& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FailAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    FailAuthenticator(const mqbcfg::AuthenticatorPluginConfig* config,
                      bslma::Allocator*                        allocator);

    /// Destructor.
    ~FailAuthenticator() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

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

    /// Start the Authenticator and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    int start(bsl::ostream& errorDescription) BSLS_KEYWORD_OVERRIDE;

    /// Stop the Authenticator.
    void stop() BSLS_KEYWORD_OVERRIDE;
};

// ====================================
// class FailAuthenticatorPluginFactory
// ====================================

class FailAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    FailAuthenticatorPluginFactory();
    ~FailAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
