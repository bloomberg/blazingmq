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

// mqbnet_authenticationcontext.h                                -*-C++-*-
#ifndef INCLUDED_MQBNET_AUTHENTICATIONCONTEXT
#define INCLUDED_MQBNET_AUTHENTICATIONCONTEXT

/// @file mqbnet_authenticationcontext.h
///
/// @brief Provide the context for authenticating connections.
///

// MQB
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

// BDE
#include <bslmt_mutex.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace mqbnet {

// ===========================
// class AuthenticationContext
// ===========================

/// VST for the context associated with an connection being authenticated.
class AuthenticationContext {
  public:
    // TYPES
    enum State {
        e_AUTHENTICATING = 0,
        e_AUTHENTICATED,
    };

    typedef bsl::function<int(
        bsl::ostream&                                 errorDescription,
        const bsl::shared_ptr<AuthenticationContext>& context,
        const bsl::shared_ptr<bmqio::Channel>&        channel)>
        ReauthenticateCb;

  private:
    // DATA

    /// The authentication result to be used for authorization. It is first set
    /// during the initial authentication, and can be updated later
    /// during re-authentication.
    bsl::shared_ptr<mqbplug::AuthenticationResult> d_authenticationResultSp;

    /// The mutex to protect the AuthenticationResult.
    mutable bslmt::Mutex d_mutex;

    InitialConnectionContext* d_initialConnectionContext_p;

    bmqp_ctrlmsg::AuthenticationMessage d_authenticationMessage;

    /// The encoding type used for sending a message. It should match with the
    /// encoding type of the received message.
    bmqp::EncodingType::Enum d_authenticationEncodingType;

    ReauthenticateCb     d_reauthenticateCb;
    bsls::AtomicInt      d_state;
    ConnectionType::Enum d_connectionType;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AuthenticationContext(const AuthenticationContext&);  // = delete;
    AuthenticationContext&
    operator=(const AuthenticationContext&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(NegotiationContext,
                                   bslma::UsesBslmaAllocator)
    // CREATORS
    AuthenticationContext(
        InitialConnectionContext*                  initialConnectionContext,
        const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage,
        bmqp::EncodingType::Enum                   authenticationEncodingType,
        const ReauthenticateCb&                    reauthenticateCb,
        State                                      state,
        ConnectionType::Enum connectionType = ConnectionType::e_UNKNOWN);

    // MANIPULATORS
    AuthenticationContext& setAuthenticationResult(
        const bsl::shared_ptr<mqbplug::AuthenticationResult>& value);
    AuthenticationContext&
    setInitialConnectionContext(InitialConnectionContext* value);
    AuthenticationContext&
    setAuthenticationMessage(const bmqp_ctrlmsg::AuthenticationMessage& value);
    AuthenticationContext&
    setAuthenticationEncodingType(bmqp::EncodingType::Enum value);
    AuthenticationContext&
    setAuthenticateCallback(const ReauthenticateCb& value);
    AuthenticationContext& setConnectionType(ConnectionType::Enum value);

    bsls::AtomicInt& state();

    // ACCESSORS

    /// This function holds a mutex lock while accessing the
    /// `d_authenticationResultSp` to ensure thread safety.
    const bsl::shared_ptr<mqbplug::AuthenticationResult>&
    authenticationResult() const;

    InitialConnectionContext* initialConnectionContext() const;
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage() const;
    bmqp::EncodingType::Enum authenticationEncodingType() const;
    const ReauthenticateCb&  reauthenticateCb() const;
    ConnectionType::Enum     connectionType() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
