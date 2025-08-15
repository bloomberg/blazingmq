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

// mqbnet_initialconnectionhandler.h                            -*-C++-*-
#ifndef INCLUDED_MQBNET_INITIALCONNECTIONHANDLER
#define INCLUDED_MQBNET_INITIALCONNECTIONHANDLER

/// @file mqbnet_initialconnectionhandler.h
/// @brief Provide a handler for initial connection.
///
/// @bbref{mqbnet::InitialConnectionHandler} reads from IO (if this is an
/// incoming connection) and commands authenticator and negotiator.  A session
/// would be created at the end upon success.

// MQB
#include <mqbnet_initialconnectioncontext.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_memory.h>
#include <bsl_optional.h>
#include <bsl_variant.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqio {
class Channel;
}

namespace mqbnet {

// ==============================
// class InitialConnectionHandler
// ==============================

class InitialConnectionHandler {
  public:
    // TYPES
    typedef bsl::shared_ptr<mqbnet::InitialConnectionContext>
        InitialConnectionContextSp;

  public:
    // CREATORS

    /// Destructor
    virtual ~InitialConnectionHandler();

    // MANIPULATORS

    /// Method invoked by the client of this object to negotiate a session.
    /// The specified `context` is an in-out member holding the initial
    /// connection context to use, including an `InitialConnectionCompleteCb`,
    /// which must be called with the result, whether success or failure, of
    /// the initial connection.
    /// The InitialConnectionHandler concrete implementation can modify some of
    /// the members during the initial connection (i.e., between the
    /// `handleInitialConnection()` method and the invocation of the
    /// `InitialConnectionCompleteCb` method.  Note that if no initial
    /// connection is needed, the `InitialConnectionCompleteCb` may be invoked
    /// directly from inside the call to `handleInitialConnection()`.
    virtual void
    handleInitialConnection(const InitialConnectionContextSp& context) = 0;

    /// Handle an event occurs under the current state.
    /// The specified `input` is the event to handle, the specified `context`
    /// is the initial connection context associated to this event, and the
    /// specified `message` is an optional message that may be used to
    /// handle the event.
    virtual void handleEvent(
        mqbnet::InitialConnectionEvent::Enum input,
        const InitialConnectionContextSp&    context,
        const bsl::optional<bsl::variant<bmqp_ctrlmsg::AuthenticationMessage,
                                         bmqp_ctrlmsg::NegotiationMessage> >&
            message = bsl::nullopt) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
