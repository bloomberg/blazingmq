// Copyright 2017-2025 Bloomberg Finance L.P.
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

// mqbauthn_authenticationcontroller.h                          -*-C++-*-
#ifndef INCLUDED_MQBAUTHN_AUTHENTICATIONCONTROLLER
#define INCLUDED_MQBAUTHN_AUTHENTICATIONCONTROLLER

/// @file mqbauthn_authenticationcontroller.h
///
/// @brief Provide a small utility class that orchestrates authenticator
/// plugins.
///
/// @bbref{AuthenticationController} provides a small utility class that
/// orchestrates authenticator plugins. Responsibilities include creating and
/// holding plugin instances, tracking an optional anonymous credential, and
/// offering start/stop and authenticate operations used by higher-level
/// components.

// MQB
#include <mqbcfg_messages.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginmanager.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_unordered_set.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace mqbauthn {

// ==============================
// class AuthenticationController
// ==============================

class AuthenticationController {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.AUTHENTICATIONCONTROLLER");

  private:
    // PRIVATE TYPES
    typedef bslma::ManagedPtr<mqbplug::Authenticator>        AuthenticatorMp;
    typedef bsl::unordered_map<bsl::string, AuthenticatorMp> AuthenticatorMap;

    // DATA

    /// Registered authenticator plugins.
    /// Mapping an authentication mechanism to a mqbplug::Authenticator.
    /// The mechanisms are stored in upper case to make the lookup
    /// case-insensitive.
    AuthenticatorMap d_authenticators;

    /// Anonymous credential. If set, this credential will be used for
    /// authentication when the client does not provide any credential.
    /// Default anonymous credential will be used if not set.
    bsl::optional<mqbcfg::Credential> d_anonymousCredential;

    /// Used to instantiate Authenticator
    /// plugins at start-time.
    mqbplug::PluginManager* d_pluginManager_p;

    /// True if this component is started.
    bool d_isStarted;

    /// Allocator to use.
    bslma::Allocator* d_allocator_p;

  private:
    // PRIVATE MANIPULATORS

  private:
    // NOT IMPLEMENTED
    AuthenticationController(const AuthenticationController& other)
        BSLS_CPP11_DELETED;
    AuthenticationController&
    operator=(const AuthenticationController& other) BSLS_CPP11_DELETED;

    // PRIVATE MANIPULATORS

    /// Set anonymous credential from config.
    int setAnonymousCredential(bsl::ostream& errorDescription);

    /// Initialize authenticators from configuration.
    int initializeAuthenticators(bsl::ostream& errorDescription);

    /// Collect all available plugin factories (built-in and external).
    /// Load them into the specified `pluginFactories` collection.
    /// Return 0 on success, non-zero on error.
    int collectAvailablePluginFactories(
        bsl::unordered_set<mqbplug::PluginFactory*>* pluginFactories);

    /// Create authenticators based on configuration from the specified
    /// `authenticatorConfig` using the specified `pluginFactories`.
    /// Return 0 on success, non-zero on error.
    int createConfiguredAuthenticators(
        bsl::ostream&                                      errorDescription,
        const bsl::unordered_set<mqbplug::PluginFactory*>& pluginFactories,
        const mqbcfg::AuthenticatorConfig& authenticatorConfig);

    /// Validate that anonymous credential mechanism matches a configured
    /// authenticator.  Return 0 on success, non-zero on error.
    int validateAnonymousCredential(
        bsl::ostream&                      errorDescription,
        const mqbcfg::AuthenticatorConfig& authenticatorConfig);

    /// Ensure at least one authenticator is available for default
    /// authentication, adding default AnonPassAuthenticator if none are
    /// configured. Return 0 on success, non-zero on error.
    int ensureDefaultAuthenticator(bsl::ostream& errorDescription);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticationController,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    AuthenticationController(mqbplug::PluginManager* pluginManager,
                             bslma::Allocator*       allocator);

    // MANIPULATORS

    /// Start the AuthenticationController.  Return 0 on success, or a non-zero
    /// return code on error and fill in the specified `errorDescription`
    /// stream with the description of the error.
    int start(bsl::ostream& errorDescription);

    /// Stop the AuthenticationController.
    void stop();

    /// Authenticate using the specified AuthenticationData `input` and
    /// `mechanism`.  On success, populate the specified `result` with the
    /// authentication result.
    /// Return 0 on success, or a non-zero return code on error and fill in the
    /// specified `errorDescription` stream with the description of the error.
    /// Note that the `mechanism` is case insensitive.
    int authenticate(bsl::ostream& errorDescription,
                     bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
                     bsl::string_view                                mechanism,
                     const mqbplug::AuthenticationData&              input);

    /// Return the anonymous credential used for authentication.
    /// If no anonymous credential is set, return an empty optional.
    const bsl::optional<mqbcfg::Credential>& anonymousCredential();
};

}  // close package namespace
}  // close enterprise namespace

#endif
