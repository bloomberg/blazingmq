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

#ifndef INCLUDED_MQBAUTHN_BASICAUTHORIZER
#define INCLUDED_MQBAUTHN_BASICAUTHORIZER

/// @file mqbauthn_basicauthorizer.h
///
/// @brief Provide a basic authorizer plugin that uses username and
/// password.
///
/// @bbref{mqbauthn::BasicAuthorizer} provides a built-in authorizer
/// plugin that authorizes all actions. It's designed to represent the current
/// authorization policy, which allows all clients access.
///
/// @bbref{mqbauthn::BasicAuthorizerPluginFactory} is the corresponding factory
/// classes for the Authorizer plugin.

// MQB
#include <mqbplug_authenticator.h>
#include <mqbplug_authorizer.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION

namespace mqbauthz {

// ========================
// class BasicAuthorizer
// ========================

class BasicAuthorizer : public mqbplug::Authorizer {
  public:
    // CLASS DATA

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHN.BASICAUTHORIZER");

  public:
    // CREATORS

    /// Destructor.
    ~BasicAuthorizer() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of the plugin.
    bsl::string_view name() const BSLS_KEYWORD_OVERRIDE;

    /// Check if the supplied action is allowed based on the result of
    /// authentication. This authorizer always allows all actions regardless of
    /// the identity of the client.
    ///
    /// @param action The action being authorized
    /// @param authnResult The result of an authenticated connection
    bool authorize(const mqbplug::Action&               action,
                   const mqbplug::AuthenticationResult& authnResult)
        BSLS_KEYWORD_OVERRIDE;
};

// =====================================
// class BasicAuthorizerPluginFactory
// =====================================

class BasicAuthorizerPluginFactory : public mqbplug::AuthorizerPluginFactory {
  public:
    // CREATORS
    ~BasicAuthorizerPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create a `BasicAuthorizer` using the supplied allocator.
    bslma::ManagedPtr<mqbplug::Authorizer>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
