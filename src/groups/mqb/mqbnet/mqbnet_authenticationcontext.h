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

// BDE
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

  private:
    // DATA
    InitialConnectionContext*           d_initialConnectionContext_p;
    bmqp_ctrlmsg::AuthenticationMessage d_authenticationMessage;
    bsls::AtomicInt                     d_state;
    bool                                d_isReversed;
    ConnectionType::Enum                d_connectionType;

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
        bool                                       isReversed,
        State                                      state,
        ConnectionType::Enum connectionType = ConnectionType::e_UNKNOWN);

    // MANIPULATORS
    AuthenticationContext&
    setInitialConnectionContext(InitialConnectionContext* value);
    AuthenticationContext&
    setAuthenticationMessage(const bmqp_ctrlmsg::AuthenticationMessage& value);
    AuthenticationContext& setIsReversed(bool value);
    AuthenticationContext& setConnectionType(ConnectionType::Enum value);

    bsls::AtomicInt& state();

    // ACCESSORS
    InitialConnectionContext* initialConnectionContext() const;
    const bmqp_ctrlmsg::AuthenticationMessage& authenticationMessage() const;
    bool                                       isReversed() const;
    ConnectionType::Enum                       connectionType() const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
