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
#include <mqbplug_pluginfactory.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_iostream.h>
#include <bsl_optional.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcfg {
class AuthenticatorPluginConfig;
}

namespace mqbplug {

// ========================
// class AuthenticationData
// ========================

/// Data used during authentication in the broker.
class AuthenticationData BSLS_KEYWORD_FINAL {
  private:
    // DATA
    bsl::vector<char> d_authnPayload;
    bsl::string       d_clientIpAddress;

  public:
    // CREATORS
    AuthenticationData(const bsl::vector<char>& authnPayload,
                       bsl::string_view         clientIpAddress,
                       bslma::Allocator*        allocator = 0);

    // ACESSORS

    /// Return the authentication material provided by the client.
    const bsl::vector<char>& authnPayload() const;

    /// Return the IP Address of the client.
    bsl::string_view clientIpAddress() const;
};

// ==========================
// class AuthenticationResult
// ==========================

/// The results of authentication.
class AuthenticationResult {
  public:
    // CREATORS
    virtual ~AuthenticationResult();

    // ACCESSORS

    /// Return the principal in human-readable format.
    virtual bsl::string_view principal() const = 0;

    /// Return the remaining lifetime of an authenticated session.
    virtual const bsl::optional<bsls::Types::Int64>& lifetimeMs() const = 0;
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
    virtual bsl::string_view name() const = 0;

    /// Return the authentication mechanism supported by this Authenticator.
    /// Guaranteed to return a valid mechanism once the Authenticator is
    /// constructed.
    virtual bsl::string_view mechanism() const = 0;

    /// Authenticate using the data provided in the specified `input`.
    /// - Return `0` on success, and populate the specified `result` with
    ///   client identity and its remaining lifetime if it has a fixed
    ///   duration.
    /// - Return a non-zero plugin-specific return code upon failure, and
    ///   populate the specified `errorDescription` with a brief reason for
    ///   logging purposes.
    virtual int
    authenticate(bsl::ostream& errorDescription,
                 bsl::shared_ptr<mqbplug::AuthenticationResult>* result,
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
    static const mqbcfg::AuthenticatorPluginConfig*
    findAuthenticatorConfig(bsl::string_view name);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class AuthenticationData
// ------------------------

inline AuthenticationData::AuthenticationData(
    const bsl::vector<char>& authnPayload,
    bsl::string_view         clientIpAddress,
    bslma::Allocator*        allocator)
: d_authnPayload(authnPayload)
, d_clientIpAddress(clientIpAddress, allocator)
{
}

inline const bsl::vector<char>& AuthenticationData::authnPayload() const
{
    return d_authnPayload;
}

inline bsl::string_view AuthenticationData::clientIpAddress() const
{
    return d_clientIpAddress;
}

}  // close package namespace
}  // close enterprise namespace

#endif
