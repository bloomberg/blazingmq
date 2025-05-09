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

// mqbnet_authenticator.h -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATOR
#define INCLUDED_MQBNET_AUTHENTICATOR

/// @file mqbnet_authenticator.h
///
/// @brief Provide a protocol for an authenticator.
///
/// @bbref{mqbnet::Authenticator} is a protocol for an authenticator that
/// (re)authenticates a connection with a BlazingMQ client or another bmqbrkr.

// MQB
#include <mqbnet_authenticationcontext.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_memory.h>

namespace BloombergLP {

namespace mqbnet {

// ===================
// class Authenticator
// ===================

/// Protocol for an Authenticator
class Authenticator {
  public:
    // CREATORS

    /// Destructor
    virtual ~Authenticator();

    // MANIPULATORS

    /// Authenticate the connection based on the type of AuthenticationMessage
    /// in the specified `context`.  Set `isContinueRead` to true if we want to
    /// continue reading instead of finishing authentication.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int handleAuthentication(
        bsl::ostream&                                 errorDescription,
        bool*                                         isContinueRead,
        const bsl::shared_ptr<AuthenticationContext>& context) = 0;

    /// Send out outbound authentication message or reverse connection request
    /// with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int authenticationOutboundOrReverse(
        const bsl::shared_ptr<AuthenticationContext>& context) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
