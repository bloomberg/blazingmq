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

#ifndef INCLUDED_MQBAUTHZ_AUTHORIZATIONCONTROLLER
#define INCLUDED_MQBAUTHZ_AUTHORIZATIONCONTROLLER

/// @file mqbauthn_authorizationcontroller.h
///
/// @brief Provide an utility class that orchestrates authorization
/// plugins.

// MQB
#include <mqbplug_authorizer.h>
#include <mqbplug_pluginmanager.h>
#include <mqbplug_plugintype.h>

// BDE
#include <ball_log.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_string_view.h>
#include <bsl_unordered_set.h>
#include <bslma_managedptr.h>
#include <bslmf_movableref.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbplug {
class PluginManager;
class PluginFactory;
class AuthenticationResult;
}

namespace mqbcfg {
class AuthorizerConfig;
}

namespace mqbauthz {

// =============================
// class AuthorizationController
// =============================

class AuthorizationController {
  public:
    // TYPES
    typedef bslma::ManagedPtr<mqbplug::Authorizer> AuthorizerMp;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBAUTHZ.AUTHORIZATIONCONTROLLER");

    // PRIVATE TYPES
    typedef bsl::unordered_set<mqbplug::PluginFactory*> PluginFactories;

    // DATA

    /// Registered authorizer plugin
    AuthorizerMp d_authorizer_mp;

    // NOT IMPLEMENTED
    AuthorizationController(const AuthorizationController& other)
        BSLS_KEYWORD_DELETED;
    AuthorizationController&
    operator=(const AuthorizationController& other) BSLS_KEYWORD_DELETED;

    // PRIVATE STATIC MEMBERS

    /// Collect all available plugin factories of the specified `type`
    /// (built-in and external).  Load them into the specified
    /// `pluginFactories` collection.
    static void collectAvailablePluginFactories(
        PluginFactories*              pluginFactories,
        const mqbplug::PluginManager& pluginManager,
        mqbplug::PluginType::Enum     type);

    /// Create authorizer based on configuration from the specified
    /// `authorizerConfig` using the specified `pluginFactories`.
    /// Return 0 on success, non-zero on error.
    static int createConfiguredAuthorizer(
        AuthorizerMp*                   result,
        bsl::ostream&                   errorDescription,
        const PluginFactories&          pluginFactories,
        const mqbcfg::AuthorizerConfig& authorizerConfig,
        bsl::allocator<>                allocator);

    /// Ensure at least one authorizer is available for default
    /// Authorization, adding default BasicAuthorizer if none are
    /// configured.
    static void ensureDefaultAuthorizer(AuthorizerMp*    result,
                                        bsl::allocator<> allocator);

  public:
    // CREATORS

    AuthorizationController(bslmf::MovableRef<AuthorizerMp> authorizer);

    /// Create the AuthorizationController.  Return 0 on success, or a non-zero
    /// return code on error and fill in the specified `errorDescription`
    /// stream with the description of the error.
    ///
    /// @param[out] result The place to construct the authorization controller
    /// @param[out] errorDescription A stream where details of an error message
    /// will be printed, if an error ocurred.
    /// @param authorizerConfig The authorization configuration for the
    /// application
    /// @param pluginManager The plugin manager for the application
    /// @param allocator An allocator to use for the
    /// @returns A non-zero value if an error ocurred, or zero on success.
    static int
    allocateManaged(bslma::ManagedPtr<AuthorizationController>* result,
                    bsl::ostream&                   errorDescription,
                    const mqbcfg::AuthorizerConfig& authorizerConfig,
                    const mqbplug::PluginManager&   pluginManager,
                    const bsl::allocator<>& allocator = bsl::allocator<>());

    // ACCESSORS

    /// Get the authorizer configured on this object.
    mqbplug::Authorizer& authorizer();
};

}  // close package namespace
}  // close enterprise namespace

#endif
