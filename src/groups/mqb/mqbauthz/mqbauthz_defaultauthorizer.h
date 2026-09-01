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

#ifndef INCLUDED_MQBAUTHZ_DEFAULTAUTHORIZER
#define INCLUDED_MQBAUTHZ_DEFAULTAUTHORIZER

/// @file mqbauthz_defaultauthorizer.h
///
/// @brief Provide the built-in default authorizer plugin.
///
/// @bbref{mqbauthz::DefaultAuthorizer} provides the built-in default
/// authorizer plugin that authorizes all actions. It's designed to
/// represent the current authorization policy, which allows all access.
///
/// @bbref{mqbauthz::DefaultAuthorizerPluginFactory} is the corresponding
/// factory class for the Authorizer plugin.

// MQB
#include <mqbauthz_policy.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_authorizer.h>
#include <mqbpoly_policies.h>

// BDE
#include <ball_log.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsls_keyword.h>

namespace BloombergLP {

// FORWARD DECLARATION

namespace mqbact {
class Action;
}

namespace mqbcfg {
class AuthorizerPluginConfig;
}

namespace mqbauthz {

// =======================
// class DefaultAuthorizer
// =======================

class DefaultAuthorizer : public mqbplug::Authorizer {
  public:
    // CLASS DATA
    static bsl::string_view k_NAME;

    // DATA
    Policy d_policy;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHZ.DEFAULTAUTHORIZER");

  public:
    // CREATORS

    /// Create a `DefaultAuthorizer` using the optionally specified
    /// `config`.
    explicit DefaultAuthorizer(
        const mqbcfg::AuthorizerPluginConfig* config = 0);

    /// Destructor.
    ~DefaultAuthorizer() BSLS_KEYWORD_OVERRIDE;

    // ACCESSORS

    /// Return the name of the plugin.
    bsl::string_view name() const BSLS_KEYWORD_OVERRIDE;

    /// Check if the supplied action is allowed based on the result of
    /// authentication. This authorizer always allows all actions regardless of
    /// the identity of the client.
    ///
    /// @param action The action being authorized
    /// @param authnResult The result of an authenticated connection
    bool authorize(const mqbact::Action&                action,
                   const mqbplug::AuthenticationResult& authnResult)
        BSLS_KEYWORD_OVERRIDE;
};

// ====================================
// class DefaultAuthorizerPluginFactory
// ====================================

class DefaultAuthorizerPluginFactory
: public mqbplug::AuthorizerPluginFactory {
  public:
    // CREATORS
    ~DefaultAuthorizerPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Create a `DefaultAuthorizer` using the supplied allocator.
    bslma::ManagedPtr<mqbplug::Authorizer>
    create(bslma::Allocator* allocator) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
