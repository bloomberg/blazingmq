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

#ifndef INCLUDED_MQBPLUG_AUTHORIZER
#define INCLUDED_MQBPLUG_AUTHORIZER

/// @file mqbplug_authorizer.h
///
/// @brief Provide base classes for the 'Authorizer' plugin.

// MQB
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_variant.h>
#include <bslma_bslallocator.h>
#include <bslmf_iscopyconstructible.h>
#include <bslmf_movableref.h>
#include <bsls_compilerfeatures.h>
#include <bslstl_inplace.h>

namespace BloombergLP {

// FORWARD DECLARATION

namespace mqbcfg {
class AuthorizerPluginConfig;
}

namespace mqbact {
class Action;
}

namespace mqbplug {

// ===================
// class Authorizer
// ===================

/// Interface for an Authorizer.
class Authorizer {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~Authorizer();

    // ACESSORS

    /// Return the name of the plugin.
    ///
    /// This method is valid to call at any time after construction, including
    /// before `start()` is called.
    virtual bsl::string_view name() const = 0;

    // MANIPULATORS

    /// Check if the supplied action is allowed based on the result of
    /// authentication.
    ///
    /// @param action The action being authorized
    /// @param authnResult The result of an authenticated connection
    virtual bool authorize(const mqbact::Action&       action,
                           const AuthenticationResult& authnResult) = 0;
};

// ================================
// class AuthorizerPluginFactory
// ================================

/// This is the base class for the factory of plugins of type `Authorizer`.
/// All it does is allows to instantiate a concrete object of the
/// `Authorizer` interface, taking any required (plugin specific) arguments.
class AuthorizerPluginFactory : public PluginFactory {
  public:
    // CREATORS
    AuthorizerPluginFactory();

    ~AuthorizerPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    virtual bslma::ManagedPtr<Authorizer>
    create(bslma::Allocator* allocator) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
