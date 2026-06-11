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

#ifndef INCLUDED_MQBPLUG_CREDENTIALPROVIDER
#define INCLUDED_MQBPLUG_CREDENTIALPROVIDER

//@PURPOSE: Provide base classes for the 'CredentialProvider' plugin.
//
//@CLASSES:
//  mqbplug::CredentialProvider: interface for a plugin of type
//                               'e_CREDENTIAL_PROVIDER'.
//  mqbplug::CredentialProviderPluginFactory: base class for the
//                                           'CredentialProvider'
//                                           plugin factory.
//
//@DESCRIPTION: This component provides definitions for the
// 'mqbplug::CredentialProvider' interface and
// 'mqbplug::CredentialProviderPluginFactory' used as base classes for
// plugins that supply credentials for outbound broker-to-broker
// authentication.  A 'CredentialProvider' produces a callable that
// returns an 'AuthnCredential' when invoked.

// MQB
#include <mqbplug_authncredential.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_optional.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace mqbplug {

// ========================
// class CredentialProvider
// ========================

/// Interface for a plugin that supplies credentials for outbound
/// broker-to-broker authentication.
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class CredentialProvider {
  public:
    // TYPES

    /// A callable that returns an `AuthnCredential` on success, or
    /// `nullopt` on failure (with a description written to the `error`
    /// stream).  The broker invokes this function each time it needs
    /// credentials for an outbound connection or reauthentication.
    typedef bsl::function<bsl::optional<AuthnCredential>(bsl::ostream& error)>
        CredentialCb;

    // CREATORS

    /// Destroy this object.
    virtual ~CredentialProvider();

    // MANIPULATORS

    /// Return a credential-providing function.  This method is called once
    /// at broker startup after `start()` succeeds; the returned function is
    /// stored and invoked for each outbound connection and
    /// reauthentication.  The behavior is undefined unless `start()` has
    /// been called successfully.
    virtual CredentialCb load() = 0;

    /// Start the CredentialProvider and return 0 on success, or return
    /// a non-zero value and populate the specified `errorDescription` with
    /// the description of any failure encountered.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop the CredentialProvider.
    virtual void stop() = 0;
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

// ======================================
// class CredentialProviderPluginFactory
// ======================================

/// This is the base class for the factory of plugins of type
/// `CredentialProvider`.  It allows instantiation of a concrete
/// `CredentialProvider` object.
// NOLINTBEGIN(cppcoreguidelines-special-member-functions)
class CredentialProviderPluginFactory : public PluginFactory {
  public:
    // CREATORS
    CredentialProviderPluginFactory();

    ~CredentialProviderPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    virtual bslma::ManagedPtr<CredentialProvider>
    create(bslma::Allocator* allocator) = 0;
};
// NOLINTEND(cppcoreguidelines-special-member-functions)

}  // close package namespace
}  // close enterprise namespace

#endif
