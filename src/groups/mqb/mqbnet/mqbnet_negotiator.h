// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbnet_negotiator.h                                                -*-C++-*-
#ifndef INCLUDED_MQBNET_NEGOTIATOR
#define INCLUDED_MQBNET_NEGOTIATOR

//@PURPOSE: Provide a protocol for a session negotiator.
//
//@CLASSES:
//  mqbnet::Negotiator:        protocol for a session negotiator
//
//@SEE_ALSO:
//  mqbnet::Session: protocol of the negotiated sessions
//
//@DESCRIPTION: 'mqbnet::Negotiator' is a protocol for a session negotiator
// that uses a provided established channel to negotiate and create an
// 'mqbnet::Session' object.  'mqbnet::InitialConnectionContext' is a
// value-semantic type holding the context associated with a session being
// negotiated.  It allows bi-directional generic communication between the
// application layer and the transport layer: for example, a user data
// information can be passed in at application layer, kept and carried over in
// the transport layer and retrieved in the negotiator concrete implementation.
// Similarly, a 'cookie' can be passed in from application layer, to the result
// callback notification in the transport layer (usefull for 'listen-like'
// established connection where the entry point doesn't allow to bind specific
// user data, which then can be retrieved at application layer during
// negotiation).
//
// Note that the 'InitialConnectionContext' passed in to the
// 'InitialConnectionHandler::handleInitialConnection()' method can be modified
// in the InitialConnectionHandler concrete implementation to set some of its
// members that the caller will leverage and use.

// MQB
#include <mqbnet_negotiationcontext.h>

// BDE
#include <bsl_memory.h>

namespace BloombergLP {

namespace mqbnet {

// FORWARD DECLARATION
class Session;

// ================
// class Negotiator
// ================

/// Protocol for a session negotiator.
class Negotiator {
  public:
    // CREATORS

    /// Destructor
    virtual ~Negotiator();

    // MANIPULATORS

    /// Create a `session` based on the type of initial connection message in
    /// the specified `context`.  Set `isContinueRead` to true if we want to
    /// continue reading instead of creating a session just yet.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int createSessionOnMsgType(
        bsl::ostream&                              errorDescription,
        bsl::shared_ptr<mqbnet::Session>*          session,
        bool*                                      isContinueRead,
        const bsl::shared_ptr<NegotiationContext>& context) = 0;

    /// Send out outbound negotiation message with the specified `context`.
    /// Return 0 on success, or a non-zero error code and populate the
    /// specified `errorDescription` with a description of the error otherwise.
    virtual int
    negotiateOutbound(bsl::ostream& errorDescription,
                      const bsl::shared_ptr<NegotiationContext>& context) = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
