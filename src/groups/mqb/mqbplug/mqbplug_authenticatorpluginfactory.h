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

// mqbplug_authenticatorpluginfactory.h                               -*-C++-*-
#ifndef INCLUDED_MQBPLUG_AUTHENTICATORPLUGINFACTORY
#define INCLUDED_MQBPLUG_AUTHENTICATORPLUGINFACTORY

//@PURPOSE: Provide base classes for the 'Authenticator' plugin.
//
//@CLASSES:
//  mqbplug::AuthenticationData: interface for accessing data used for
//                               authentication.
//  mqbplug::Authenticator: interface for a plugin of type 'e_AUTHENTICATOR'.
//  mqbplug::AuthenticatorPluginFactory: base class for the 'Authenticator'
//                                       plugin factory.
//
//@DESCRIPTION: This component provide definitions for classes
// 'mqbplug::AuthenticationData', 'mqbplug::Authenticator',
// 'mqbplug::AuthenticatorPluginFactory' used as base classes for plugins that
// publish statistics and for their factories.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_timeinterval.h>
#include <bslstl_stringref.h>

#include <ntci_upgradable.h>

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

    /// Authenticate the provided authentication data. Return true on success
    /// and populate the client's identity in the specified `principal`,
    /// or false upon failure and populate the specified `errorDescription`
    /// with a brief reason for logging purposes. If the client's identity
    /// has a fixed lifetime, populate the lifetimeMs with the remaining
    /// duration.
    virtual bool authenticate(bsl::ostream&             errorDescription,
                              bsl::string&              principal,
                              int&                      lifetimeMs,
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

}  // close package namespace
}  // close enterprise namespace

#endif
