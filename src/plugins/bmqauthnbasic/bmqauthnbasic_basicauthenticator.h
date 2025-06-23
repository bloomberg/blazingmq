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

// bmqauthnbasic_basicauthenticator.h                             -*-C++-*-
#ifndef INCLUDED_AUTHNBASIC_BASICAUTHENTICATOR
#define INCLUDED_AUTHNBASIC_BASICAUTHENTICATOR

//@PURPOSE: Provide a plugin that authenticates using username and password.
//
//@CLASSES:
//  bmqauthnbasic::BasicAuthenticator: Authenticator plugin that
//  authenticates using username and password.
//
//@DESCRIPTION:
//  'bmqauthnbasic::BasicAuthenticator' implements an authenticator for
//  testing or development purposes. It uses basic mechanism and authenticate
//  based on username and password.
//  The credential that's passed in as AuthenticationData is expected to be
//  a string of the form "username:password".

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>

// BMQ

// BDE
#include <ball_log.h>
#include <bsl_map.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsla_annotations.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqauthnbasic {

// ===============================
// class BasicAuthenticationResult
// ===============================

class BasicAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;
    BSLA_UNUSED bslma::Allocator* d_allocator_p;

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
    static constexpr const char* k_NAME = "BasicAuthenticator";

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQAUTHNBASIC.BASICAUTHENTICATOR");

    // DATA
    const mqbcfg::AuthenticatorPluginConfig* d_authenticatorConfig_p;

    bool d_isStarted;

    bsl::map<bsl::string, bsl::string> d_credentials;

    bslma::Allocator* d_allocator_p;

  public:
    // NOT IMPLEMENTED
    BasicAuthenticator(const BasicAuthenticator& other)            = delete;
    BasicAuthenticator& operator=(const BasicAuthenticator& other) = delete;

    // CREATORS

    BasicAuthenticator(const mqbcfg::AuthenticatorPluginConfig* config,
                       bslma::Allocator*                        allocator);

    /// Destructor.
    ~BasicAuthenticator() BSLS_KEYWORD_OVERRIDE;

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
