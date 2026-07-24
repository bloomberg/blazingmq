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

#ifndef INCLUDED_MQBI_AUTHORIZER
#define INCLUDED_MQBI_AUTHORIZER

/// @file mqbi_authorizer.h
///
/// @brief Provide a protocol for an authorizer.

namespace BloombergLP {

// FORWARD DECLARATION

namespace mqbact {
class Action;
}

namespace mqbplug {
class AuthenticationResult;
}

namespace mqbi {

// ================
// class Authorizer
// ================

/// Protocol for an authorizer
class Authorizer {
  public:
    // CREATORS

    /// Destructor
    virtual ~Authorizer();

    // MANIPULATORS

    /// Check if the supplied action is allowed based on the result of
    /// authentication.
    ///
    /// @param action The action being authorized
    /// @param authnResult The result of an authenticated connection
    virtual bool
    authorize(const mqbact::Action&                action,
              const mqbplug::AuthenticationResult& authnResult) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
