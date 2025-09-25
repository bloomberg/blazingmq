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
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>

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

    /// Start the Authenticator.  Return 0 on success, or a non-zero error
    /// code and populate the specified `errorDescription` with a description
    /// of the error otherwise.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop the Authenticator.
    virtual void stop() = 0;

    /// Authenticate the connection based on the type of AuthenticationMessage
    /// in the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int handleAuthentication(
        bsl::ostream&                                    errorDescription,
        const bsl::shared_ptr<InitialConnectionContext>& context,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMsg) = 0;

    /// Send out outbound authentication message with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int authenticationOutbound(
        const bsl::shared_ptr<AuthenticationContext>& context) = 0;

    /// Schedule a re-authentication job in the thread pool using the
    /// specified `context` and `channel`.  Return 0 on success, or a
    /// non-zero error code and populate the specified `errorDescription`
    /// with a description of the error otherwise.
    virtual int
    reauthenticateAsync(bsl::ostream& errorDescription,
                        const bsl::shared_ptr<AuthenticationContext>& context,
                        const bsl::shared_ptr<bmqio::Channel>& channel) = 0;

    /// Methods invoked when a channel is closed.
    virtual void
    onClose(const bsl::shared_ptr<AuthenticationContext>& context) = 0;

    // ACCESSORS

    /// Return the anonymous credential used for authentication.
    /// If no anonymous credential is set, return an empty optional.
    virtual const bsl::optional<mqbcfg::Credential>&
    anonymousCredential() const = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
