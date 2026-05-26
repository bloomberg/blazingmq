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

#ifndef INCLUDED_MQBNET_AUTHENTICATIONCLIENT
#define INCLUDED_MQBNET_AUTHENTICATIONCLIENT

/// @file mqbnet_authenticationclient.h
///
/// @brief Provide a protocol for a client-side authenticator.
///
/// @bbref{mqbnet::AuthenticationClient} is a protocol for the client side of
/// broker-to-broker authentication for a single connection.  An
/// implementation sends an @bbref{bmqp_ctrlmsg::AuthenticationRequest}
/// on an outbound channel and processes the resulting
/// @bbref{bmqp_ctrlmsg::AuthenticationResponse}.

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_ostream.h>

namespace BloombergLP {

namespace mqbnet {

// ==========================
// class AuthenticationClient
// ==========================

/// Protocol for the client side of broker-to-broker authentication.
class AuthenticationClient {
  public:
    // CREATORS

    /// Destructor
    virtual ~AuthenticationClient();

    // MANIPULATORS

    /// Send an AuthenticationRequest using credentials from the configured
    /// provider.  Return 0 on success, or a non-zero value and populate the
    /// specified `errorDescription` with the description of any failure
    /// encountered.
    virtual int authenticate(bsl::ostream& errorDescription) = 0;

    /// Process the specified `response` received from the remote broker.
    /// Return 0 if authentication succeeded, or a non-zero value and
    /// populate the specified `errorDescription` with the description of
    /// any failure encountered.
    virtual int
    handleResponse(bsl::ostream&                              errorDescription,
                   const bmqp_ctrlmsg::AuthenticationMessage& response) = 0;

    /// Stop the authentication client, cancelling any pending
    /// reauthentication timers.
    virtual void stop() = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
