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

#ifndef INCLUDED_MQBA_AUTHORIZER
#define INCLUDED_MQBA_AUTHORIZER

/// @file mqba_authorizer.h
///
/// @brief Provide an authorizer for authenticating a connection.

// MQB
#include <mqbi_authorizer.h>

// BDE
#include <bsl_ostream.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace mqbauthz {
class AuthorizationController;
}

namespace mqba {

// ===================
// class Authorizer
// ===================

/// authorizer for a BlazingMQ session with client or broker
class Authorizer : public mqbi::Authorizer {
  private:
    // DATA

    /// Authorization Controller.
    mqbauthz::AuthorizationController* d_authzController_p;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    Authorizer(const Authorizer&) BSLS_KEYWORD_DELETED;
    Authorizer& operator=(const Authorizer&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a new `authorizer` adapting the specified `authnController`.
    explicit Authorizer(mqbauthz::AuthorizationController* authnController);

    /// Destructor
    ~Authorizer() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS

    /// Check if the supplied action is allowed based on the result of
    /// authentication.
    ///
    /// @param action The action being authorized
    /// @param authnResult The result of an authenticated connection
    bool authorize(const mqbact::Action&                action,
                   const mqbplug::AuthenticationResult& authnResult)
        BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
