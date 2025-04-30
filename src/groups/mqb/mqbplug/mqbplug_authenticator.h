// Copyright 2025 Bloomberg Finance L.P.
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

// mqbplug_authenticator.h                                            -*-C++-*-
#ifndef INCLUDED_MQBPLUG_AUTHENTICATOR
#define INCLUDED_MQBPLUG_AUTHENTICATOR

//@PURPOSE: Provide base classes for the 'Authenticator' plugin.
//
//@CLASSES:
//  mqbplug::AuthenticationData: interface for accessing data used for
//                               authentication.
//  mqbplug::AuthenticationResult: the results of authentication.
//  mqbplug::Authenticator: interface for a plugin of type 'e_AUTHENTICATOR'.
//  mqbplug::AuthenticatorPluginFactory: base class for the 'Authenticator'
//                                       plugin factory.
//
//@DESCRIPTION: This component provide definitions for classes
// 'mqbplug::AuthenticationData', 'mqbplug::AuthenticationResult',
// 'mqbplug::Authenticator', 'mqbplug::AuthenticatorPluginFactory' used
// as base classes for plugins that publish statistics and for their factories.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslstl_stringref.h>

namespace BloombergLP {
namespace mqbplug {

// ========================
// class AuthenticationData
// ========================

/// Interface for accessing data used during authentication in the broker.
class AuthenticationData {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~AuthenticationData();

    // ACESSORS

    /// Return the authentication material provided by the client.
    virtual const bsl::vector<char>& authPayload() const = 0;

    /// Return the IP Address of the client.
    virtual bslstl::StringRef clientIpAddress() const = 0;
};

// ==========================
// class AuthenticationResult
// ==========================

/// The results of authentication.
class AuthenticationResult BSLS_KEYWORD_FINAL {
  private:
    // DATA

    /// Client identity.
    bsl::string d_principal;

    /// The remaining lifetime of a client identity.
    /// Empty value if client identity has a non-fixed lifetime.
    bsl::optional<bsls::Types::Int64> d_lifetimeMs;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AuthenticationResult,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Construct this object using the optionally specified `allocator`.
    explicit AuthenticationResult(bslma::Allocator* allocator = 0);

    // ACCESSORS

    /// Return the corresponding data field.
    const bsl::string&                       principal() const;
    const bsl::optional<bsls::Types::Int64>& lifetimeMs() const;

    // MANIPULATORS
    /// Reset the state of this structure.
    void reset();

    /// Set the corresponding field of this structure.
    void setPrincipal(const bslstl::StringRef& principal);
    void setLifetimeMs(bsls::Types::Int64 lifetimeMs);
};

// ===================
// class Authenticator
// ===================

/// Interface for an Authenticator.
class Authenticator {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~Authenticator();

    // ACESSORS

    /// Return the name of the plugin.
    virtual bslstl::StringRef name() const = 0;

    /// Return the authentication mechanism the Authenticator supports.
    virtual bslstl::StringRef mechanism() const = 0;

    /// Authenticate using the data provided in the specified `input`.
    /// - Return `0` on success, and populate the specified `result` with
    ///   client identity and its remaining lifetime if it has a fixed
    ///   duration.
    /// - Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes.
    virtual int authenticate(bsl::ostream&             errorDescription,
                             AuthenticationResult*     result,
                             const AuthenticationData& input) const = 0;

    // MANIPULATORS

    /// Start the Authenticator and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop the Authenticator.
    virtual void stop() = 0;
};

// ================================
// class AuthenticatorPluginFactory
// ================================

/// This is the base class for the factory of plugins of type `Authenticator`.
/// All it does is allows to instantiate a concrete object of the
/// `Authenticator` interface, taking any required (plugin specific) arguments.
class AuthenticatorPluginFactory : public PluginFactory {
  public:
    // CREATORS
    AuthenticatorPluginFactory();

    ~AuthenticatorPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    virtual bslma::ManagedPtr<Authenticator>
    create(bslma::Allocator* allocator) = 0;
};

// ========================
// struct AuthenticatorUtil
// ========================

struct AuthenticatorUtil {
    // STATIC CLASS METHODS

    /// Find authenticator config with the specified `name`
    // TODO: statconsumer pass in `bslstl::StringRef`. should it be const & ?
    static const mqbcfg::AuthenticatorPluginConfig*
    findAuthenticatorConfig(const bslstl::StringRef& name);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class AuthenticationResult
// --------------------------

// CREATORS

inline AuthenticationResult::AuthenticationResult(bslma::Allocator* allocator)
: d_principal(allocator)
, d_lifetimeMs()
{
    // NOTHING
}

// ACCESSORS

inline const bsl::string& AuthenticationResult::principal() const
{
    return d_principal;
}

inline const bsl::optional<bsls::Types::Int64>&
AuthenticationResult::lifetimeMs() const
{
    return d_lifetimeMs;
}

// MANIPULATORS

inline void AuthenticationResult::reset()
{
    d_principal.clear();
    d_lifetimeMs.reset();
}

inline void
AuthenticationResult::setPrincipal(const bslstl::StringRef& principal)
{
    d_principal = principal;
}

inline void AuthenticationResult::setLifetimeMs(bsls::Types::Int64 lifetimeMs)
{
    d_lifetimeMs = lifetimeMs;
}

}  // close package namespace
}  // close enterprise namespace

#endif
