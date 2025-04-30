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

// BMQ

// MQB
#include <bslstl_unorderedmap.h>
#include <mqbplug_authenticator.h>
#include <mqbplug_pluginmanager.h>

// BDE
#include <ball_log.h>
#include <bsl_unordered_map.h>
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
    typedef bslma::ManagedPtr<mqbplug::Authenticator> AuthenticatorMp;
    typedef bsl::unordered_map<bslstl::StringRef, AuthenticatorMp>
        AuthenticatorMap;

    // DATA

    /// Used to instantiate 'Authenticator'
    /// plugins at start-time.
    mqbplug::PluginManager* d_pluginManager_p;

    /// Registered authenticators
    /// Mapping an authentication mechanism to a mqbplug::Authenticator
    AuthenticatorMap d_authenticators;

    /// Fallback principal
    bsl::string d_principal;

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

    void stop();

    int authenticate(bsl::ostream&                      errorDescription,
                     mqbplug::AuthenticationResult*     result,
                     const bslstl::StringRef&           mechanism,
                     const mqbplug::AuthenticationData& input);
};

}  // close package namespace
}  // close enterprise namespace

#endif
