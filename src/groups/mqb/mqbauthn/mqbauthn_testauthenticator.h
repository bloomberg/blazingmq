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

// mqbauthn_testauthenticator.h                                -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_TESTAUTHENTICATOR
#define INCLUDED_MQBAUTHN_TESTAUTHENTICATOR

/// @file mqbauthn_testauthenticator.h
///
/// @brief Provide a simple test authenticator plugin
///
/// @bbref{mqbauthn::TestAuthenticator} provides a simple authenticator
/// plugin designed for testing and development purposes.  It always accepts
/// authentication requests and supports configurable delays to simulate
/// slow authentication services.  This plugin is designed to be extensible
/// to support diverse integration testing scenarios.
/// @bbref{mqbauthn::TestAuthenticationResult} and
/// @bbref{mqbauthn::TestAuthenticatorPluginFactory} are the corresponding
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

// ==============================
// class TestAuthenticationResult
// ==============================

class TestAuthenticationResult : public mqbplug::AuthenticationResult {
  private:
    // DATA
    bsl::string                       d_principal;
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestAuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    TestAuthenticationResult(bsl::string_view   principal,
                             bsls::Types::Int64 lifetimeMs,
                             bslma::Allocator*  allocator);

    ~TestAuthenticationResult() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    bsl::string_view principal() const BSLS_KEYWORD_OVERRIDE;
    const bsl::optional<bsls::Types::Int64>&
    lifetimeMs() const BSLS_KEYWORD_OVERRIDE;
};

// =======================
// class TestAuthenticator
// =======================

class TestAuthenticator : public mqbplug::Authenticator {
  public:
    // PUBLIC CLASS DATA
    static const char* k_NAME;
    static const char* k_MECHANISM;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.TESTAUTHENTICATOR");

    // DATA
    bslma::Allocator* d_allocator_p;

    bool d_isStarted;

    int d_sleepTimeMs;  // Sleep time in milliseconds before authentication

  private:
    // NOT IMPLEMENTED
    TestAuthenticator(const TestAuthenticator& other) BSLS_KEYWORD_DELETED;
    TestAuthenticator&
    operator=(const TestAuthenticator& other) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestAuthenticator,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    TestAuthenticator(const mqbcfg::AuthenticatorPluginConfig* config,
                      bslma::Allocator*                        allocator = 0);

    /// Destructor.
    ~TestAuthenticator() BSLS_KEYWORD_OVERRIDE;

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

// ====================================
// class TestAuthenticatorPluginFactory
// ====================================

class TestAuthenticatorPluginFactory
: public mqbplug::AuthenticatorPluginFactory {
  public:
    // CREATORS
    TestAuthenticatorPluginFactory();
    ~TestAuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    bslma::ManagedPtr<mqbplug::Authenticator>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
